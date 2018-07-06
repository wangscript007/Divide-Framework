#include "Headers/SceneGraphNode.h"
#include "Core/Headers/BaseClasses.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Water/Headers/Water.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Geometry/Shapes/Headers/Object3D.h"


SceneGraphNode::SceneGraphNode(SceneNode* node) : _node(node), 
												  _parent(NULL),
												  _grandParent(NULL),
												  _wasActive(true),
											      _active(true),
								                  _transform(NULL),
								                  _noDefaultTransform(false),
												  _inView(true),
												  _sorted(false),
												  _silentDispose(false),
												  _sceneGraph(NULL),
												  _updateTimer(GETMSTIME()),
												  _childQueue(0)
{
}

//If we are destroyng the current graph node
SceneGraphNode::~SceneGraphNode(){
	//delete children nodes recursively
	for_each(NodeChildren::value_type& it, _children){
		delete it.second;
		it.second = NULL;
	}

	//and delete the transform bound to this node
	if(_transform != NULL) {
		delete _transform;
		_transform = NULL;
	} 
	_children.clear();

}
std::vector<BoundingBox >&  SceneGraphNode::getBBoxes(std::vector<BoundingBox >& boxes ){
	//Unload every sub node recursively
	for_each(NodeChildren::value_type& it, _children){
		it.second->getBBoxes(boxes);
	}
	boxes.push_back(getBoundingBox());
	return boxes;
}

//When unloading the current graph node
bool SceneGraphNode::unload(){
	//Unload every sub node recursively
	for_each(NodeChildren::value_type& it, _children){
		it.second->unload();
	}
	//Some debug output ...
	if(!_silentDispose && getParent() && _node){
		Console::getInstance().printfn("Removing: %s (%s)",_node->getName().c_str(), getName().c_str());
	}
	//if not root
	if(getParent()){
		//Remove the SceneNode that we own
		RemoveResource(_node);
	}
	return true;
}

//Print's out the SceneGraph structure to the Console
void SceneGraphNode::print(){
	//Starting from the current node
	SceneGraphNode* parent = this;
	U8 i = 0;
	//Count how deep in the graph we are 
	//by counting how many ancestors we have before the "root" node
	while(parent != NULL){
		parent = parent->getParent();
		i++;
	}
	//get out material's name
	Material* mat = getNode()->getMaterial();
	//Some strings to hold the names of our material and shader
	std::string material("none"),shader("none");
	//If we have a material
	if(mat){
		//Get the material's name
		material = mat->getName();
		//If we have a shader
		if(mat->getShaderProgram()){
			//Get the shader's name
			shader = mat->getShaderProgram()->getName();
		}
	}
	//Print our current node's information
	Console::getInstance().printfn("%s (Resource: %s, Material: %s (Shader: %s) )", getName().c_str(),getNode()->getName().c_str(),material.c_str(),shader.c_str());
	//Repeat for each child, but prefix it with the appropriate number of dashes
	//Based on our ancestor counting earlier
	for_each(NodeChildren::value_type& it, _children){
		for(U8 j = 0; j < i; j++){
			Console::getInstance().printf("-");
		}
		it.second->print();
	}
}

//Change current SceneGraphNode's parent
void SceneGraphNode::setParent(SceneGraphNode* parent) {
	if(_parent){
		//Remove us from the old parent's children map
		NodeChildren::iterator& it = _parent->getChildren().find(getName());
		_parent->getChildren().erase(it);
	}
	//Set the parent pointer to the new parent
	_parent = parent;
	//Add ourselves in the new parent's children map
	//Time to add it to the children map
	std::pair<unordered_map<std::string, SceneGraphNode*>::iterator, bool > result;
	//Try and add it to the map
	result = _parent->getChildren().insert(std::make_pair(getName(),this));
	//If we had a collision (same name?)
	if(!result.second){
		//delete the old SceneGraphNode
		delete (result.first)->second; 
		//And add this one instead
		(result.first)->second = this;
	}
	//That's it. Parent Transforms will be updated in the next render pass;
}

//Add a new SceneGraphNode to the current node's child list based on a SceneNode
SceneGraphNode* SceneGraphNode::addNode(SceneNode* const node,const std::string& name){
	//Create a new SceneGraphNode with the SceneNode's info
	SceneGraphNode* sceneGraphNode = New SceneGraphNode(node);
	//Validate it to be safe
	assert(sceneGraphNode);
	//We need to name the new SceneGraphNode
	//We start off with the name passed as a parameter as it has a higher priority
	std::string sgName(name);
	//If we did not supply a custom name
	if(sgName.empty()){
		//Use the SceneNode's name
		sgName = node->getName();
	}
	//Name the new SceneGraphNode
	sceneGraphNode->setName(sgName);

	//Get our current's node parent
	SceneGraphNode* parentNode = getParent();
	//Set the current node's parent as the new node's grandparent
	sceneGraphNode->setGrandParent(parentNode);
	//Get the new node's transform
	Transform* nodeTransform = sceneGraphNode->getTransform();
	//If the current node and the new node have transforms, 
	//Update the relationship between the 2
	if(nodeTransform && getTransform()){
		//The child node's parentMatrix is our current transform matrix
		if(parentNode){
			nodeTransform->setParentMatrix(getTransform()->getMatrix());
		}
	}

	//Set the current node as the new node's parrent	
	sceneGraphNode->setParent(this);

	//Do all the post load operations on the SceneNode
	//Pass a reference to the newly created SceneGraphNode in case we need transforms or bounding boxes
	node->postLoad(sceneGraphNode);
	//return the newly created node
	return sceneGraphNode;
}

//Remove a child node from this Node
void SceneGraphNode::removeNode(SceneGraphNode* node){
	//find the node in the children map
	NodeChildren::iterator& it = _children.find(node->getName());
	//If we found the node we are looking for
	if(it != _children.end()) {
		//Remove it from the map
		_children.erase(it);
	}
	//Beware. Removing a node, does no unload it!
	//Call delete on the SceneGraphNode's pointer to do that
}

//Finding a node based on the name of the SceneGraphNode or the SceneNode it holds
//Switching is done by setting sceneNodeName to false if we pass a SceneGraphNode name
//or to true if we search by a SceneNode's name
SceneGraphNode* SceneGraphNode::findNode(const std::string& name, bool sceneNodeName){
	//Null return value as default
	SceneGraphNode* returnValue = NULL;
	 //Make sure a name exists
	if (!name.empty()){
		//check if it is the name we are looking for
		//If we are searching for a SceneNode ...
		if(sceneNodeName){
			if (_node->getName().compare(name) == 0){
				// We got the node!
				return this;
			}
		}else{ //If we are searching for a SceneGraphNode ...
			if (getName().compare(name) == 0){
				// We got the node!
				return this;
			}
		}

		//The current node isn't the one we wan't, so recursively check all children
		for_each(NodeChildren::value_type& it, _children){
			returnValue = it.second->findNode(name);
				if(returnValue != NULL){
					// if it is not NULL it is the node we are looking for
					// so just pass it through
					return returnValue;
			}
		}
	}
	
    // no children's name matches or there are no more children
    // so return NULL, indicating that the node was not found yet
    return NULL;
}

//This updates the SceneGraphNode's transform by deleting the old one first
void SceneGraphNode::setTransform(Transform* const t) {
	delete _transform; 
	_transform = t;
}

//Get the node's transform
Transform* const SceneGraphNode::getTransform(){
	//A node does not necessarily have a transform
	//If this is the case, we can either create a default one
	//Or return NULL. When creating a node we can specify if we do not want a default transform
	if(!_noDefaultTransform && !_transform){
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