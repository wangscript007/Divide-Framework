#include "Headers/PhysXSceneInterface.h"
#include "Scenes/Headers/Scene.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

using namespace physx;

bool PhysXSceneInterface::init(){
	//Create the scene
	if(!PhysX::getInstance().getSDK()){
		Console::getInstance().errorfn("PhysX SDK not initialized!");
		return false;
	}
	PxSceneDesc sceneDesc(PhysX::getInstance().getSDK()->getTolerancesScale());
	sceneDesc.gravity=PxVec3(0.0f, -9.8f, 0.0f);
	if(!sceneDesc.cpuDispatcher) {
		PxDefaultCpuDispatcher* mCpuDispatcher = PxDefaultCpuDispatcherCreate(1);
		if(!mCpuDispatcher){
			Console::getInstance().errorfn("PhysX Scene Interface: PxDefaultCpuDispatcherCreate failed!");
		}
		 sceneDesc.cpuDispatcher = mCpuDispatcher;
	}
	if(!sceneDesc.filterShader){
		sceneDesc.filterShader  = PhysX::getInstance().getFilterShader();
	}

	_gScene = PhysX::getInstance().getSDK()->createScene(sceneDesc);
	if (!_gScene){
		Console::getInstance().errorfn("PhysX Scene Interface: error creating scene!");
		return false;
	}

	_gScene->setVisualizationParameter(PxVisualizationParameter::eSCALE,     1.0);
	_gScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
	PhysX::getInstance().registerActiveScene(this);
	return true;
}

bool PhysXSceneInterface::exit(){
	Console::getInstance().d_printfn("Shuting down PhysXSceneInterface");
	return true;
}

void PhysXSceneInterface::idle(){
}

void PhysXSceneInterface::release(){
	if(!_gScene){
		Console::getInstance().errorfn("PhysX: trying to close invalid scene!");
		return;
	}
	_gScene->release();
}

void PhysXSceneInterface::update(){
	if(!_gScene) return;
	boost::lock_guard<boost::mutex> lock(_creationMutex);
	for_each(PxRigidDynamic* actor, _sceneRigidDynamicActors){
		updateActor(actor);

	}
	for_each(PxRigidStatic* actor, _sceneRigidStaticActors){
		updateActor(actor);
	}
}

void PhysXSceneInterface::updateActor(PxRigidActor* actor){
   PxU32 nShapes = actor->getNbShapes(); 
   PxShape** shapes=New PxShape*[nShapes];
   actor->getShapes(shapes, nShapes);
   Transform* t = static_cast<Transform*>(actor->userData);
   while (nShapes--){ 
      updateShape(shapes[nShapes], t); 
   } 
   delete [] shapes;
}

void PhysXSceneInterface::updateShape(PxShape* shape, Transform* t){
	if(!t || !shape) return;
	PxTransform pT = PxShapeExt::getGlobalPose(*shape);
	if(shape->getGeometryType() == PxGeometryType::ePLANE){
		t->scale(shape->getActor().getObjectSize());
		//ToDo: Remove hack! Find out why plane isn't rotating - Ionut
		t->rotate(vec3(1,0,0),90);
	}else{
		t->rotateQuaternion(Quaternion(pT.q.x,pT.q.y,pT.q.z,pT.q.w));
	}
	t->setPosition(vec3(pT.p.x,pT.p.y,pT.p.z));
	
	
}

void PhysXSceneInterface::process(){
	if(!_gScene) return;
	_gScene->simulate(_timeStep);  
	_gScene->fetchResults(true);
}

void PhysXSceneInterface::addRigidStaticActor(physx::PxRigidStatic* actor) {
	Console::getInstance().printfn("CALLED1");
	if(!PhysX::getInstance().getSDK()) return;
	Console::getInstance().printfn("CALLED2");
	if(!_gScene) return;
	Console::getInstance().printfn("CALLED3");
	_creationMutex.lock();
	Console::getInstance().printfn("CALLED4");
	_sceneRigidStaticActors.push_back(actor);
	Console::getInstance().printfn("CALLED5");
	_creationMutex.unlock();
	Console::getInstance().printfn("CALLED6");
	_gScene->addActor(*actor);
	Console::getInstance().printfn("CALLED7");
	addToSceneGraph(actor);
	Console::getInstance().printfn("CALLED8");
}

void PhysXSceneInterface::addRigidDynamicActor(physx::PxRigidDynamic* actor) {
	if(!PhysX::getInstance().getSDK()) return;
	if(!_gScene) return;
	_creationMutex.lock();
	_sceneRigidDynamicActors.push_back(actor); 
	_creationMutex.unlock();
	_gScene->addActor(*actor);
	addToSceneGraph(actor);
}

void PhysXSceneInterface::addToSceneGraph(PxRigidActor* actor){
	Console::getInstance().printfn("CALLEDA");
	if(!actor) return;
	Console::getInstance().printfn("CALLEDB");
	U32 nbActors = _gScene->getNbActors(PxActorTypeSelectionFlag::eRIGID_DYNAMIC | PxActorTypeSelectionFlag::eRIGID_STATIC);
	std::stringstream ss;
	PxShape** shapes=New PxShape*[actor->getNbShapes()];
	actor->getShapes(shapes, actor->getNbShapes());   
	//ToDo: Only 1 shape per actor for now. Fix This! -Ionut
	Object3D* actorGeometry = NULL;
	Console::getInstance().printfn("CALLEDC");
	switch(shapes[0]->getGeometryType()){
		 case PxGeometryType::eBOX: {
			ss << "BoxActor_" << nbActors;
			actorGeometry = ResourceManager::getInstance().loadResource<Box3D>(ResourceDescriptor(ss.str()));
			PxBoxGeometry box;
			shapes[0]->getBoxGeometry(box);
			dynamic_cast<Box3D*>(actorGeometry)->setSize(box.halfExtents.x * 2);
	   }
       break;
	   case PxGeometryType::ePLANE:
		   ss << "PlaneActor_" << nbActors;
		   actorGeometry = ResourceManager::getInstance().loadResource<Quad3D>(ResourceDescriptor(ss.str()));
	   break;
	} 
    delete [] shapes;
	if(actorGeometry){
		_creationMutex.lock();
		SceneGraphNode* tempNode = _parentScene->addGeometry(actorGeometry);
		_creationMutex.unlock();
		actor->userData = (void*)tempNode->getTransform();
	}
}