#include "Headers/Scene.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"

#include "Utility/Headers/XMLParser.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

#include "Environment/Sky/Headers/Sky.h"
#include "Environment/Water/Headers/Water.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

#include "Physics/Headers/PhysicsSceneInterface.h"


namespace Divide {

namespace {
struct selectionQueueDistanceFrontToBack {
    selectionQueueDistanceFrontToBack(const vec3<F32>& eyePos)
        : _eyePos(eyePos) {}

    bool operator()(const SceneGraphNode_wptr& a, const SceneGraphNode_wptr& b) const {
        F32 dist_a =
            a.lock()->get<BoundsComponent>()->getBoundingBox().nearestDistanceFromPointSquared(_eyePos);
        F32 dist_b =
            b.lock()->get<BoundsComponent>()->getBoundingBox().nearestDistanceFromPointSquared(_eyePos);
        return dist_a > dist_b;
    }

   private:
    vec3<F32> _eyePos;
};
};

Scene::Scene(PlatformContext& context, const stringImpl& name)
    : Resource(ResourceType::DEFAULT, name),
      _context(context),
      _LRSpeedFactor(5.0f),
      _loadComplete(false),
      _cookCollisionMeshesScheduled(false),
      _pxScene(nullptr),
      _paramHandler(ParamHandler::instance())
{
    _sceneTimer = 0UL;
    _sceneState = MemoryManager_NEW SceneState(*this);
    _input = MemoryManager_NEW SceneInput(*this, _context._INPUT);
    _sceneGraph = MemoryManager_NEW SceneGraph(*this);
    _aiManager = MemoryManager_NEW AI::AIManager(*this);
    _lightPool = MemoryManager_NEW LightPool(*this, _context._GFX);
    _envProbePool = MemoryManager_NEW SceneEnvironmentProbePool(*this);

    _GUI = MemoryManager_NEW SceneGUIElements(*this, _context.gui());

    if (Config::Build::IS_DEBUG_BUILD) {
        RenderStateBlock primitiveDescriptor;
        _linesPrimitive = _context._GFX.newIMP();
        _linesPrimitive->name("LinesRayPick");
        _linesPrimitive->stateHash(primitiveDescriptor.getHash());
        _linesPrimitive->paused(true);
    } else {
        _linesPrimitive = nullptr;
    }

}

Scene::~Scene()
{
    MemoryManager::DELETE(_sceneState);
    MemoryManager::DELETE(_input);
    MemoryManager::DELETE(_sceneGraph);
    MemoryManager::DELETE(_aiManager);
    MemoryManager::DELETE(_lightPool);
    MemoryManager::DELETE(_envProbePool);
    MemoryManager::DELETE(_GUI);
    for (IMPrimitive*& prim : _octreePrimitives) {
        MemoryManager::DELETE(prim);
    }
}

bool Scene::onStartup() {
    return true;
}

bool Scene::onShutdown() {
    return true;
}

bool Scene::frameStarted() {
    std::unique_lock<std::mutex> lk(_perFrameArenaMutex);
    _perFrameArena.clear();
    return true;
}

bool Scene::frameEnded() {
    return true;
}

bool Scene::idle() {  // Called when application is idle
    if (!_modelDataArray.empty()) {
        loadXMLAssets(true);
    }

    if (!_sceneGraph->getRoot().hasChildren()) {
        return false;
    }

    _sceneGraph->idle();

    Attorney::SceneRenderStateScene::playAnimations(
        renderState(),
        ParamHandler::instance().getParam<bool>(_ID("mesh.playAnimations"), true));

    if (_cookCollisionMeshesScheduled && checkLoadFlag()) {
        if (_context._GFX.getFrameCount() > 1) {
            _sceneGraph->getRoot().get<PhysicsComponent>()->cookCollisionMesh(_name);
            _cookCollisionMeshesScheduled = false;
        }
    }

    if (Config::Build::IS_DEBUG_BUILD) {
        _linesPrimitive->paused(!renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_DEBUG_LINES));
    }

    _lightPool->idle();

    return true;
}

void Scene::addMusic(MusicType type, const stringImpl& name, const stringImpl& srcFile) {
    ResourceDescriptor music(name);
    music.setResourceLocation(srcFile);
    music.setFlag(true);
    hashAlg::emplace(state().music(type),
                     _ID_RT(name),
                     CreateResource<AudioDescriptor>(music));
}

void Scene::addPatch(vectorImpl<FileData>& data) {
}

void Scene::loadXMLAssets(bool singleStep) {
    while (!_modelDataArray.empty()) {
        const FileData& it = _modelDataArray.top();
        // vegetation is loaded elsewhere
        if (it.type == GeometryType::VEGETATION) {
            _vegetationDataArray.push_back(it);
        } else  if (it.type == GeometryType::PRIMITIVE) {
            loadGeometry(it);
        } else {
            loadModel(it);
        }
        _modelDataArray.pop();

        if (singleStep) {
            break;
        }
    }
}

