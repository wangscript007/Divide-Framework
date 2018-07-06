#include "Headers/WarScene.h"
#include "Headers/WarSceneAISceneImpl.h"

#include "GUI/Headers/GUIMessageBox.h"
#include "Geometry/Material/Headers/Material.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/Camera/Headers/ThirdPersonCamera.h"
#include "Rendering/Camera/Headers/FirstPersonCamera.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"
#include "Managers/Headers/CameraManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/AIManager.h"

#include "Dynamics/Entities/Particles/ConcreteUpdaters/Headers/ParticleBasicTimeUpdater.h"
#include "Dynamics/Entities/Particles/ConcreteUpdaters/Headers/ParticleBasicColorUpdater.h"
#include "Dynamics/Entities/Particles/ConcreteUpdaters/Headers/ParticleEulerUpdater.h"
#include "Dynamics/Entities/Particles/ConcreteUpdaters/Headers/ParticleFloorUpdater.h"

#include "Dynamics/Entities/Particles/ConcreteGenerators/Headers/ParticleBoxGenerator.h"
#include "Dynamics/Entities/Particles/ConcreteGenerators/Headers/ParticleColorGenerator.h"
#include "Dynamics/Entities/Particles/ConcreteGenerators/Headers/ParticleVelocityGenerator.h"
#include "Dynamics/Entities/Particles/ConcreteGenerators/Headers/ParticleTimeGenerator.h"

