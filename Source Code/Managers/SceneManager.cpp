#include "SceneManager.h"
#include "Managers/CameraManager.h"
#include "Rendering/Application.h"
#include "Rendering/Frustum.h"
#include "SceneList.h"
#include "Utility/Headers/ParamHandler.h"

using namespace std;

SceneManager::SceneManager() : _scene(NULL), _currentSelection(NULL){}


Scene* SceneManager::loadScene(const string& name){
	Scene* scene = NULL;
	if(name.compare("MainScene") == 0){
		scene = New MainScene();
		_resDB.insert(make_pair("MainScene", scene ));	
	}else if(name.compare("CubeScene") == 0){
		scene = New CubeScene();
		_resDB.insert(make_pair("CubeScene", scene));
	}else if(name.compare("NetworkScene") == 0){
		scene = New NetworkScene();
		_resDB.insert(make_pair("NetworkScene", scene));
	}else if(name.compare("PingPongScene") == 0){
		scene = New PingPongScene();
		_resDB.insert(make_pair("PingPongScene", scene));
	}else if(name.compare("FlashScene") == 0){
		scene = New FlashScene();
		_resDB.insert(make_pair("FlashScene", scene));
	}else if(name.compare("AITenisScene") == 0){
		scene = New AITenisScene();
		_resDB.insert(make_pair("AITenisScene", scene));
	}else{
		scene = NULL;
	}
	return scene;
}

void SceneManager::toggleBoundingBoxes(){
	if(!_scene->drawBBox() && _scene->drawObjects())	{
		_scene->drawBBox(true);
		_scene->drawObjects(true);
	}else if (_scene->drawBBox() && _scene->drawObjects()){
		_scene->drawBBox(true);
		_scene->drawObjects(false);
	}else{
		_scene->drawBBox(false);
		_scene->drawObjects(true);
	}
}

void SceneManager::deleteSelection(){
	if(_currentSelection != NULL){
		_currentSelection->scheduleDeletion();
	}
}

void SceneManager::findSelection(U32 x, U32 y){
	ParamHandler& par = ParamHandler::getInstance();
    F32 value_fov = 0.7853f;    //this is 45 degrees converted to radians
    F32 value_aspect = par.getParam<F32>("aspectRatio");
	F32 half_window_width = Application::getInstance().getWindowDimensions().width / 2.0f;
	F32 half_window_height = Application::getInstance().getWindowDimensions().height / 2.0f;

    F32 modifier_x, modifier_y;
        //mathematical handling of the difference between
        //your mouse position and the 'center' of the window

    vec3 point;
        //the untransformed ray will be put here

    F32 point_dist = par.getParam<F32>("zFar");
        //it'll be put this far on the Z plane

    vec3 camera_origin;
        //this is where the camera sits, in 3dspace

    vec3 point_xformed;
        //this is the transformed point

    vec3 final_point;
    vec4 color(0.0, 1.0, 0.0, 1.0);

    //These lines are the biggest part of this function.
    //This is where the mouse position is turned into a mathematical
    //'relative' of 3d space. The conversion to an actual point
    modifier_x = (F32)tan( value_fov * 0.5f )
        * (( 1.0f - x / half_window_width ) * ( value_aspect ) );
    modifier_y = (F32)tan( value_fov * 0.5f )
        * -( 1.0f - y / half_window_height );

    //These 3 take our modifier_x/y values and our 'casting' distance
    //to throw out a point in space that lies on the point_dist plane.
    //If we were using completely untransformed, untranslated space,
    //this would be fine - but we're not :)
    point.x = modifier_x * point_dist;
    point.y = modifier_y * point_dist;
    point.z = point_dist;

    //Next we make an openGL call to grab our MODELVIEW_MATRIX -
    //This is the matrix that rasters 3d points to 2d space - which is
    //kinda what we're doing, in reverse
	mat4 temp = Frustum::getInstance().getModelviewMatrix();
	
    //Some folks would then invert the matrix - I invert the results.

    //First, to get the camera_origin, we transform the 12, 13, 14
    //slots of our pulledMatrix - this gets us the actual viewing
    //position we are 'sitting' at when the function is called
    camera_origin.x = -(
        temp.mat[0] * temp.mat[12] +
        temp.mat[1] * temp.mat[13] +
        temp.mat[2] * temp.mat[14]);
    camera_origin.y = -(
        temp.mat[4] * temp.mat[12] +
        temp.mat[5] * temp.mat[13] +
        temp.mat[6] * temp.mat[14]);
    camera_origin.z = -(
        temp.mat[8] * temp.mat[12] +
        temp.mat[9] * temp.mat[13] +
        temp.mat[10] * temp.mat[14]);

    //Second, we transform the position we generated earlier - the '3d'
    //mouse position - by our viewing matrix.
    point_xformed.x = -(
        temp.mat[0] * point[0] +
        temp.mat[1] * point[1] +
        temp.mat[2] * point[2]);
    point_xformed.y = -(
        temp.mat[4] * point[0] +
        temp.mat[5] * point[1] +
        temp.mat[6] * point[2]);
    point_xformed.z = -(
        temp.mat[8] * point[0] +
        temp.mat[9] * point[1] +
        temp.mat[10] * point[2]);

    final_point = point_xformed + camera_origin;

	vec3 origin(CameraManager::getInstance().getActiveCamera()->getEye());
	vec3 dir = origin.direction(final_point);
	
	//ToDo: fix this!!!!! -Ionut
	/*Ray r(origin,dir);
	_currentSelection = NULL;
	for(unordered_map<string, Object3D* >::iterator it = getGeometryArray().begin(); 
													 it != getGeometryArray().end(); it++){
		assert(it->second != NULL);
		(it->second)->setSelected(false);
		
		if((it->second)->getBoundingBox().intersect(r,par.getParam<F32>("zNear"),par.getParam<F32>("zFar")/2.0f)){
			(it->second)->setSelected(true);
			_currentSelection = it->second;
			break;
		}
	}*/
}