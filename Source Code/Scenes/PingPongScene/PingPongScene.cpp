#include "Headers/PingPongScene.h"

#include "Rendering/Headers/Frustum.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

REGISTER_SCENE(PingPongScene);

//begin copy-paste
void PingPongScene::preRender(){
    vec2<F32> _sunAngle = vec2<F32>(0.0f, RADIANS(45.0f));
    _sunvector = vec3<F32>(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
                            -cosf(_sunAngle.y),
                            -sinf(_sunAngle.x) * sinf(_sunAngle.y));

    LightManager::getInstance().getLight(0)->setDirection(_sunvector);
    getSkySGN(0)->getNode<Sky>()->setSunVector(_sunvector);
}
//<<end copy-paste

void PingPongScene::processGUI(const U64 deltaTime){
    D32 FpsDisplay = getSecToMs(0.3);
    if (_guiTimers[0] >= FpsDisplay){
        _GUI->modifyText("fpsDisplay", "FPS: %3.0f. FrameTime: %3.1f", ApplicationTimer::getInstance().getFps(), ApplicationTimer::getInstance().getFrameTime());
        _guiTimers[0] = 0.0;
    }
    Scene::processGUI(deltaTime);
}

void PingPongScene::processTasks(const U64 deltaTime){
    static vec2<F32> _sunAngle = vec2<F32>(0.0f, RADIANS(45.0f));
    static bool direction = false;
    if (!direction){
        _sunAngle.y += 0.005f;
        _sunAngle.x += 0.005f;
    }
    else{
        _sunAngle.y -= 0.005f;
        _sunAngle.x -= 0.005f;
    }

    if (_sunAngle.y <= RADIANS(25) || _sunAngle.y >= RADIANS(70))
        direction = !direction;

    _sunvector = vec3<F32>(-cosf(_sunAngle.x) * sinf(_sunAngle.y),
        -cosf(_sunAngle.y),
        -sinf(_sunAngle.x) * sinf(_sunAngle.y));
    /*
    LightManager::getInstance().getLight(0)->setDirection(_sunvector);
    getSkySGN(0)->getNode<Sky>()->setSunVector(_sunvector);
*/

    Scene::processTasks(deltaTime);
}

void PingPongScene::resetGame(){
    _directionTowardsAdversary = true;
    _upwardsDirection = false;
    _touchedAdversaryTableHalf = false;
    _touchedOwnTableHalf = false;
    _lost = false;
    _sideDrift = 0;
    clearTasks();
    _ballSGN->getTransform()->setPosition(vec3<F32>(0, 2 ,2));
}

void PingPongScene::serveBall(){
    _GUI->modifyText("insults","");
    resetGame();

    if(getTasks().empty()){//A maximum of 1 Tasks allowed
        Kernel* kernel = Application::getInstance().getKernel();
        Task_ptr newGame(New Task(kernel->getThreadPool(),30,true,false,DELEGATE_BIND(&PingPongScene::test,this,rand() % 5,TYPE_INTEGER)));
        addTask(newGame);
    }
}

