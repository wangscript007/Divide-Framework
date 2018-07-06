#include "Headers/SubMesh.h"
#include "Managers/Headers/ResourceManager.h"


bool SubMesh::computeBoundingBox(SceneGraphNode* const node){
	BoundingBox& bb = node->getBoundingBox();
	if(bb.isComputed()) return true;
	bb.set(vec3(100000.0f, 100000.0f, 100000.0f),vec3(-100000.0f, -100000.0f, -100000.0f));

	std::vector<vec3>&	tPosition	= _geometry->getPosition();

	for(U32 i=0; i < tPosition.size(); i++){
		bb.Add( tPosition[i] );
	}

	return SceneNode::computeBoundingBox(node);
}


bool SubMesh::unload(){
	getIndices().clear(); 
	return SceneNode::unload();
}