bool Scene::loadModel(const FileData& data) {
    static const U32 normalMask = to_const_uint(SGNComponent::ComponentType::NAVIGATION) |
                                  to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                  to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                  to_const_uint(SGNComponent::ComponentType::RENDERING);

    ResourceDescriptor model(data.ModelName);
    model.setResourceLocation(data.ModelName);
    model.setFlag(true);
    Mesh_ptr thisObj = CreateResource<Mesh>(model);
    if (!thisObj) {
        Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD_MODEL")),
                         data.ModelName.c_str());
        return false;
    }

    SceneGraphNode_ptr meshNode =
        _sceneGraph->getRoot().addNode(thisObj,
                                       normalMask,
                                       data.physicsUsage ? data.physicsStatic ? PhysicsGroup::GROUP_STATIC
                                                                              : PhysicsGroup::GROUP_DYNAMIC
                                                         : PhysicsGroup::GROUP_IGNORE,
                                       data.ItemName);
    meshNode->get<RenderingComponent>()->castsShadows(data.castsShadows);
    meshNode->get<RenderingComponent>()->receivesShadows(data.receivesShadows);
    meshNode->get<PhysicsComponent>()->setScale(data.scale);
    meshNode->get<PhysicsComponent>()->setRotation(data.orientation);
    meshNode->get<PhysicsComponent>()->setPosition(data.position);
    if (data.staticUsage) {
        meshNode->usageContext(SceneGraphNode::UsageContext::NODE_STATIC);
    }
    if (data.navigationUsage) {
        meshNode->get<NavigationComponent>()->navigationContext(NavigationComponent::NavigationContext::NODE_OBSTACLE);
    }

    if (data.useHighDetailNavMesh) {
        meshNode->get<NavigationComponent>()->navigationDetailOverride(true);
    }
    return true;
}

bool Scene::loadGeometry(const FileData& data) {
    static const U32 normalMask = to_const_uint(SGNComponent::ComponentType::NAVIGATION) |
                                  to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                  to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                  to_const_uint(SGNComponent::ComponentType::RENDERING);

    std::shared_ptr<Object3D> thisObj;
    ResourceDescriptor item(data.ItemName);
    item.setResourceLocation(data.ModelName);
    if (data.ModelName.compare("Box3D") == 0) {
        item.setPropertyList(Util::StringFormat("%2.2f", data.data));
        thisObj = CreateResource<Box3D>(item);
    } else if (data.ModelName.compare("Sphere3D") == 0) {
        thisObj = CreateResource<Sphere3D>(item);
        static_cast<Sphere3D*>(thisObj.get())->setRadius(data.data);
    } else if (data.ModelName.compare("Quad3D") == 0) {
        vec3<F32> scale = data.scale;
        vec3<F32> position = data.position;
        P32 quadMask;
        quadMask.i = 0;
        quadMask.b[0] = 1;
        item.setBoolMask(quadMask);
        thisObj = CreateResource<Quad3D>(item);
        static_cast<Quad3D*>(thisObj.get())
            ->setCorner(Quad3D::CornerLocation::TOP_LEFT, vec3<F32>(0, 1, 0));
        static_cast<Quad3D*>(thisObj.get())
            ->setCorner(Quad3D::CornerLocation::TOP_RIGHT, vec3<F32>(1, 1, 0));
        static_cast<Quad3D*>(thisObj.get())
            ->setCorner(Quad3D::CornerLocation::BOTTOM_LEFT, vec3<F32>(0, 0, 0));
        static_cast<Quad3D*>(thisObj.get())
            ->setCorner(Quad3D::CornerLocation::BOTTOM_RIGHT, vec3<F32>(1, 0, 0));
    } else if (data.ModelName.compare("Text3D") == 0) {
        /// set font file
        item.setResourceLocation(data.data3);
        item.setPropertyList(data.data2);
        thisObj = CreateResource<Text3D>(item);
        static_cast<Text3D*>(thisObj.get())->setWidth(data.data);
    } else {
        Console::errorfn(Locale::get(_ID("ERROR_SCENE_UNSUPPORTED_GEOM")),
                         data.ModelName.c_str());
        return false;
    }
    STUBBED("Load material from XML disabled for primitives! - Ionut")
    Material_ptr tempMaterial = nullptr; /* = XML::loadMaterial(data.ItemName + "_material")*/;
    if (!tempMaterial) {
        ResourceDescriptor materialDescriptor(data.ItemName + "_material");
        tempMaterial = CreateResource<Material>(materialDescriptor);
        tempMaterial->setDiffuse(data.colour);
        tempMaterial->setShadingMode(Material::ShadingMode::BLINN_PHONG);
    }

    thisObj->setMaterialTpl(tempMaterial);
    SceneGraphNode_ptr thisObjSGN = _sceneGraph->getRoot().addNode(thisObj,
                                                                   normalMask,
                                                                   data.physicsUsage ? data.physicsStatic ? PhysicsGroup::GROUP_STATIC
                                                                                                          : PhysicsGroup::GROUP_DYNAMIC
                                                                                     : PhysicsGroup::GROUP_IGNORE);
    thisObjSGN->get<PhysicsComponent>()->setScale(data.scale);
    thisObjSGN->get<PhysicsComponent>()->setRotation(data.orientation);
    thisObjSGN->get<PhysicsComponent>()->setPosition(data.position);
    thisObjSGN->get<RenderingComponent>()->castsShadows(data.castsShadows);
    thisObjSGN->get<RenderingComponent>()->receivesShadows(data.receivesShadows);
    if (data.staticUsage) {
        thisObjSGN->usageContext(SceneGraphNode::UsageContext::NODE_STATIC);
    }
    if (data.navigationUsage) {
        thisObjSGN->get<NavigationComponent>()->navigationContext(NavigationComponent::NavigationContext::NODE_OBSTACLE);
    }

    if (data.useHighDetailNavMesh) {
        thisObjSGN->get<NavigationComponent>()->navigationDetailOverride(true);
    }
    return true;
}