namespace Divide {

REGISTER_SCENE(WarScene);

namespace {
    static vec2<F32> g_sunAngle(0.0f, Angle::DegreesToRadians(45.0f));
    static bool g_direction = false;
};

WarScene::WarScene()
    : Scene(),
      _sun(nullptr),
      _infoBox(nullptr),
      _sceneReady(false),
      _lastNavMeshBuildTime(0UL)
{
    for (U8 i = 0; i < 2; ++i) {
        _flag[i] = nullptr;
        _faction[i] = nullptr;
    }
}

WarScene::~WarScene()
{
}

void WarScene::processGUI(const U64 deltaTime) {
    D32 FpsDisplay = Time::SecondsToMilliseconds(0.3);
    if (_guiTimers[0] >= FpsDisplay) {
        const Camera& cam = renderState().getCamera();
        const vec3<F32>& eyePos = cam.getEye();
        const vec3<F32>& euler = cam.getEuler();
        _GUI->modifyText("fpsDisplay", "FPS: %3.0f. FrameTime: %3.1f",
                         Time::ApplicationTimer::getInstance().getFps(),
                         Time::ApplicationTimer::getInstance().getFrameTime());
        _GUI->modifyText("RenderBinCount", "Number of items in Render Bin: %d",
                         GFX_RENDER_BIN_SIZE);
        _GUI->modifyText("camPosition",
                         "Position [ X: %5.2f | Y: %5.2f | Z: %5.2f ] [Pitch: "
                         "%5.2f | Yaw: %5.2f]",
                         eyePos.x, eyePos.y, eyePos.z, euler.pitch, euler.yaw);
        _guiTimers[0] = 0.0;
    }
    Scene::processGUI(deltaTime);
}

void WarScene::processTasks(const U64 deltaTime) {
    if (!_sceneReady) {
        return;
    }

    D32 SunTimer = Time::Milliseconds(10);
    D32 AnimationTimer1 = Time::SecondsToMilliseconds(5);
    D32 AnimationTimer2 = Time::SecondsToMilliseconds(10);

    if (_taskTimers[0] >= SunTimer) {
        if (!g_direction) {
            g_sunAngle.y += 0.005f;
            g_sunAngle.x += 0.005f;
        } else {
            g_sunAngle.y -= 0.005f;
            g_sunAngle.x -= 0.005f;
        }

        if (!IS_IN_RANGE_INCLUSIVE(g_sunAngle.y, 
                                   Angle::DegreesToRadians(25.0f),
                                   Angle::DegreesToRadians(55.0f))) {
            g_direction = !g_direction;
        }

        vec3<F32> sunVector(-cosf(g_sunAngle.x) * sinf(g_sunAngle.y),
                            -cosf(g_sunAngle.y),
                            -sinf(g_sunAngle.x) * sinf(g_sunAngle.y));

        _sun->setDirection(sunVector);
        _currentSky->getNode<Sky>()->setSunProperties(sunVector,
            _sun->getDiffuseColor());

        _taskTimers[0] = 0.0;
    }

    if (_taskTimers[1] >= AnimationTimer1) {
        for (NPC* const npc : _armyNPCs[0]) {
            npc->playNextAnimation();
        }
        _taskTimers[1] = 0.0;
    }

    if (_taskTimers[2] >= AnimationTimer2) {
        for (NPC* const npc : _armyNPCs[1]) {
            npc->playNextAnimation();
        }
        _taskTimers[2] = 0.0;
    }

    Scene::processTasks(deltaTime);
}

void WarScene::updateSceneStateInternal(const U64 deltaTime) {
    if (!_sceneReady) {
        return;
    }
    static U64 totalTime = 0;

    totalTime += deltaTime;

#ifdef _DEBUG
    if (!AI::AIManager::getInstance().getNavMesh(
            _army[0][0]->getAgentRadiusCategory())) {
        return;
    }

    _lines[to_uint(DebugLines::DEBUG_LINE_OBJECT_TO_TARGET)].resize(_army[0].size() +
                                               _army[1].size());
    // renderState().drawDebugLines(true);
    U32 count = 0;
    for (U8 i = 0; i < 2; ++i) {
        for (AI::AIEntity* const character : _army[i]) {
            _lines[to_uint(DebugLines::DEBUG_LINE_OBJECT_TO_TARGET)][count]._startPoint.set(
                character->getPosition());
            _lines[to_uint(DebugLines::DEBUG_LINE_OBJECT_TO_TARGET)][count]._endPoint.set(
                character->getDestination());
            _lines[to_uint(DebugLines::DEBUG_LINE_OBJECT_TO_TARGET)][count]._color.set(
                i == 1 ? 255 : 0, 0, i == 1 ? 0 : 255, 255);
            count++;
        }
    }
#endif
}

bool WarScene::load(const stringImpl& name, GUI* const gui) {
    // Load scene resources
    bool loadState = SCENE_LOAD(name, gui, true, true);
    // Add a light
    _sun = addLight(LightType::DIRECTIONAL,
               GET_ACTIVE_SCENEGRAPH().getRoot()).getNode<DirectionalLight>();
    // Add a skybox
    _currentSky =
        &addSky(CreateResource<Sky>(ResourceDescriptor("Default Sky")));
    // Position camera
    renderState().getCamera().setEye(vec3<F32>(54.5f, 25.5f, 1.5f));
    renderState().getCamera().setGlobalRotation(-90 /*yaw*/, 35 /*pitch*/);
    _sun->csmSplitCount(3);  // 3 splits
    _sun->csmSplitLogFactor(0.925f);
    _sun->csmNearClipOffset(25.0f);
    // Add some obstacles
    SceneGraphNode* cylinder[5];
    cylinder[0] = _sceneGraph.findNode("cylinderC");
    cylinder[1] = _sceneGraph.findNode("cylinderNW");
    cylinder[2] = _sceneGraph.findNode("cylinderNE");
    cylinder[3] = _sceneGraph.findNode("cylinderSW");
    cylinder[4] = _sceneGraph.findNode("cylinderSE");

    for (U8 i = 0; i < 5; ++i) {
        RenderingComponent* const renderable =
            std::begin(cylinder[i]->getChildren())
                ->second->getComponent<RenderingComponent>();
        renderable->getMaterialInstance()->setDoubleSided(true);
        std::begin(cylinder[i]->getChildren())
            ->second->getNode()
            ->getMaterialTpl()
            ->setDoubleSided(true);
    }

    SceneNode* cylinderMeshNW = cylinder[1]->getNode();
    SceneNode* cylinderMeshNE = cylinder[2]->getNode();
    SceneNode* cylinderMeshSW = cylinder[3]->getNode();
    SceneNode* cylinderMeshSE = cylinder[4]->getNode();

    stringImpl currentName;
    SceneNode* currentMesh = nullptr;
    SceneGraphNode* baseNode = nullptr;
    
    std::pair<I32, I32> currentPos;
    for (U8 i = 0; i < 40; ++i) {
        if (i < 10) {
            baseNode = cylinder[1];
            currentMesh = cylinderMeshNW;
            currentName = "Cylinder_NW_" + std::to_string((I32)i);
            currentPos.first = -200 + 40 * i + 50;
            currentPos.second = -200 + 40 * i + 50;
        } else if (i >= 10 && i < 20) {
            baseNode = cylinder[2];
            currentMesh = cylinderMeshNE;
            currentName = "Cylinder_NE_" + std::to_string((I32)i);
            currentPos.first = 200 - 40 * (i % 10) - 50;
            currentPos.second = -200 + 40 * (i % 10) + 50;
        } else if (i >= 20 && i < 30) {
            baseNode = cylinder[3];
            currentMesh = cylinderMeshSW;
            currentName = "Cylinder_SW_" + std::to_string((I32)i);
            currentPos.first = -200 + 40 * (i % 20) + 50;
            currentPos.second = 200 - 40 * (i % 20) - 50;
        } else {
            baseNode = cylinder[4];
            currentMesh = cylinderMeshSE;
            currentName = "Cylinder_SE_" + std::to_string((I32)i);
            currentPos.first = 200 - 40 * (i % 30) - 50;
            currentPos.second = 200 - 40 * (i % 30) - 50;
        }

        SceneGraphNode& crtNode = _sceneGraph.getRoot().addNode(*currentMesh,
                                                                currentName);
        crtNode.setSelectable(true);
        crtNode.usageContext(baseNode->usageContext());
        PhysicsComponent* pComp = crtNode.getComponent<PhysicsComponent>();
        NavigationComponent* nComp =
            crtNode.getComponent<NavigationComponent>();
        pComp->physicsGroup(
            baseNode->getComponent<PhysicsComponent>()->physicsGroup());
        nComp->navigationContext(
            baseNode->getComponent<NavigationComponent>()->navigationContext());
        nComp->navigationDetailOverride(
            baseNode->getComponent<NavigationComponent>()
                ->navMeshDetailOverride());

        pComp->setScale(baseNode->getComponent<PhysicsComponent>()->getScale());
        pComp->setPosition(
            vec3<F32>(currentPos.first, -0.01f, currentPos.second));
    }
    SceneGraphNode* baseFlagNode = cylinder[1];
    _flag[0] = &_sceneGraph.getRoot().addNode(*cylinderMeshNW, "Team1Flag");
    _flag[0]->setSelectable(false);
    _flag[0]->usageContext(baseFlagNode->usageContext());
    PhysicsComponent* flagPComp = _flag[0]->getComponent<PhysicsComponent>();
    NavigationComponent* flagNComp =
        _flag[0]->getComponent<NavigationComponent>();
    flagPComp->physicsGroup(
        baseFlagNode->getComponent<PhysicsComponent>()->physicsGroup());
    flagNComp->navigationContext(NavigationComponent::NavigationContext::NODE_IGNORE);
    flagPComp->setScale(
        baseFlagNode->getComponent<PhysicsComponent>()->getScale() *
        vec3<F32>(0.05f, 1.1f, 0.05f));
    flagPComp->setPosition(vec3<F32>(25.0f, 0.1f, -206.0f));

    _flag[1] = &_sceneGraph.getRoot().addNode(*cylinderMeshNW, "Team2Flag");
    _flag[1]->setSelectable(false);
    _flag[1]->usageContext(baseFlagNode->usageContext());

    flagPComp = _flag[1]->getComponent<PhysicsComponent>();
    flagNComp = _flag[1]->getComponent<NavigationComponent>();

    flagPComp->physicsGroup(
        baseFlagNode->getComponent<PhysicsComponent>()->physicsGroup());
    flagNComp->navigationContext(NavigationComponent::NavigationContext::NODE_IGNORE);
    flagPComp->setScale(
        baseFlagNode->getComponent<PhysicsComponent>()->getScale() *
        vec3<F32>(0.05f, 1.1f, 0.05f));
    flagPComp->setPosition(vec3<F32>(25.0f, 0.1f, 206.0f));

    AI::WarSceneAISceneImpl::registerFlags(*_flag[0], *_flag[1]);

#ifdef _DEBUG
    const U32 particleCount = 200;
#else
    const U32 particleCount = 20000;
#endif

    const F32 emitRate = particleCount / 4;

    /*ParticleData particles(
        particleCount,
        to_uint(ParticleData::Properties::PROPERTIES_POS) |
        to_uint(ParticleData::Properties::PROPERTIES_VEL) |
        to_uint(ParticleData::Properties::PROPERTIES_ACC) | 
        to_uint(ParticleData::Properties::PROPERTIES_COLOR) |
        to_uint(ParticleData::Properties::PROPERTIES_COLOR_TRANS));

    std::shared_ptr<ParticleSource> particleSource =
        std::make_shared<ParticleSource>(emitRate);
    std::shared_ptr<ParticleBoxGenerator> boxGenerator =
        std::make_shared<ParticleBoxGenerator>();
    particleSource->addGenerator(boxGenerator);
    std::shared_ptr<ParticleColorGenerator> colGenerator =
        std::make_shared<ParticleColorGenerator>();
    particleSource->addGenerator(colGenerator);
    std::shared_ptr<ParticleVelocityGenerator> velGenerator =
        std::make_shared<ParticleVelocityGenerator>();
    particleSource->addGenerator(velGenerator);
    std::shared_ptr<ParticleTimeGenerator> timeGenerator =
        std::make_shared<ParticleTimeGenerator>();
    particleSource->addGenerator(timeGenerator);

    SceneGraphNode& testSGN =
        addParticleEmitter("TESTPARTICLES", particles, _sceneGraph.getRoot());
    ParticleEmitter* test = testSGN.getNode<ParticleEmitter>();
    testSGN.getComponent<PhysicsComponent>()->translateY(5);
    test->setDrawImpostor(true);
    test->enableEmitter(true);
    test->addSource(particleSource);

    std::shared_ptr<ParticleEulerUpdater> eulerUpdater =
        std::make_shared<ParticleEulerUpdater>();
    eulerUpdater->_globalAcceleration.set(0.0f, -15.0f, 0.0f, 0.0f);
    test->addUpdater(eulerUpdater);
    test->addUpdater(std::make_shared<ParticleBasicTimeUpdater>());
    test->addUpdater(std::make_shared<ParticleBasicColorUpdater>());
    test->addUpdater(std::make_shared<ParticleFloorUpdater>());*/

    state().generalVisibility(state().generalVisibility() * 2);

    Application::getInstance()
        .getKernel()
        .getCameraMgr()
        .getActiveCamera()
        ->setHorizontalFoV(135);

    SceneInput::PressReleaseActions cbks;
    cbks.second = DELEGATE_BIND(&WarScene::toggleCamera, this);
    _input->addKeyMapping(Input::KeyCode::KC_TAB, cbks);

    _sceneReady = true;
    return loadState;
}

void WarScene::toggleCamera() {
    static bool fpsCameraActive = false;
    static bool tpsCameraActive = false;
    static bool flyCameraActive = true;

    if (_currentSelection != nullptr) {
        if (flyCameraActive) {
            if (fpsCameraActive) {
                renderState().getCameraMgr().popActiveCamera();
            }
            renderState().getCameraMgr().pushActiveCamera("tpsCamera");
            static_cast<ThirdPersonCamera&>(renderState().getCamera())
                .setTarget(*_currentSelection);
            flyCameraActive = false;
            tpsCameraActive = true;
            return;
        }
    }
    if (tpsCameraActive) {
        renderState().getCameraMgr().pushActiveCamera("defaultCamera");
        tpsCameraActive = false;
        flyCameraActive = true;
    }
}

bool WarScene::loadResources(bool continueOnErrors) {
    _GUI->addButton("Simulate", "Simulate",
                    vec2<I32>(renderState().cachedResolution().width - 220,
                              renderState().cachedResolution().height / 1.1f),
                    vec2<U32>(100, 25), vec3<F32>(0.65f),
                    DELEGATE_BIND(&WarScene::startSimulation, this));

    _GUI->addText("fpsDisplay",  // Unique ID
                  vec2<I32>(60, 60),  // Position
                  Font::DIVIDE_DEFAULT,  // Font
                  vec3<F32>(0.0f, 0.2f, 1.0f),  // Color
                  "FPS: %s", 0);  // Text and arguments
    _GUI->addText("RenderBinCount", vec2<I32>(60, 70), Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.6f, 0.2f, 0.2f),
                  "Number of items in Render Bin: %d", 0);