void PingPongScene::test(boost::any a, CallbackParam b){
    if(getTasks().empty()) return;
    bool updated = false;
    std::string message;
    Transform* ballTransform = _ballSGN->getTransform();
    vec3<F32> ballPosition  = ballTransform->getPosition();

    SceneGraphNode* table = _sceneGraph->findNode("table");
    SceneGraphNode* net = _sceneGraph->findNode("net");
    SceneGraphNode* opponent = _sceneGraph->findNode("opponent");
    SceneGraphNode* paddle = _sceneGraph->findNode("paddle");
    vec3<F32> paddlePosition   = paddle->getTransform()->getPosition();
    vec3<F32> opponentPosition = opponent->getTransform()->getPosition();
    vec3<F32> tablePosition     = table->getTransform()->getPosition();

    //Is the ball coming towards us or towards the opponent?
    _directionTowardsAdversary ? ballPosition.z -= 0.11f : ballPosition.z += 0.11f;
    //Up or down?
    _upwardsDirection ? 	ballPosition.y += 0.084f : 	ballPosition.y -= 0.084f;

    //Is the ball moving to the right or to the left?
    ballPosition.x += _sideDrift*0.15f;
    if(opponentPosition.x != ballPosition.x)
        opponent->getTransform()->translateX(ballPosition.x - opponentPosition.x);

    ballTransform->translate(ballPosition - ballTransform->getPosition());

    //Did we hit the table? Bounce then ...
    if(table->getBoundingBox().Collision(_ballSGN->getBoundingBox())){
        if(ballPosition.z > tablePosition.z){
            _touchedOwnTableHalf = true;
            _touchedAdversaryTableHalf = false;
        }else{
            _touchedOwnTableHalf = false;
            _touchedAdversaryTableHalf = true;
        }
        _upwardsDirection = true;
    }
    //Kinetic  energy depletion
    if(ballPosition.y > 2.1f) _upwardsDirection = false;

    //Did we hit the paddle?
    if(_ballSGN->getBoundingBox().Collision(paddle->getBoundingBox())){
        _sideDrift = ballPosition.x - paddlePosition.x;
        //If we hit the ball with the upper margin of the paddle, add a slight impuls to the ball
        if(ballPosition.y >= paddlePosition.y) ballPosition.z -= 0.12f;

        _directionTowardsAdversary = true;
    }

    if(ballPosition.y + 0.75f < table->getBoundingBox().getMax().y){
        //If we hit the ball and it landed on the opponent's table half
        //Or if the opponent hit the ball and it landed on our table half
        if((_touchedAdversaryTableHalf && _directionTowardsAdversary) ||
           (!_directionTowardsAdversary && !_touchedOwnTableHalf))
            _lost = false;
        else
            _lost = true;

        updated = true;
    }
    //Did we win or lose?
    if(ballPosition.z >= paddlePosition.z){
        _lost = true;
        updated = true;
    }
    if(ballPosition.z <= opponentPosition.z){
        _lost = false;
        updated = true;
    }

    if(_ballSGN->getBoundingBox().Collision(net->getBoundingBox())){
        if(_directionTowardsAdversary){
            //Did we hit the net?
            _lost = true;
        }else{
            //Did the opponent hit the net?
            _lost = false;
        }
        updated = true;
    }

    //Did we hit the opponent? Then change ball direction ... BUT ...
    //Add a small chance that we win
    if(random(30) != 2)
    if(_ballSGN->getBoundingBox().Collision(opponent->getBoundingBox())){
        _sideDrift = ballPosition.x - opponent->getTransform()->getPosition().x;
        _directionTowardsAdversary = false;
    }
    //Add a spin effect to the ball
    ballTransform->rotateEuler(vec3<F32>(ballPosition.z,1,1));

    if(updated){
        if(_lost){
            message = "You lost!";
            _score--;

            if(b == TYPE_INTEGER){
                I32 quote = boost::any_cast<I32>(a);
                if(_score % 3 == 0 ) _GUI->modifyText("insults",(char*)_quotes[quote].c_str());
            }
        }else{
            message = "You won!";
            _score++;
        }

        _GUI->modifyText("Score","Score: %d",_score);
        _GUI->modifyText("Message",(char*)message.c_str());
        resetGame();
    }
}

void PingPongScene::processInput(const U64 deltaTime){
    
    if (_freeFly){
        defaultCameraControls();
        _wasInFreeFly = true;
        return;
    }

    Camera& cam = renderState().getCamera();
    if (_wasInFreeFly){
        //Position the camera
        cam.setPitch(-90);
        cam.setYaw(0);
        cam.setRoll(0);
        cam.setEye(vec3<F32>(0, 2.5f, 6.5f));
        _wasInFreeFly = false;
    }
    //Move FB = Forward/Back = up/down
    //Move LR = Left/Right
    static F32 paddleMovementDivisor = 10;
    //Camera controls
    if (state()._angleLR) cam.rotateYaw(state()._angleLR);
    if (state()._angleUD) cam.rotatePitch(state()._angleUD);

    SceneGraphNode* paddle = _sceneGraph->findNode("paddle");

    vec3<F32> pos = paddle->getTransform()->getPosition();

    //Paddle movement is limited to the [-3,3] range except for Y-descent
    if(state()._moveFB){
        if((state()._moveFB > 0 && pos.y >= 3) || (state()._moveFB < 0 && pos.y <= 0.5f)) return;
        paddle->getTransform()->translateY(state()._moveFB / paddleMovementDivisor);
    }

    if(state()._moveLR){
        //Left/right movement is flipped for proper control
        if((state()._moveLR < 0 && pos.x >= 3) || (state()._moveLR > 0 && pos.x <= -3)) return;
        paddle->getTransform()->translateX(state()._moveLR / paddleMovementDivisor);
    }
}