SceneGraphNode_ptr Scene::addParticleEmitter(const stringImpl& name,
                                             std::shared_ptr<ParticleData> data,
                                             SceneGraphNode& parentNode) {
    static const U32 particleMask = to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                    to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                    to_const_uint(SGNComponent::ComponentType::RENDERING);
    DIVIDE_ASSERT(!name.empty(),
                  "Scene::addParticleEmitter error: invalid name specified!");

    ResourceDescriptor particleEmitter(name);
    std::shared_ptr<ParticleEmitter> emitter = CreateResource<ParticleEmitter>(particleEmitter);

    DIVIDE_ASSERT(emitter != nullptr,
                  "Scene::addParticleEmitter error: Could not instantiate emitter!");

    if (Application::instance().isMainThread()) {
        emitter->initData(data);
    } else {
        Application::instance().mainThreadTask([&emitter, &data] { emitter->initData(data); });
    }

    return parentNode.addNode(emitter, particleMask, PhysicsGroup::GROUP_IGNORE);
}


SceneGraphNode_ptr Scene::addLight(LightType type,
                                   SceneGraphNode& parentNode) {
    static const U32 lightMask = to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                 to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                 to_const_uint(SGNComponent::ComponentType::RENDERING);

    const char* lightType = "";
    switch (type) {
        case LightType::DIRECTIONAL:
            lightType = "_directional_light ";
            break;
        case LightType::POINT:
            lightType = "_point_light_";
            break;
        case LightType::SPOT:
            lightType = "_spot_light_";
            break;
    }

    ResourceDescriptor defaultLight(
        getName() +
        lightType +
        to_stringImpl(_lightPool->getLights(type).size()));

    defaultLight.setEnumValue(to_uint(type));
    defaultLight.setUserPtr(_lightPool);
    std::shared_ptr<Light> light = CreateResource<Light>(defaultLight);
    if (type == LightType::DIRECTIONAL) {
        light->setCastShadows(true);
    }
    return parentNode.addNode(light, lightMask, PhysicsGroup::GROUP_IGNORE);
}

void Scene::toggleFlashlight() {
    static const U32 lightMask = to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                 to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                 to_const_uint(SGNComponent::ComponentType::RENDERING);

    if (_flashLight.lock() == nullptr) {

        ResourceDescriptor tempLightDesc("MainFlashlight");
        tempLightDesc.setEnumValue(to_const_uint(LightType::SPOT));
        tempLightDesc.setUserPtr(_lightPool);
        std::shared_ptr<Light> tempLight = CreateResource<Light>(tempLightDesc);
        tempLight->setDrawImpostor(false);
        tempLight->setRange(30.0f);
        tempLight->setCastShadows(true);
        tempLight->setDiffuseColour(DefaultColours::WHITE());
        _flashLight = _sceneGraph->getRoot().addNode(tempLight, lightMask, PhysicsGroup::GROUP_IGNORE);
    }

    _flashLight.lock()->getNode<Light>()->setEnabled(!_flashLight.lock()->getNode<Light>()->getEnabled());
}

SceneGraphNode_ptr Scene::addSky(const stringImpl& nodeName) {
    ResourceDescriptor skyDescriptor("Default Sky");
    skyDescriptor.setID(to_uint(std::floor(Camera::activeCamera()->getZPlanes().y * 2)));

    std::shared_ptr<Sky> skyItem = CreateResource<Sky>(skyDescriptor);
    DIVIDE_ASSERT(skyItem != nullptr, "Scene::addSky error: Could not create sky resource!");

    static const U32 normalMask = 
        to_const_uint(SGNComponent::ComponentType::NAVIGATION) |
        to_const_uint(SGNComponent::ComponentType::PHYSICS) |
        to_const_uint(SGNComponent::ComponentType::BOUNDS) |
        to_const_uint(SGNComponent::ComponentType::RENDERING);

    SceneGraphNode_ptr skyNode = _sceneGraph->getRoot().addNode(skyItem,
                                                                normalMask,
                                                                PhysicsGroup::GROUP_IGNORE,
                                                                nodeName);
    skyNode->lockVisibility(true);

    return skyNode;
}

