#include "GFXDevice.h"
#include "Core/Headers/Application.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Graphs/Headers/RenderQueue.h"

using namespace std;

void GFXDevice::setApi(RenderAPI api){

	switch(api)	{

	case 0: //OpenGL 1.0
	case 1: //OpenGL 1.2
	case 2: //OpenGL 2.0
	case 3: //OpenGL 2.1
	case 4: //OpenGL 2.2
	case 5: //OpenGL 3.0
	case 6: //OpenGL 3.2
	default:
		_api = GL_API::getInstance();
		break;

	case 7: //DX 8
	case 8: //DX 9
	case 9: //DX 10
		_api = DX_API::getInstance();
		break;
	case 10: //Placeholder
		break;
	//Ionut: OpenGL 4.0 and DX 11 in another life maybe :)
	};
	_api.setId(api);
}

void GFXDevice::resizeWindow(U16 w, U16 h){

	Application::getInstance().setWindowWidth(w);
    Application::getInstance().setWindowHeight(h);
	_api.resizeWindow(w,h);
	PostFX::getInstance().reshapeFBO(w, h);
}

void GFXDevice::renderModel(Object3D* const model){
	//All geometry is stored in VBO format
	if(!model) return;

	if(model->getIndices().empty()){
		model->onDraw(); //something wrong! Re-run pre-draw tests!
	}
	_api.renderModel(model);

}

void GFXDevice::setRenderStage(RENDER_STAGE stage){
	_renderStage = stage;
}

void GFXDevice::setRenderState(RenderState& state,bool force) {
	//Memorize the previous rendering state as the current state before changing
	_previousRenderState = _currentRenderState;
	//Apply the new updates to the state
	_api.setRenderState(state,force);
	//Update the current state
	_currentRenderState = state;
}

void GFXDevice::toggleWireframe(bool state){
	_wireframeMode = !_wireframeMode;
	_api.toggleWireframe(_wireframeMode);
}

bool _drawBBoxes = false;
void GFXDevice::processRenderQueue(){

	SceneNode* sn = NULL;
	SceneGraphNode* sgn = NULL;
	Transform* t = NULL;
	//Shadows are applied only in the final stage
	if(getRenderStage() == FINAL_STAGE){ 
		//Tell the lightManager to bind all available depth maps
		LightManager::getInstance().bindDepthMaps();
	}

	//Sort the render queue by the specified key
	//This only affects the final rendering stage
	RenderQueue::getInstance().sort();

	//Draw the entire queue;
	//Limited to 65536 (2^16) items per queue pass!
	for(U16 i = 0; i < RenderQueue::getInstance().getRenderQueueStackSize(); i++){
		//Get the current scene node
		sgn = RenderQueue::getInstance().getItem(i)._node;
		//And validate it
		assert(sgn);
		//Get it's transform
		t = sgn->getTransform();
		//And it's attached SceneNode
		sn = sgn->getNode();
		//Validate the SceneNode
		assert(sn);
		//Call any pre-draw operations on the SceneNode (refresh VBO, update materials, etc)
		sn->onDraw();
		//Transform the Object (Rot, Trans, Scale)
		setObjectState(t);
		//setup materials and render the node
		//As nodes are sorted, this should be very fast
		//We need to apply different materials for each stage
		switch(getRenderStage()){
			case FINAL_STAGE:
			case DEFERRED_STAGE:
			case REFLECTION_STAGE:
			case ENVIRONMENT_MAPPING_STAGE:
				//ToDo: Avoid changing states by not unbind old material/shader/states and checking agains default
				sn->prepareMaterial(sgn);
				break;
		}
		//Call the rendering interface for the current SceneNode
		//The stage exclusion mask should do the rest
		sn->render(sgn); 
		//Unbind current material properties
		//ToDo: Optimize this!!!! -Ionut
		switch(getRenderStage()){
			case FINAL_STAGE:
			case DEFERRED_STAGE:
			case REFLECTION_STAGE:
			case ENVIRONMENT_MAPPING_STAGE:
				sn->releaseMaterial();
				break;
		}
		//Drop all applied transformations so that they do not affect the next node
		releaseObjectState(t);
		sn->postDraw();
	}
	//Unbind all depth maps 
	if(getRenderStage() == FINAL_STAGE){ 
		LightManager::getInstance().unbindDepthMaps();
	}

	_drawBBoxes = SceneManager::getInstance().getActiveScene()->drawBBox();
	if(getRenderStage() == FINAL_STAGE){
		//Unbind all shaders
		ShaderManager::getInstance().unbind();
		//Draw all BBoxes
		for(U16 i = 0; i < RenderQueue::getInstance().getRenderQueueStackSize(); i++){
			//Get the current scene node
			sgn = RenderQueue::getInstance().getItem(i)._node;
			//draw bounding box if needed and only in the final stage to prevent Shadow/PostFX artifcats
			BoundingBox& bb = sgn->getBoundingBox();
			//Draw the bounding box if it's always on or if the scene demands it
			if(bb.getVisibility() || _drawBBoxes){
				//This is the only immediate mode draw call left in the rendering api's
				drawBox3D(bb.getMin(),bb.getMax());
			}
		}
	}
}


void GFXDevice::renderInViewport(const vec4& rect, boost::function0<void> callback){
	_api.renderInViewport(rect,callback);
}

void  GFXDevice::generateCubeMap(FrameBufferObject& cubeMap, const vec3& pos, boost::function0<void> callback){
	//Don't need to override cubemap rendering callback
	if(callback.empty()){
		SceneGraph* sg = SceneManager::getInstance().getActiveScene()->getSceneGraph();
		//Default case is that everything is reflected
		callback = boost::bind(boost::bind(&SceneGraph::render, sg));
	}
	//Only use cube map FBO's
	if(cubeMap.getType() != FrameBufferObject::FBO_CUBE_COLOR){
		Console::getInstance().errorfn("GFXDevice: trying to generate cubemap in invalid FBO type!");
		return;
	}
	//Get some global vars
	Camera* cam = CameraManager::getInstance().getActiveCamera();
	ParamHandler& par = ParamHandler::getInstance();
	F32 zNear  = par.getParam<F32>("zNear");
	F32 zFar   = par.getParam<F32>("zFar");
	//Save our current camera settings
	cam->SaveCamera();
	//And save all camera transform matrices
	lockModelView();
	lockProjection();
	//set a 90 degree vertical FoV perspective projection
	setPerspectiveProjection(90.0,1,vec2(zNear,zFar));
	//Save our old rendering stage
	RENDER_STAGE prev = getRenderStage();
	//And set the current render stage to 
	setRenderStage(ENVIRONMENT_MAPPING_STAGE);
	//For each of the environment's faces (TOP,DOWN,NORTH,SOUTH,EAST,WEST)
	for(U8 i = 0; i < 6; i++){
		//Set the correct camera orientation and position for current face
		cam->RenderLookAtToCubeMap( pos, i );
		//Bind our FBO's current face
		cubeMap.Begin(i);
			//draw our scene
			callback();
		//Unbind this face
		cubeMap.End(i);
	}
	//Return to our previous rendering stage
	setRenderStage(prev);
	//Restore transfom matrices
	releaseProjection();
	releaseModelView();
	//And restore camera
	cam->RestoreCamera();
}