bool PingPongScene::load(const std::string& name, CameraManager* const cameraMgr, GUI* const gui){
    _freeFly = false;
    _wasInFreeFly = false;

    //Load scene resources
    bool loadState = SCENE_LOAD(name,cameraMgr,gui,true,true);
    //Add a light
    addDefaultLight();
    addDefaultSky();
    //Position the camera
    //renderState().getCamera().setPitch(-90);
    renderState().getCamera().setEye(vec3<F32>(0,2.5f,6.5f));

    return loadState;
}

bool PingPongScene::loadResources(bool continueOnErrors){
    //Create a ball
    ResourceDescriptor minge("Ping Pong Ball");
    _ball = CreateResource<Sphere3D>(minge);
    _ballSGN = addGeometry(_ball,"PingPongBallSGN");
    _ball->setResolution(16);
    _ball->setRadius(0.1f);
    _ballSGN->getTransform()->translate(vec3<F32>(0, 2 ,2));
    _ball->getMaterial()->setDiffuse(vec4<F32>(0.4f,0.4f,0.4f,1.0f));
    _ball->getMaterial()->setAmbient(vec4<F32>(0.25f,0.25f,0.25f,1.0f));
    _ball->getMaterial()->setShininess(36.8f);
    _ball->getMaterial()->setSpecular(vec4<F32>(0.774597f,0.774597f,0.774597f,1.0f));

    /*ResourceDescriptor tempLight("Light Omni");
    tempLight.setId(2);
    tempLight.setEnumValue(LIGHT_TYPE_POINT);
    Light* light = CreateResource<Light>(tempLight);
    addLight(light, _sceneGraph->getRoot());
    light->setLightProperties(LIGHT_PROPERTY_BRIGHTNESS, 30.0f);
    light->setCastShadows(false);
    light->setPosition(vec3<F32>(0, 6, 2));
    */
    //Buttons and text labels
    _GUI->addButton("Serve", "Serve", vec2<I32>(renderState().cachedResolution().width-120 ,
                    renderState().cachedResolution().height/1.1f),
                    vec2<U32>(100,25),vec3<F32>(0.65f),
                    DELEGATE_BIND(&PingPongScene::serveBall,this));

    _GUI->addText("Score",vec2<I32>(renderState().cachedResolution().width - 120, renderState().cachedResolution().height/1.3f),
                  Font::DIVIDE_DEFAULT,vec3<F32>(1,0,0), "Score: %d",0);

    _GUI->addText("Message",vec2<I32>(renderState().cachedResolution().width - 120, renderState().cachedResolution().height/1.5f),
                  Font::DIVIDE_DEFAULT,vec3<F32>(1,0,0), "");
    _GUI->addText("insults",vec2<I32>(renderState().cachedResolution().width/4, renderState().cachedResolution().height/3),
                  Font::DIVIDE_DEFAULT,vec3<F32>(0,1,0), "");
    _GUI->addText("fpsDisplay",               //Unique ID
                  vec2<I32>(60,60),           //Position
                  Font::DIVIDE_DEFAULT,       //Font
                  vec3<F32>(0.0f,0.2f, 1.0f), //Color
                  "FPS: %s",0);               //Text and arguments
    //Add some taunts
    _quotes.push_back("Ha ha ... even Odin's laughin'!");
    _quotes.push_back("If you're a ping-pong player, I'm Jimmy Page");
    _quotes.push_back("Ooolee, ole ole ole, see the ball? ... It's past your end");
    _quotes.push_back("You're lucky the room's empty. I'd be so ashamed otherwise if I were you");
    _quotes.push_back("It's not the hard. Even a monkey can do it.");

    _guiTimers.push_back(0.0); //Fps
    _taskTimers.push_back(0.0); //Light
    return true;
}

bool PingPongScene::onKeyUp(const OIS::KeyEvent& key){
    bool keyState = Scene::onKeyUp(key);
    switch(key.key)	{
        default: break;
        case OIS::KC_F: _freeFly = !_freeFly;
    }
    return keyState;
}



bool PingPongScene::onJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis,I32 deadZone){
    return Scene::onJoystickMoveAxis(key,axis,deadZone);
}

bool PingPongScene::onJoystickButtonUp(const OIS::JoyStickEvent& key, I8 button){
    if(button == 0 && key.device->getID() != InputInterface::JOY_1)  serveBall();
    return Scene::onJoystickButtonUp(key, button);
}