U16 Scene::registerInputActions() {
    _input->flushCache();

    auto none = [](InputParams param) {};
    auto deleteSelection = [this](InputParams param) { _sceneGraph->deleteNode(_currentSelection, false); };
    auto increaseCameraSpeed = [this](InputParams param){
        Camera& cam = *Camera::activeCamera();
        F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
        if (currentCamMoveSpeedFactor < 50) {
            cam.setMoveSpeedFactor(currentCamMoveSpeedFactor + 1.0f);
            cam.setTurnSpeedFactor(cam.getTurnSpeedFactor() + 1.0f);
        }
    };
    auto decreaseCameraSpeed = [this](InputParams param) {
        Camera& cam = *Camera::activeCamera();
        F32 currentCamMoveSpeedFactor = cam.getMoveSpeedFactor();
        if (currentCamMoveSpeedFactor > 1.0f) {
            cam.setMoveSpeedFactor(currentCamMoveSpeedFactor - 1.0f);
            cam.setTurnSpeedFactor(cam.getTurnSpeedFactor() - 1.0f);
        }
    };
    auto increaseResolution = [this](InputParams param) {_context._GFX.increaseResolution();};
    auto decreaseResolution = [this](InputParams param) {_context._GFX.decreaseResolution();};
    auto moveForward = [this](InputParams param) {state().moveFB(SceneState::MoveDirection::POSITIVE);};
    auto moveBackwards = [this](InputParams param) {state().moveFB(SceneState::MoveDirection::NEGATIVE);};
    auto stopMoveFWDBCK = [this](InputParams param) {state().moveFB(SceneState::MoveDirection::NONE);};
    auto strafeLeft = [this](InputParams param) {state().moveLR(SceneState::MoveDirection::NEGATIVE);};
    auto strafeRight = [this](InputParams param) {state().moveLR(SceneState::MoveDirection::POSITIVE);};
    auto stopStrafeLeftRight = [this](InputParams param) {state().moveLR(SceneState::MoveDirection::NONE);};
    auto rollCCW = [this](InputParams param) {state().roll(SceneState::MoveDirection::POSITIVE);};
    auto rollCW = [this](InputParams param) {state().roll(SceneState::MoveDirection::NEGATIVE);};
    auto stopRollCCWCW = [this](InputParams param) {state().roll(SceneState::MoveDirection::NONE);};
    auto turnLeft = [this](InputParams param) { state().angleLR(SceneState::MoveDirection::NEGATIVE);};
    auto turnRight = [this](InputParams param) { state().angleLR(SceneState::MoveDirection::POSITIVE);};
    auto stopTurnLeftRight = [this](InputParams param) { state().angleLR(SceneState::MoveDirection::NONE);};
    auto turnUp = [this](InputParams param) {state().angleUD(SceneState::MoveDirection::NEGATIVE);};
    auto turnDown = [this](InputParams param) {state().angleUD(SceneState::MoveDirection::POSITIVE);};
    auto stopTurnUpDown = [this](InputParams param) {state().angleUD(SceneState::MoveDirection::NONE);};
    auto togglePauseState = [](InputParams param){
        ParamHandler& par = ParamHandler::instance();
        par.setParam(_ID("freezeLoopTime"), !par.getParam(_ID("freezeLoopTime"), false));
    };
    auto toggleDepthOfField = [](InputParams param) {
        PostFX& postFX = PostFX::instance();
        if (postFX.getFilterState(FilterType::FILTER_DEPTH_OF_FIELD)) {
            postFX.popFilter(FilterType::FILTER_DEPTH_OF_FIELD);
        } else {
            postFX.pushFilter(FilterType::FILTER_DEPTH_OF_FIELD);
        }
    };
    auto toggleBloom = [](InputParams param) {
        PostFX& postFX = PostFX::instance();
        if (postFX.getFilterState(FilterType::FILTER_BLOOM)) {
            postFX.popFilter(FilterType::FILTER_BLOOM);
        } else {
            postFX.pushFilter(FilterType::FILTER_BLOOM);
        }
    };
    auto toggleSkeletonRendering = [this](InputParams param) {renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_SKELETONS);};
    auto toggleAxisLineRendering = [this](InputParams param) {renderState().toggleAxisLines();};
    auto toggleWireframeRendering = [this](InputParams param) {renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_WIREFRAME);};
    auto toggleGeometryRendering = [this](InputParams param) { renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY);};
    auto toggleDebugLines = [this](InputParams param) {renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_DEBUG_LINES);};
    auto toggleBoundingBoxRendering = [this](InputParams param) {renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_AABB);};
    auto toggleShadowMapDepthBufferPreview = [this](InputParams param) {
        ParamHandler& par = ParamHandler::instance();
        LightPool::togglePreviewShadowMaps(_context._GFX);
        par.setParam<bool>(
            _ID("rendering.previewDepthBuffer"),
            !par.getParam<bool>(_ID("rendering.previewDepthBuffer"), false));
    };
    auto takeScreenshot = [this](InputParams param) { _context._GFX.Screenshot("screenshot_"); };
    auto toggleFullScreen = [this](InputParams param) { _context._GFX.toggleFullScreen(); };
    auto toggleFlashLight = [this](InputParams param) {toggleFlashlight(); };
    auto toggleOctreeRegionRendering = [this](InputParams param) {renderState().toggleOption(SceneRenderState::RenderOptions::RENDER_OCTREE_REGIONS);};
    auto select = [this](InputParams  param) {findSelection(); };
    auto lockCameraToMouse = [this](InputParams  param) {state().cameraLockedToMouse(true); };
    auto releaseCameraFromMouse = [this](InputParams  param) {
        state().cameraLockedToMouse(false);
        state().angleLR(SceneState::MoveDirection::NONE);
        state().angleUD(SceneState::MoveDirection::NONE);
    };
    auto rendererDebugView = [](InputParams param) {SceneManager::instance().getRenderer().toggleDebugView();};
    auto shutdown = [](InputParams param) {Application::instance().RequestShutdown();};
    auto povNavigation = [this](InputParams param) {
        if (param._var[0] & OIS::Pov::North) {  // Going up
            state().moveFB(SceneState::MoveDirection::POSITIVE);
        }
        if (param._var[0] & OIS::Pov::South) {  // Going down
            state().moveFB(SceneState::MoveDirection::NEGATIVE);
        }
        if (param._var[0] & OIS::Pov::East) {  // Going right
            state().moveLR(SceneState::MoveDirection::POSITIVE);
        }
        if (param._var[0] & OIS::Pov::West) {  // Going left
            state().moveLR(SceneState::MoveDirection::NEGATIVE);
        }
        if (param._var[0] == OIS::Pov::Centered) {  // stopped/centered out
            state().moveLR(SceneState::MoveDirection::NONE);
            state().moveFB(SceneState::MoveDirection::NONE);
        }
    };

    auto axisNavigation = [this](InputParams param) {
        I32 axis = param._var[2];
        Input::Joystick joystick = static_cast<Input::Joystick>(param._var[3]);

        Input::JoystickInterface* joyInterface = _context._INPUT.getJoystickInterface();
        
        const Input::JoystickData& joyData = joyInterface->getJoystickData(joystick);
        I32 deadZone = joyData._deadZone;
        I32 axisABS = std::min(param._var[0], joyData._max);

        switch (axis) {
            case 0: {
                if (axisABS > deadZone) {
                    state().angleUD(SceneState::MoveDirection::POSITIVE);
                } else if (axisABS < -deadZone) {
                    state().angleUD(SceneState::MoveDirection::NEGATIVE);
                } else {
                    state().angleUD(SceneState::MoveDirection::NONE);
                }
            } break;
            case 1: {
                if (axisABS > deadZone) {
                    state().angleLR(SceneState::MoveDirection::POSITIVE);
                } else if (axisABS < -deadZone) {
                    state().angleLR(SceneState::MoveDirection::NEGATIVE);
                } else {
                    state().angleLR(SceneState::MoveDirection::NONE);
                }
            } break;

            case 2: {
                if (axisABS < -deadZone) {
                    state().moveFB(SceneState::MoveDirection::POSITIVE);
                } else if (axisABS > deadZone) {
                    state().moveFB(SceneState::MoveDirection::NEGATIVE);
                } else {
                    state().moveFB(SceneState::MoveDirection::NONE);
                }
            } break;
            case 3: {
                if (axisABS < -deadZone) {
                    state().moveLR(SceneState::MoveDirection::NEGATIVE);
                } else if (axisABS > deadZone) {
                    state().moveLR(SceneState::MoveDirection::POSITIVE);
                } else {
                    state().moveLR(SceneState::MoveDirection::NONE);
                }
            } break;
        }
    };

    U16 actionID = 0;
    InputActionList& actions = _input->actionList();
    actions.registerInputAction(actionID++, none);
    actions.registerInputAction(actionID++, deleteSelection);
    actions.registerInputAction(actionID++, increaseCameraSpeed);
    actions.registerInputAction(actionID++, decreaseCameraSpeed);
    actions.registerInputAction(actionID++, increaseResolution);
    actions.registerInputAction(actionID++, decreaseResolution);
    actions.registerInputAction(actionID++, moveForward);
    actions.registerInputAction(actionID++, moveBackwards);
    actions.registerInputAction(actionID++, stopMoveFWDBCK);
    actions.registerInputAction(actionID++, strafeLeft);
    actions.registerInputAction(actionID++, strafeRight);
    actions.registerInputAction(actionID++, stopStrafeLeftRight);
    actions.registerInputAction(actionID++, rollCCW);
    actions.registerInputAction(actionID++, rollCW);
    actions.registerInputAction(actionID++, stopRollCCWCW);
    actions.registerInputAction(actionID++, turnLeft);
    actions.registerInputAction(actionID++, turnRight);
    actions.registerInputAction(actionID++, stopTurnLeftRight);
    actions.registerInputAction(actionID++, turnUp);
    actions.registerInputAction(actionID++, turnDown);
    actions.registerInputAction(actionID++, stopTurnUpDown);
    actions.registerInputAction(actionID++, togglePauseState);
    actions.registerInputAction(actionID++, toggleDepthOfField);
    actions.registerInputAction(actionID++, toggleBloom);
    actions.registerInputAction(actionID++, toggleSkeletonRendering);
    actions.registerInputAction(actionID++, toggleAxisLineRendering);
    actions.registerInputAction(actionID++, toggleWireframeRendering);
    actions.registerInputAction(actionID++, toggleGeometryRendering);
    actions.registerInputAction(actionID++, toggleDebugLines);
    actions.registerInputAction(actionID++, toggleBoundingBoxRendering);
    actions.registerInputAction(actionID++, toggleShadowMapDepthBufferPreview);
    actions.registerInputAction(actionID++, takeScreenshot);
    actions.registerInputAction(actionID++, toggleFullScreen);
    actions.registerInputAction(actionID++, toggleFlashLight);
    actions.registerInputAction(actionID++, toggleOctreeRegionRendering);
    actions.registerInputAction(actionID++, select);
    actions.registerInputAction(actionID++, lockCameraToMouse);
    actions.registerInputAction(actionID++, releaseCameraFromMouse);
    actions.registerInputAction(actionID++, rendererDebugView);
    actions.registerInputAction(actionID++, shutdown);
    actions.registerInputAction(actionID++, povNavigation);
    actions.registerInputAction(actionID++, axisNavigation);

    return actionID;
}

