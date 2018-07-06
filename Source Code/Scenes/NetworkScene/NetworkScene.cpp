#include "Headers/NetworkScene.h"
#include "Network/Headers/ASIOImpl.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Managers/Headers/SceneManager.h"

namespace Divide {

void NetworkScene::processInput(const U64 deltaTime) {
    Light* light = LightManager::instance().getLight(0, LightType::DIRECTIONAL);
    vec4<F32> vSunColor = Lerp(vec4<F32>(1.0f, 0.5f, 0.0f, 1.0f),
        vec4<F32>(1.0f, 1.0f, 0.8f, 1.0f),
        0.25f + cosf(_sunAngle.y) * 0.75f);

    light->setDiffuseColor(vSunColor);

    _currentSky = addSky();
}

void NetworkScene::processGUI(const U64 deltaTime) {
    D64 FpsDisplay = Time::SecondsToMilliseconds(0.3);
    D64 TimeDisplay = Time::SecondsToMilliseconds(0.01);
    D64 ServerPing = Time::SecondsToMilliseconds(1.0);
    if (_guiTimers[0] >= FpsDisplay) {
        _GUI->modifyText(_ID("fpsDisplay"),
                         Util::StringFormat("FPS: %5.2f", Time::ApplicationTimer::instance().getFps()));
        _guiTimers[0] = 0.0;
    }

    if (_guiTimers[1] >= TimeDisplay) {
        _GUI->modifyText(_ID("timeDisplay"),
                         Util::StringFormat("Elapsed time: %5.0f", time));
        _guiTimers[1] = 0.0;
    }

    if (_taskTimers[2] >= ServerPing) {
        _GUI->modifyText(
            _ID("statusText"),
            (char*)_paramHandler.getParam<stringImpl>(_ID("asioStatus")).c_str());
        _GUI->modifyText(_ID("serverMessage"),
                         (char*)_paramHandler.getParam<stringImpl>(
                             _ID("serverResponse")).c_str());
        _guiTimers[2] = 0.0;
    }
}

void NetworkScene::checkPatches(I64 btnGUID) {
    if (_modelDataArray.empty()) return;
    WorldPacket p(OPCodesEx::CMSG_GEOMETRY_LIST);
    p << stringImpl("NetworkScene");
    p << _modelDataArray.size();

    /*for(vectorImpl<FileData>::iterator _iter = std::begin(_modelDataArray);
    _iter != std::end(_modelDataArray); ++_iter)    {
        p << (*_iter).ItemName;
        p << (*_iter).ModelName;
        p << (*_iter).version;
    }*/
    ASIOImpl::instance().sendPacket(p);
}

bool NetworkScene::load(const stringImpl& name, GUI* const gui) {
    ASIOImpl::instance().init(
        _paramHandler.getParam<stringImpl>(_ID("serverAddress")).c_str(), "443");
    // Load scene resources
    bool loadState = SCENE_LOAD(name, gui, true, true);

    _paramHandler.setParam(_ID("serverResponse"), "waiting");
    addLight(LightType::DIRECTIONAL, _sceneGraph->getRoot());
    _currentSky = addSky();
    renderState().getCamera().setEye(vec3<F32>(0, 30, -30));

    return loadState;
}

void NetworkScene::test(I64 btnGUID) {
    WorldPacket p(OPCodesEx::CMSG_PING);
    p << Time::ElapsedMilliseconds();
    ASIOImpl::instance().sendPacket(p);
}

void NetworkScene::connect(I64 btnGUID) {
    _GUI->modifyText(_ID("statusText"), "Connecting to server ...");
    ASIOImpl::instance().connect(_paramHandler.getParam<stringImpl>(_ID("serverAddress")), "443");
}

void NetworkScene::disconnect(I64 btnGUID) {
    if (!ASIOImpl::instance().isConnected()) {
        _GUI->modifyText(_ID("statusText"), "Disconnecting to server ...");
    }
    ASIOImpl::instance().disconnect();
}

bool NetworkScene::loadResources(bool continueOnErrors) {
    _sunAngle = vec2<F32>(0.0f, Angle::DegreesToRadians(45.0f));
    _sunvector =
        vec4<F32>(-cosf(_sunAngle.x) * sinf(_sunAngle.y), -cosf(_sunAngle.y),
                  -sinf(_sunAngle.x) * sinf(_sunAngle.y), 0.0f);

    const vec2<U16>& resolution
        = Application::instance().windowManager().getActiveWindow().getDimensions();

    _GUI->addText(_ID("fpsDisplay"),  // Unique ID
                  vec2<I32>(60, 60),  // Position
                  Font::DIVIDE_DEFAULT,  // Font
                  vec4<U8>(0, 164, 255, 255),  // Color
                  Util::StringFormat("FPS: %d", 0));  // Text and arguments
    _GUI->addText(_ID("timeDisplay"), vec2<I32>(60, 70), Font::DIVIDE_DEFAULT,
                  vec4<U8>(164, 64, 64, 255),
                  Util::StringFormat("Elapsed time: %5.0f", Time::ElapsedSeconds()));

    _GUI->addText(_ID("serverMessage"),
                  vec2<I32>(resolution.width / 4,
                            resolution.height / 1),
                  Font::DIVIDE_DEFAULT, 
                  vec4<U8>(128, 128, 64, 255),
                  Util::StringFormat("Server says: %s", "<< nothing yet >>"));
    _GUI->addText(_ID("statusText"),
                  vec2<I32>(resolution.width / 3,
                            resolution.height / 2),
                  Font::DIVIDE_DEFAULT,
                  vec4<U8>(64, 128, 64, 255),
                  "");

    _GUI->addButton(
        _ID("getPing"), "ping me",
        vec2<I32>(60, to_int(resolution.height / 1.1f)),
        vec2<U32>(100, 25), vec3<F32>(0.6f, 0.6f, 0.6f),
        DELEGATE_BIND(&NetworkScene::test, this, std::placeholders::_1));
    _GUI->addButton(
        _ID("disconnect"), "disconnect",
        vec2<I32>(180, to_int(resolution.height / 1.1f)),
        vec2<U32>(100, 25), vec3<F32>(0.5f, 0.5f, 0.5f),
        DELEGATE_BIND(&NetworkScene::disconnect, this, std::placeholders::_1));
    _GUI->addButton(
        _ID("connect"), "connect",
        vec2<I32>(300, to_int(resolution.height / 1.1f)),
        vec2<U32>(100, 25), vec3<F32>(0.65f, 0.65f, 0.65f),
        DELEGATE_BIND(&NetworkScene::connect, this, std::placeholders::_1));
    _GUI->addButton(
        _ID("patch"), "patch",
        vec2<I32>(420, to_int(resolution.height / 1.1f)),
        vec2<U32>(100, 25), vec3<F32>(0.65f, 0.65f, 0.65f),
        DELEGATE_BIND(&NetworkScene::checkPatches, this, std::placeholders::_1));

    _guiTimers.push_back(0.0f);  // Fps
    _guiTimers.push_back(0.0f);  // Time
    _guiTimers.push_back(0.0f);  // Server Ping
    return true;
}
};