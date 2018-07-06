#include "SceneGraphNode.h"
#include "Utility/Headers/BaseClasses.h"
#include "Terrain/Terrain.h"
#include "Terrain/Water.h"
#include "Hardware/Video/Light.h"
#include "Geometry/Object3D.h"
#include "Managers/SceneManager.h"
#include "RenderQueue.h"

SceneGraphNode::SceneGraphNode(SceneNode* node) : _node(node), 
												  _parent(NULL),
												  _grandParent(NULL),
												  _wasActive(true),
											      _active(true),
								                  _transform(NULL),
								                  _noDefaultTransform(false),
												  _inView(true),
												  _sorted(false),
												  _sceneGraph(NULL),
												  _updateTimer(GETMSTIME()),
												  _childQueue(0)
{
}


SceneGraphNode::~SceneGraphNode(){
	if(_transform != NULL) {
		delete _transform;
		_transform = NULL;
	} 
}

bool SceneGraphNode::unload(){
	foreach(NodeChildren::value_type it, _children){
		if(it.second->unload()){

		}else{
			Console::getInstance().errorfn("SceneGraphNode: Could not unload node [ %s ]",it.second->getNode()->getName().c_str());
			return false;
		}
	}
	//Console::getInstance().printfn("Removing: %s (%s)",_node->getName().c_str(), getName().c_str());
	_children.clear();

	if(getParent()){ //if not root
		RemoveResource(_node);
	}
	return true;
}

void SceneGraphNode::print(){
	SceneGraphNode* parent = this;
	U8 i = 0;
	while(parent != NULL){
		parent = parent->getParent();
		i++;
	}
	Material* mat = getNode()->getMaterial();
	std::string material("none"),shader("none");
	if(mat){
		material = mat->getName();
		if(mat->getShader()){
			shader = mat->getShader()->getName();
		}
	}
	Console::getInstance().printfn("%s (Resource: %s, Material: %s (Shader: %s) )", getName().c_str(),getNode()->getName().c_str(),material.c_str(),shader.c_str());
	foreach(NodeChildren::value_type it, _children){
		for(U8 j = 0; j < i; j++){
			Console::getInstance().printf("-");
		}
		it.second->print();
	}
		
}
void SceneGraphNode::setParent(SceneGraphNode* parent){
	if(getParent()){
		getParent()->removeNode(_node);
	}
	_parent = parent;
}

void SceneGraphNode::removeNode(SceneNode* node){
	SceneGraphNode* tempNode = findNode(node->getName());
	if(tempNode){
		_children.erase(_children.find(tempNode->getName()));
	}
}

SceneGraphNode* SceneGraphNode::addNode(SceneNode* node,const std::string& name){
	if(node->getSceneGraphNode()){
		SceneGraphNode* pSGN = node->getSceneGraphNode()->getParent();
		if(pSGN){
			pSGN->removeNode(node);
		}
	}
	SceneGraphNode* sceneGraphNode = New SceneGraphNode(node);
	SceneGraphNode* parentNode = this->getParent();
	assert(sceneGraphNode);
	
	sceneGraphNode->setParent(this);
	sceneGraphNode->setGrandParent(parentNode);

	Transform* nodeTransform = sceneGraphNode->getTransform();

	if(parentNode){
		if(_grandParent){//If we have a parent, and our parent isn't root ..
			Transform* parentTransform = parentNode->getTransform(); // ... get this parent's transform ...
			if(nodeTransform && parentTransform){ // ... and as long as the parent has a transform ...
				nodeTransform->setParentMatrix(parentTransform->getMatrix()); // ... store it as a parent matrix ...
			}
		}
	}

	std::pair<unordered_map<std::string, SceneGraphNode*>::iterator, bool > result;
	std::string sgName(name);
	
	if(sgName.empty()){
		sgName = node->getName();
	}
	result = _children.insert(std::make_pair(sgName,sceneGraphNode));
	if(!result.second){
		delete (result.first)->second; 
		(result.first)->second = sceneGraphNode;
	}
	sceneGraphNode->setName(sgName);
	node->postLoad(sceneGraphNode);
	return sceneGraphNode;
}

SceneGraphNode* SceneGraphNode::findNode(const std::string& name){
	if(_node->getName().compare(name) == 0){ //if the current node is what we are looking for, return it
		return this;
	}else{ //else, check the immediat children if they match
		NodeChildren::iterator findResult = _children.find(name);
		if(findResult != _children.end()){
			return findResult->second;
		}else{ //else, for each child, check it's children
			foreach(NodeChildren::value_type it, _children){
				SceneGraphNode* result = it.second->findNode(name);
				if(result) return result;

			}
		}
	}
	return NULL; //finally ... return nothing
}

void SceneGraphNode::updateTransformsAndBounds(){
	//if(GETMSTIME() - _updateTimer  < 10) return; //update every ten milliseconds

	//Update from leafs to root:
	foreach(NodeChildren::value_type &it, _children){
		it.second->updateTransformsAndBounds();
	}

	//Compute the BoundingBox if it isn't already
	if(!_boundingBox.isComputed()){
		//Mesh BB is composed of SubMesh BB's. Compute from leaf to root to ensure proper calculations
		_node->computeBoundingBox(this);
	}
	Transform* transform = getTransform();
	if(transform){
		//Transform the bounding box if we have a new transform
		//if(getTransform()->isDirty()){
			_boundingBox.Transform(_initialBoundingBox,transform->getMatrix());
		//}
		if(getParent()){
			//update parent matrix (cached)
			transform->setParentMatrix(getParent()->getTransform()->getMatrix());
		}
	}
	
	if(getParent()){
		getParent()->getBoundingBox().Add(getBoundingBox());
	}

	//_updateTimer = GETMSTIME();
}

void SceneGraphNode::updateVisualInformation(){
	//Bounding Boxes should be updated, so we can early cull now.
	Scene* curentScene = SceneManager::getInstance().getActiveScene();
	//Hold a pointer to the scenegraph
	if(!_sceneGraph){
		_sceneGraph = SceneManager::getInstance().getActiveScene()->getSceneGraph();
	}

	//if current node is active
	if(_active) {
		if(_node->isInView(true,_boundingBox)){
			_inView = (curentScene->drawObjects() && _active && _node->getRenderState());
		}
		if(_inView){
		//	continue;//ToDo: --Early cull here? -Ionut
		//}else{
			RenderQueue::getInstance().addNodeToQueue(this);
		}
	}

	foreach(NodeChildren::value_type &it, _children){
		it.second->updateVisualInformation();
	}
}

Transform* SceneGraphNode::getTransform(){
	if(!_transform && !_noDefaultTransform){
		_transform = New Transform();
		assert(_transform);
	}
	return _transform;
}

template<class T>
T* SceneGraphNode::getNode(){return dynamic_cast<T>(_node);}
template<>
WaterPlane* SceneGraphNode::getNode<WaterPlane>(){ return dynamic_cast<WaterPlane*>(_node); }
template<>
Terrain* SceneGraphNode::getNode<Terrain>(){ return dynamic_cast<Terrain*>(_node); }
template<>
Light* SceneGraphNode::getNode<Light>(){ return dynamic_cast<Light*>(_node); }
template<>
Object3D* SceneGraphNode::getNode<Object3D>(){ return dynamic_cast<Object3D*>(_node); }