void Scene::loadKeyBindings() {
    XML::loadDefaultKeybindings("keyBindings.xml", this);
}

bool Scene::load(const stringImpl& name) {
    setState(ResourceState::RES_LOADING);

    static const U32 normalMask = to_const_uint(SGNComponent::ComponentType::NAVIGATION) |
                                  to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                  to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                  to_const_uint(SGNComponent::ComponentType::RENDERING);

    STUBBED("ToDo: load skyboxes from XML")
    _name = name;

    loadXMLAssets();
    SceneGraphNode& root = _sceneGraph->getRoot();
    // Add terrain from XML
    if (!_terrainInfoArray.empty()) {
        for (TerrainDescriptor* terrainInfo : _terrainInfoArray) {
            ResourceDescriptor terrain(terrainInfo->getVariable("terrainName"));
            terrain.setPropertyDescriptor(*terrainInfo);
            std::shared_ptr<Terrain> temp = CreateResource<Terrain>(terrain);

            SceneGraphNode_ptr terrainTemp = root.addNode(temp, normalMask, PhysicsGroup::GROUP_STATIC);
            terrainTemp->setActive(terrainInfo->getActive());
            terrainTemp->usageContext(SceneGraphNode::UsageContext::NODE_STATIC);
            _terrainList.push_back(terrainTemp->getName());

            NavigationComponent* nComp = terrainTemp->get<NavigationComponent>();
            nComp->navigationContext(NavigationComponent::NavigationContext::NODE_OBSTACLE);
            MemoryManager::DELETE(terrainInfo);
        }
    }
    _terrainInfoArray.clear();

    // Camera position is overridden in the scene's XML configuration file
    if (ParamHandler::instance().getParam<bool>(_ID_RT((getName() + "options.cameraStartPositionOverride").c_str()))) {
        Camera::activeCamera()->setEye(vec3<F32>(
            _paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraStartPosition.x").c_str())),
            _paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraStartPosition.y").c_str())),
            _paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraStartPosition.z").c_str()))));
        vec2<F32> camOrientation(_paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraStartOrientation.xOffsetDegrees").c_str())),
                                 _paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraStartOrientation.yOffsetDegrees").c_str())));
        Camera::activeCamera()->setGlobalRotation(camOrientation.y /*yaw*/, camOrientation.x /*pitch*/);
    } else {
        Camera::activeCamera()->setEye(vec3<F32>(0, 50, 0));
    }

    Camera::activeCamera()->setMoveSpeedFactor(_paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraSpeed.move").c_str()), 1.0f));
    Camera::activeCamera()->setTurnSpeedFactor(_paramHandler.getParam<F32>(_ID_RT((getName() + ".options.cameraSpeed.turn").c_str()), 1.0f));

    addSelectionCallback(DELEGATE_BIND(&GUI::selectionChangeCallback, &_context.gui(), this));

    _loadComplete = true;
    return _loadComplete;
}

bool Scene::unload() {
    // prevent double unload calls
    if (!checkLoadFlag()) {
        return false;
    }
    clearTasks();
    _lightPool->clear();
    /// Destroy physics (:D)
    _pxScene->release();
    MemoryManager::DELETE(_pxScene);
    _context._PFX.setPhysicsScene(nullptr);
    clearObjects();
    _loadComplete = false;
    return true;
}

bool Scene::loadResources(bool continueOnErrors) {
    return true;
}

void Scene::postLoad() {
    _sceneGraph->postLoad();
    Console::printfn(Locale::get(_ID("CREATE_AI_ENTITIES_START")));
    initializeAI(true);
    Console::printfn(Locale::get(_ID("CREATE_AI_ENTITIES_END")));
}

void Scene::postLoadMainThread() {
    assert(Application::instance().isMainThread());
    setState(ResourceState::RES_LOADED);
}

void Scene::rebuildShaders() {
    SceneGraphNode_ptr selection(_currentSelection.lock());
    if (selection != nullptr) {
        selection->get<RenderingComponent>()->rebuildMaterial();
    } else {
        ShaderProgram::rebuildAllShaders();
    }
}

void Scene::onSetActive() {
    _context._PFX.setPhysicsScene(_pxScene);
    _aiManager->pauseUpdate(false);

    _context._SFX.stopMusic();
    _context._SFX.dumpPlaylists();

    for (U32 i = 0; i < to_const_uint(MusicType::COUNT); ++i) {
        const SceneState::MusicPlaylist& playlist = state().music(static_cast<MusicType>(i));
        if (!playlist.empty()) {
            for (const SceneState::MusicPlaylist::value_type& song : playlist) {
                _context._SFX.addMusic(i, song.second);
            }
        }
    }
    _context._SFX.playMusic(0);
}

void Scene::onRemoveActive() {
    _aiManager->pauseUpdate(true);
}

bool Scene::loadPhysics(bool continueOnErrors) {
    if (_pxScene == nullptr) {
        _pxScene = _context._PFX.NewSceneInterface(*this);
        _pxScene->init();
    }

    // Cook geometry
    if (_paramHandler.getParam<bool>(_ID("options.autoCookPhysicsAssets"), true)) {
        _cookCollisionMeshesScheduled = true;
    }
    return true;
}

bool Scene::initializeAI(bool continueOnErrors) {
    _aiTask = std::thread(DELEGATE_BIND(&AI::AIManager::update, _aiManager));
    setThreadName(&_aiTask, Util::StringFormat("AI_THREAD_SCENE_%s", getName().c_str()).c_str());
    return true;
}

 /// Shut down AIManager thread
bool Scene::deinitializeAI(bool continueOnErrors) { 
    _aiManager->stop();
    WAIT_FOR_CONDITION(!_aiManager->running());
    _aiTask.join();
        
    return true;
}

void Scene::clearObjects() {
    while (!_modelDataArray.empty()) {
        _modelDataArray.pop();
    }
    _vegetationDataArray.clear();

    _sceneGraph->unload();
}

bool Scene::updateCameraControls() {
    Camera& cam = *Camera::activeCamera();

    state().cameraUpdated(false);
    switch (cam.getType()) {
        default:
        case Camera::CameraType::FREE_FLY: {
            if (state().angleLR() != SceneState::MoveDirection::NONE) {
                cam.rotateYaw(to_float(state().angleLR()));
                state().cameraUpdated(true);
            }
            if (state().angleUD() != SceneState::MoveDirection::NONE) {
                cam.rotatePitch(to_float(state().angleUD()));
                state().cameraUpdated(true);
            }
            if (state().roll() != SceneState::MoveDirection::NONE) {
                cam.rotateRoll(to_float(state().roll()));
                state().cameraUpdated(true);
            }
            if (state().moveFB() != SceneState::MoveDirection::NONE) {
                cam.moveForward(to_float(state().moveFB()));
                state().cameraUpdated(true);
            }
            if (state().moveLR() != SceneState::MoveDirection::NONE) {
                cam.moveStrafe(to_float(state().moveLR()));
                state().cameraUpdated(true);
            }
        } break;
    }

    return state().cameraUpdated();
}

void Scene::updateSceneState(const U64 deltaTime) {
    _sceneTimer += deltaTime;
    updateSceneStateInternal(deltaTime);
    _sceneGraph->sceneUpdate(deltaTime, *_sceneState);
    findHoverTarget();
    if (checkCameraUnderwater()) {
        state().cameraUnderwater(true);
        PostFX::instance().pushFilter(FilterType::FILTER_UNDERWATER);
    } else {
        state().cameraUnderwater(false);
        PostFX::instance().popFilter(FilterType::FILTER_UNDERWATER);
    }
    SceneGraphNode_ptr flashLight = _flashLight.lock();

    if (flashLight) {
        const Camera& cam = *Camera::activeCamera();
        flashLight->get<PhysicsComponent>()->setPosition(cam.getEye());
        flashLight->get<PhysicsComponent>()->setRotation(cam.getEuler());
    }
}

void Scene::onLostFocus() {
    state().resetMovement();
    if (!Config::Build::IS_DEBUG_BUILD) {
        //_paramHandler.setParam(_ID("freezeLoopTime"), true);
    }
}

void Scene::registerTask(const TaskHandle& taskItem) { 
    _tasks.push_back(taskItem);
}

void Scene::clearTasks() {
    Console::printfn(Locale::get(_ID("STOP_SCENE_TASKS")));
    // Performance shouldn't be an issue here
    for (TaskHandle& task : _tasks) {
        if (task._task->jobIdentifier() == task._jobIdentifier) {
            task._task->stopTask();
            task.wait();
        }
    }

    _tasks.clear();
}

void Scene::removeTask(I64 jobIdentifier) {
    vectorImpl<TaskHandle>::iterator it;
    for (it = std::begin(_tasks); it != std::end(_tasks); ++it) {
        if ((*it)._task->jobIdentifier() == jobIdentifier) {
            (*it)._task->stopTask();
            _tasks.erase(it);
            (*it).wait();
            return;
        }
    }

}

void Scene::processInput(const U64 deltaTime) {
}

void Scene::processGUI(const U64 deltaTime) {
    for (U16 i = 0; i < _guiTimers.size(); ++i) {
        _guiTimers[i] += Time::MicrosecondsToMilliseconds<D64>(deltaTime);
    }
}

void Scene::processTasks(const U64 deltaTime) {
    for (U16 i = 0; i < _taskTimers.size(); ++i) {
        _taskTimers[i] += Time::MicrosecondsToMilliseconds<D64>(deltaTime);
    }
}

void Scene::debugDraw(const Camera& activeCamera, RenderStage stage, RenderSubPassCmds& subPassesInOut) {
    if (stage != RenderStage::DISPLAY || _context._GFX.isPrePass()) {
        return;
    }

    RenderSubPassCmd& subPass = subPassesInOut.back();
    if (Config::Build::IS_DEBUG_BUILD) {
        const SceneRenderState::GizmoState& currentGizmoState = renderState().gizmoState();

        if (currentGizmoState == SceneRenderState::GizmoState::SELECTED_GIZMO) {
            SceneGraphNode_ptr selection(_currentSelection.lock());
            if (selection != nullptr) {
                selection->get<RenderingComponent>()->drawDebugAxis();
            }
        }

        if (renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_OCTREE_REGIONS)) {
            for (IMPrimitive* prim : _octreePrimitives) {
                prim->paused(true);
            }

            _octreeBoundingBoxes.resize(0);
            sceneGraph().getOctree().getAllRegions(_octreeBoundingBoxes);

            size_t primitiveCount = _octreePrimitives.size();
            size_t regionCount = _octreeBoundingBoxes.size();
            if (regionCount > primitiveCount) {
                size_t diff = regionCount - primitiveCount;
                for (size_t i = 0; i < diff; ++i) {
                    _octreePrimitives.push_back(_context._GFX.newIMP());
                }
            }

            assert(_octreePrimitives.size() >= _octreeBoundingBoxes.size());

            for (size_t i = 0; i < regionCount; ++i) {
                const BoundingBox& box = _octreeBoundingBoxes[i];
                _octreePrimitives[i]->fromBox(box.getMin(), box.getMax(), vec4<U8>(255, 0, 255, 255));
                subPass._commands.push_back(_octreePrimitives[i]->toDrawCommand());
            }
        }
    }
    if (Config::Build::IS_DEBUG_BUILD) {
        subPass._commands.push_back(_linesPrimitive->toDrawCommand());
    }
    // Show NavMeshes
    _aiManager->debugDraw(subPassesInOut, false);
    _lightPool->drawLightImpostors(subPassesInOut);
    _envProbePool->debugDraw(subPassesInOut);
}

bool Scene::checkCameraUnderwater() const {
    const Camera& crtCamera = *Camera::activeCamera();
    const vec3<F32>& eyePos = crtCamera.getEye();

    RenderPassCuller::VisibleNodeList& nodes = SceneManager::instance().getVisibleNodesCache(RenderStage::DISPLAY);
    for (RenderPassCuller::VisibleNode& node : nodes) {
        SceneGraphNode_cptr nodePtr = node.second.lock();
        if (nodePtr) {
            const std::shared_ptr<SceneNode>& sceneNode = nodePtr->getNode();
            if (sceneNode->getType() == SceneNodeType::TYPE_WATER) {
                if (nodePtr->getNode<WaterPlane>()->pointUnderwater(*nodePtr, eyePos)) {
                    return true;
                }
            }
        }
    }

    return false;
}

void Scene::findHoverTarget() {
    const Camera& crtCamera = *Camera::activeCamera();
    const vec2<U16>& displaySize = Application::instance().windowManager().getActiveWindow().getDimensions();
    const vec2<F32>& zPlanes = crtCamera.getZPlanes();
    const vec2<I32>& mousePos = _input->getMousePosition();

    F32 mouseX = to_float(mousePos.x);
    F32 mouseY = displaySize.height - to_float(mousePos.y) - 1;

    const vec4<I32>& viewport = _context._GFX.getCurrentViewport();
    vec3<F32> startRay = crtCamera.unProject(mouseX, mouseY, 0.0f, viewport);
    vec3<F32> endRay = crtCamera.unProject(mouseX, mouseY, 1.0f, viewport);
    // see if we select another one
    _sceneSelectionCandidates.clear();
    // get the list of visible nodes (use Z_PRE_PASS because the nodes are sorted by depth, front to back)
    RenderPassCuller::VisibleNodeList& nodes = SceneManager::instance().getVisibleNodesCache(RenderStage::DISPLAY);

    // Cast the picking ray and find items between the nearPlane and far Plane
    Ray mouseRay(startRay, startRay.direction(endRay));
    for (RenderPassCuller::VisibleNode& node : nodes) {
        SceneGraphNode_cptr nodePtr = node.second.lock();
        if (nodePtr) {
            nodePtr->intersect(mouseRay, zPlanes.x, zPlanes.y, _sceneSelectionCandidates, false);                   
        }
    }

    if (!_sceneSelectionCandidates.empty()) {
        _currentHoverTarget = _sceneSelectionCandidates.front();
        std::shared_ptr<SceneNode> node = _currentHoverTarget.lock()->getNode();
        if (node->getType() == SceneNodeType::TYPE_OBJECT3D) {
            if (static_cast<Object3D*>(node.get())->getObjectType() == Object3D::ObjectType::SUBMESH) {
                _currentHoverTarget = _currentHoverTarget.lock()->getParent();
            }
        }

        SceneGraphNode_ptr target = _currentHoverTarget.lock();
        if (target->getSelectionFlag() != SceneGraphNode::SelectionFlag::SELECTION_SELECTED) {
            target->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_HOVER);
        }
    } else {
        SceneGraphNode_ptr target(_currentHoverTarget.lock());
        if (target) {
            if (target->getSelectionFlag() != SceneGraphNode::SelectionFlag::SELECTION_SELECTED) {
                target->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_NONE);
            }
            _currentHoverTarget.reset();
        }

    }

}

void Scene::resetSelection() {
    if (!_currentSelection.expired()) {
        _currentSelection.lock()->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_NONE);
    }

    _currentSelection.reset();
}