    _GUI->addText("camPosition", vec2<I32>(60, 100), Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.2f, 0.8f, 0.2f),
                  "Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ] [Pitch: %5.2f | "
                  "Yaw: %5.2f]",
                  renderState().getCamera().getEye().x,
                  renderState().getCamera().getEye().y,
                  renderState().getCamera().getEye().z,
                  renderState().getCamera().getEuler().pitch,
                  renderState().getCamera().getEuler().yaw);

    _GUI->addText("lampPosition", vec2<I32>(60, 120), Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.2f, 0.8f, 0.2f),
                  "Lamp Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ]", 0.0f,
                  0.0f, 0.0f);
    _infoBox = _GUI->addMsgBox("infoBox", "Info", "Blabla");
    // Add a first person camera
    Camera* cam = MemoryManager_NEW FirstPersonCamera();
    cam->fromCamera(renderState().getCameraConst());
    cam->setMoveSpeedFactor(10.0f);
    cam->setTurnSpeedFactor(10.0f);
    renderState().getCameraMgr().addNewCamera("fpsCamera", cam);
    // Add a third person camera
    cam = MemoryManager_NEW ThirdPersonCamera();
    cam->fromCamera(renderState().getCameraConst());
    cam->setMoveSpeedFactor(0.02f);
    cam->setTurnSpeedFactor(0.01f);
    renderState().getCameraMgr().addNewCamera("tpsCamera", cam);

    _guiTimers.push_back(0.0);  // Fps
    _taskTimers.push_back(0.0); // Sun animation
    _taskTimers.push_back(0.0); // animation team 1
    _taskTimers.push_back(0.0); // animation team 2
    return true;
}

};