void Scene::findSelection() {
    bool hadTarget = !_currentSelection.expired();
    bool haveTarget = !_currentHoverTarget.expired();

    I64 crtGUID = hadTarget ? _currentSelection.lock()->getGUID() : -1;
    I64 GUID = haveTarget ? _currentHoverTarget.lock()->getGUID() : -1;

    if (crtGUID != GUID) {
        resetSelection();

        if (haveTarget) {
            _currentSelection = _currentHoverTarget;
            _currentSelection.lock()->setSelectionFlag(SceneGraphNode::SelectionFlag::SELECTION_SELECTED);
        }

        for (DELEGATE_CBK<>& cbk : _selectionChangeCallbacks) {
            cbk();
        }
    }
}

bool Scene::save(ByteBuffer& outputBuffer) const {
    const Camera& cam = *Camera::activeCamera();
    outputBuffer << cam.getEye() << cam.getEuler();
    return true;
}

bool Scene::load(ByteBuffer& inputBuffer) {
    if (!inputBuffer.empty()) {
        vec3<F32> camPos;
        vec3<F32> camEuler;

        inputBuffer >> camPos >> camEuler;

        Camera& cam = *Camera::activeCamera();
        cam.setEye(camPos);
        cam.setGlobalRotation(-camEuler);
    }

    return true;
}

};