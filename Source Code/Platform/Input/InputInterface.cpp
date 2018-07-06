#include "Headers/InputInterface.h"
#include "Core/Headers/ParamHandler.h"

namespace Divide {
namespace Input {

typedef std::map<stringImpl, OIS::KeyCode> KeyByNameMap;
static KeyByNameMap initKeyByNameMap();
namespace {
    // App. heart beat frequency.
    const U8 g_nHartBeatFreq = 30;  // Hz

    // Effects update frequency (Hz) : Needs to be quite lower than app. 
    // heart beat frequency,
    // if we want to be able to calmly study effect changes ...
    const U8 g_nEffectUpdateFreq = 5;  // Hz

    KeyByNameMap g_keysByNameMap = initKeyByNameMap();
};

InputInterface::InputInterface(Kernel& parent)
     : KernelComponent(parent), 
      _pInputInterface(nullptr),
      _pEventHdlr(nullptr),
      _pJoystickInterface(nullptr),
      _pEffectMgr(nullptr),
      _bMustStop(false),
      _bIsInitialized(false),
      _keys(vectorImpl<KeyEvent>(KeyCode_PLACEHOLDER, KeyEvent(0)))
{
    for (U16 i = 0; i < KeyCode_PLACEHOLDER; ++i) {
        _keys[i]._key = static_cast<KeyCode>(i);
    }
}

InputInterface::~InputInterface()
{
    assert(!_bIsInitialized);
}

ErrorCode InputInterface::init(Kernel& kernel, const vec2<U16>& inputAreaDimensions) {
    if (_bIsInitialized) {
        return ErrorCode::NO_ERR;
    }

    Console::printfn(Locale::get(_ID("INPUT_CREATE_START")));

    OIS::ParamList pl;

    std::stringstream ss;
    ss << (size_t)(Application::instance().sysInfo()._windowHandle);
    // Create OIS input manager
    pl.insert(std::make_pair("WINDOW", ss.str()));
    pl.insert(std::make_pair("GLXWINDOW", ss.str()));
    pl.insert(std::make_pair("w32_mouse", "DISCL_FOREGROUND"));
    pl.insert(std::make_pair("w32_mouse", "DISCL_NONEXCLUSIVE"));
    pl.insert(std::make_pair("w32_keyboard", "DISCL_FOREGROUND"));
    pl.insert(std::make_pair("w32_keyboard", "DISCL_NONEXCLUSIVE"));
    pl.insert(std::make_pair("x11_mouse_grab", "false"));
    pl.insert(std::make_pair("x11_mouse_hide", "false"));
    pl.insert(std::make_pair("x11_keyboard_grab", "false"));
    pl.insert(std::make_pair("XAutoRepeatOn", "true"));

    _pInputInterface = OIS::InputManager::createInputSystem(pl);
    assert(_pInputInterface != nullptr && "InputInterface error: Could not create OIS Input Interface");

    Console::printfn(Locale::get(_ID("INPUT_CREATE_OK")),
                     _pInputInterface->inputSystemName().c_str());

    // Create the event handler.
    _pEventHdlr = MemoryManager_NEW EventHandler(this, kernel);
    assert(_pEventHdlr != nullptr && "InputInterface error: EventHandler allocation failed!");

    I32 numKB = _pInputInterface->getNumberOfDevices(OIS::OISKeyboard);
    I32 numMouse = _pInputInterface->getNumberOfDevices(OIS::OISMouse);

    if (numKB == 0) {
        Console::printfn(Locale::get(_ID("ERROR_INPUT_CREATE_KB")), "No keyboards detected");
    }
    if (numMouse == 0) {
        Console::printfn(Locale::get(_ID("ERROR_INPUT_CREATE_MOUSE")), "No mice detected");
    }

    for (I32 i = 0; i < std::max(numKB, numMouse); ++i) {
        OIS::Keyboard* kb = nullptr;
        OIS::Mouse* mouse = nullptr;

        if (i < numKB) {
            // Create a simple keyboard
            kb = static_cast<OIS::Keyboard*>(_pInputInterface->createInputObject(OIS::OISKeyboard, true));
            kb->setEventCallback(_pEventHdlr);
            _keyboardIDToEntry[kb->getID()] = i;
        }
        if (i < numMouse) {
            mouse = static_cast<OIS::Mouse*>(_pInputInterface->createInputObject(OIS::OISMouse, true));
            mouse->setEventCallback(_pEventHdlr);

            const OIS::MouseState& ms = mouse->getMouseState();  // width and height are mutable
            ms.width = inputAreaDimensions.width;
            ms.height = inputAreaDimensions.height;
            _mouseIdToEntry[mouse->getID()] = i;
        }

        _kbMouseDevices.emplace_back(kb, mouse);
    }

    // Limit max joysticks to MAX_ALLOWED_JOYSTICKS
    I32 numJoysticks = std::min(_pInputInterface->getNumberOfDevices(OIS::OISJoyStick), to_int(Joystick::COUNT));

    if (numJoysticks > 0) {
        U32 entryNum = to_const_uint(Joystick::JOYSTICK_1);

        for (I32 i = 0; i < numJoysticks; ++i) {
            OIS::JoyStick* joystick = static_cast<OIS::JoyStick*>(_pInputInterface->createInputObject(OIS::OISJoyStick, true));
            joystick->setEventCallback(_pEventHdlr);
            _joystickIdToEntry[joystick->getID()] = static_cast<Joystick>(entryNum++);
            _joysticks.push_back(joystick);
        }

        // Create the joystick manager.
        _pJoystickInterface = MemoryManager_NEW JoystickInterface(_pInputInterface, _pEventHdlr);

        for (I32 i = 0; i < numJoysticks; ++i) {
            I32 max = _joysticks[i]->MAX_AXIS - 4000;
            _pJoystickInterface->setJoystickData(static_cast<Joystick>(i), JoystickData(max / 10, max));
        }

        if (!_pJoystickInterface->wasFFDetected()) {
            Console::printfn(Locale::get(_ID("WARN_INPUT_NO_FORCE_FEEDBACK")));
        } else {
            // Create force feedback effect manager.
            assert(_pEffectMgr == nullptr);
            _pEffectMgr = MemoryManager_NEW EffectManager(_pJoystickInterface, g_nEffectUpdateFreq);
            // Initialize the event handler.
            _pEventHdlr->initialize(_pJoystickInterface, _pEffectMgr);
        }
    } else {
        Console::printfn(Locale::get(_ID("ERROR_INPUT_CREATE_JOYSTICK")), "No joystick / gamepad devices detected!");
    }

    _bIsInitialized = true;

    return ErrorCode::NO_ERR;
}

void InputInterface::onChangeWindowSize(U16 w, U16 h) {
    for (KBMousePair& it : _kbMouseDevices) {
        if (it.second) {
            const OIS::MouseState& ms = it.second->getMouseState();
            ms.width = w;
            ms.height = h;
        }
    }
}

void InputInterface::terminate() {
    if (_pInputInterface) {
        for (KBMousePair& it : _kbMouseDevices) {
            if (it.first) {
                _pInputInterface->destroyInputObject(it.first);
            }
            if (it.second) {
                _pInputInterface->destroyInputObject(it.second);
            }
        }
        _kbMouseDevices.clear();

        for (OIS::JoyStick* it : _joysticks) {
             _pInputInterface->destroyInputObject(it);
        }
        _joysticks.clear();
        MemoryManager::DELETE(_pJoystickInterface);

        OIS::InputManager::destroyInputSystem(_pInputInterface);
        _pInputInterface = nullptr;
    }

    MemoryManager::DELETE(_pEventHdlr);

    MemoryManager::DELETE(_pEffectMgr);

    _bIsInitialized = false;
}

U8 InputInterface::update(const U64 deltaTime) {
    const U8 nMaxEffectUpdateCnt = g_nHartBeatFreq / g_nEffectUpdateFreq;
    U8 nEffectUpdateCnt = 0;

    if (!_bIsInitialized) {
        return 0;
    }
    if (_bMustStop) {
        terminate();
        return 0;
    }

    // This fires off buffered events for keyboards
    for (KBMousePair& it : _kbMouseDevices) {
        if (it.first) {
            it.first->capture();
        }
        if (it.second) {
            it.second->capture();
        }
    }

    if (_joysticks.size() > 0) {
        for (OIS::JoyStick* it : _joysticks) {
            it->capture();
        }

        // This fires off buffered events for each joystick we have
        if (_pEffectMgr != nullptr) {
            _pJoystickInterface->captureEvents();
            // Update currently selected effects if time has come to.
            if (!nEffectUpdateCnt) {
                _pEffectMgr->updateActiveEffects();
                nEffectUpdateCnt = nMaxEffectUpdateCnt;
            } else {
                nEffectUpdateCnt--;
            }
        }
    }

    return 1;
}

InputState InputInterface::getKeyState(U8 deviceIndex, KeyCode keyCode) const {
    I32 kbID = keyboard(deviceIndex);
    if (kbID != -1) {
        return _kbMouseDevices[kbID].first->isKeyDown(keyCode) ? InputState::PRESSED
                                                               : InputState::RELEASED;
    }

    return InputState::COUNT;
}

InputState InputInterface::getMouseButtonState(U8 deviceIndex, MouseButton button) const {
    I32 mouseID = mouse(deviceIndex);
    if (mouseID != -1) {
        return _kbMouseDevices[mouseID].second->getMouseState().buttonDown(button) ? InputState::PRESSED
                                                                                   : InputState::RELEASED;
    }

    return InputState::COUNT;
}

InputState InputInterface::getJoystickeButtonState(Input::Joystick deviceIndex, JoystickButton button) const {
    for (hashMapImpl<I32, Joystick>::value_type it : _joystickIdToEntry) {
        if (it.second == deviceIndex) {
            for (OIS::JoyStick* deviceIt : _joysticks) {
                if (deviceIt->getID() == it.first) {
                    const OIS::JoyStickState& joyState = deviceIt->getJoyStickState();
                    return joyState.mButtons[button] ? InputState::PRESSED
                                                     : InputState::RELEASED;
                }
                DIVIDE_UNEXPECTED_CALL("Invalid joystick entry detected!");
            }
        }
    }
    return InputState::COUNT;
}

Joystick InputInterface::joystick(I32 deviceID) const {
    hashMapImpl<I32, Joystick>::const_iterator it = _joystickIdToEntry.find(deviceID);
    if (it != std::cend(_joystickIdToEntry)) {
        return it->second;
    }

    return Joystick::COUNT;
}

I32 InputInterface::keyboard(I32 deviceID) const {
    hashMapImpl<I32, I32>::const_iterator it = _keyboardIDToEntry.find(deviceID);
    if (it != std::cend(_keyboardIDToEntry)) {
        return it->second;
    }

    return -1;
}

I32 InputInterface::mouse(I32 deviceID) const {
    hashMapImpl<I32, I32>::const_iterator it = _mouseIdToEntry.find(deviceID);
    if (it != std::cend(_mouseIdToEntry)) {
        return it->second;
    }

    return -1;
}

MouseButton  InputInterface::mouseButtonByName(const stringImpl& buttonName) {
    if (Util::CompareIgnoreCase("MB_" + buttonName, "MB_Left")) {
        return MouseButton::MB_Left;
    } else if (Util::CompareIgnoreCase("MB_" + buttonName, "MB_Right")) {
        return MouseButton::MB_Right;
    } else if (Util::CompareIgnoreCase("MB_" + buttonName, "MB_Middle")) {
        return MouseButton::MB_Middle;
    } else if (Util::CompareIgnoreCase("MB_" + buttonName, "MB_Button3")) {
        return MouseButton::MB_Button3;
    } else if (Util::CompareIgnoreCase("MB_" + buttonName, "MB_Button4")) {
        return MouseButton::MB_Button4;
    } else if (Util::CompareIgnoreCase("MB_" + buttonName, "MB_Button5")) {
        return MouseButton::MB_Button5;
    } else if (Util::CompareIgnoreCase("MB_" + buttonName, "MB_Button6")) {
        return MouseButton::MB_Button6;
    }//else if (Util::CompareIgnoreCase(buttonName, "MB_Button7")) {
     //}

    return MouseButton::MB_Button7;
}

JoystickElement  InputInterface::joystickElementByName(const stringImpl& elementName) {
    if (Util::CompareIgnoreCase(elementName, "POV")) {
        return JoystickElement(JoystickElementType::POV_MOVE);
    } else if (Util::CompareIgnoreCase(elementName, "AXIS")) {
        return JoystickElement(JoystickElementType::AXIS_MOVE);
    } else if (Util::CompareIgnoreCase(elementName, "SLIDER")) {
        return JoystickElement(JoystickElementType::SLIDER_MOVE);
    } else if (Util::CompareIgnoreCase(elementName, "VECTOR")) {
        return JoystickElement(JoystickElementType::VECTOR_MOVE);
    } 
    vectorImpl<stringImpl> buttonElements = Util::Split(elementName, '_');
    assert(buttonElements.size() == 2 && "Invalid joystick element name!");
    assert(Util::CompareIgnoreCase(buttonElements[0], "BUTTON"));

    I32 buttonId = Util::ConvertData<I32, std::string>(buttonElements[1].c_str());
    return JoystickElement(JoystickElementType::BUTTON_PRESS, to_ubyte(buttonId));
}

OIS::KeyCode InputInterface::keyCodeByName(const stringImpl& name) {
    stringImpl nameCpy("KC_" + name);
    std::map<stringImpl, OIS::KeyCode>::const_iterator it;
    it = g_keysByNameMap.find(nameCpy);
    if (it == std::cend(g_keysByNameMap)) {
        std::transform(std::begin(nameCpy),
                       std::end(nameCpy),
                       std::begin(nameCpy),
                       [](char c) {
                           return static_cast<char>(::toupper(c));
                       });

        it = g_keysByNameMap.find(nameCpy);
    }
    assert(it != std::cend(g_keysByNameMap));

    return it->second;
}

KeyByNameMap initKeyByNameMap() {
    KeyByNameMap keysByNameMap;

    keysByNameMap["KC_UNASSIGNED"] = OIS::KC_UNASSIGNED;
    keysByNameMap["KC_ESCAPE"] = OIS::KC_ESCAPE;
    keysByNameMap["KC_1"] = OIS::KC_1;
    keysByNameMap["KC_2"] = OIS::KC_2;
    keysByNameMap["KC_3"] = OIS::KC_3;
    keysByNameMap["KC_4"] = OIS::KC_4;
    keysByNameMap["KC_5"] = OIS::KC_5;
    keysByNameMap["KC_6"] = OIS::KC_6;
    keysByNameMap["KC_7"] = OIS::KC_7;
    keysByNameMap["KC_8"] = OIS::KC_8;
    keysByNameMap["KC_9"] = OIS::KC_9;
    keysByNameMap["KC_0"] = OIS::KC_0;
    keysByNameMap["KC_MINUS"] = OIS::KC_MINUS;
    keysByNameMap["KC_EQUALS"] = OIS::KC_EQUALS;
    keysByNameMap["KC_BACK"] = OIS::KC_BACK;
    keysByNameMap["KC_TAB"] = OIS::KC_TAB;
    keysByNameMap["KC_Q"] = OIS::KC_Q;
    keysByNameMap["KC_W"] = OIS::KC_W;
    keysByNameMap["KC_E"] = OIS::KC_E;
    keysByNameMap["KC_R"] = OIS::KC_R;
    keysByNameMap["KC_T"] = OIS::KC_T;
    keysByNameMap["KC_Y"] = OIS::KC_Y;
    keysByNameMap["KC_U"] = OIS::KC_U;
    keysByNameMap["KC_I"] = OIS::KC_I;
    keysByNameMap["KC_O"] = OIS::KC_O;
    keysByNameMap["KC_P"] = OIS::KC_P;
    keysByNameMap["KC_LBRACKET"] = OIS::KC_LBRACKET;
    keysByNameMap["KC_RBRACKET"] = OIS::KC_RBRACKET;
    keysByNameMap["KC_RETURN"] = OIS::KC_RETURN;
    keysByNameMap["KC_LCONTROL"] = OIS::KC_LCONTROL;
    keysByNameMap["KC_A"] = OIS::KC_A;
    keysByNameMap["KC_S"] = OIS::KC_S;
    keysByNameMap["KC_D"] = OIS::KC_D;
    keysByNameMap["KC_F"] = OIS::KC_F;
    keysByNameMap["KC_G"] = OIS::KC_G;
    keysByNameMap["KC_H"] = OIS::KC_H;
    keysByNameMap["KC_J"] = OIS::KC_J;
    keysByNameMap["KC_K"] = OIS::KC_K;
    keysByNameMap["KC_L"] = OIS::KC_L;
    keysByNameMap["KC_SEMICOLON"] = OIS::KC_SEMICOLON;
    keysByNameMap["KC_APOSTROPHE"] = OIS::KC_APOSTROPHE;
    keysByNameMap["KC_GRAVE"] = OIS::KC_GRAVE;
    keysByNameMap["KC_LSHIFT"] = OIS::KC_LSHIFT;
    keysByNameMap["KC_BACKSLASH"] = OIS::KC_BACKSLASH;
    keysByNameMap["KC_Z"] = OIS::KC_Z;
    keysByNameMap["KC_X"] = OIS::KC_X;
    keysByNameMap["KC_C"] = OIS::KC_C;
    keysByNameMap["KC_V"] = OIS::KC_V;
    keysByNameMap["KC_B"] = OIS::KC_B;
    keysByNameMap["KC_N"] = OIS::KC_N;
    keysByNameMap["KC_M"] = OIS::KC_M;
    keysByNameMap["KC_COMMA"] = OIS::KC_COMMA;
    keysByNameMap["KC_PERIOD"] = OIS::KC_PERIOD;
    keysByNameMap["KC_SLASH"] = OIS::KC_SLASH;
    keysByNameMap["KC_RSHIFT"] = OIS::KC_RSHIFT;
    keysByNameMap["KC_MULTIPLY"] = OIS::KC_MULTIPLY;
    keysByNameMap["KC_LMENU"] = OIS::KC_LMENU;
    keysByNameMap["KC_SPACE"] = OIS::KC_SPACE;
    keysByNameMap["KC_CAPITAL"] = OIS::KC_CAPITAL;
    keysByNameMap["KC_F1"] = OIS::KC_F1;
    keysByNameMap["KC_F2"] = OIS::KC_F2;
    keysByNameMap["KC_F3"] = OIS::KC_F3;
    keysByNameMap["KC_F4"] = OIS::KC_F4;
    keysByNameMap["KC_F5"] = OIS::KC_F5;
    keysByNameMap["KC_F6"] = OIS::KC_F6;
    keysByNameMap["KC_F7"] = OIS::KC_F7;
    keysByNameMap["KC_F8"] = OIS::KC_F8;
    keysByNameMap["KC_F9"] = OIS::KC_F9;
    keysByNameMap["KC_F10"] = OIS::KC_F10;
    keysByNameMap["KC_NUMLOCK"] = OIS::KC_NUMLOCK;
    keysByNameMap["KC_SCROLL"] = OIS::KC_SCROLL;
    keysByNameMap["KC_NUMPAD7"] = OIS::KC_NUMPAD7;
    keysByNameMap["KC_NUMPAD8"] = OIS::KC_NUMPAD8;
    keysByNameMap["KC_NUMPAD9"] = OIS::KC_NUMPAD9;
    keysByNameMap["KC_SUBTRACT"] = OIS::KC_SUBTRACT;
    keysByNameMap["KC_NUMPAD4"] = OIS::KC_NUMPAD4;
    keysByNameMap["KC_NUMPAD5"] = OIS::KC_NUMPAD5;
    keysByNameMap["KC_NUMPAD6"] = OIS::KC_NUMPAD6;
    keysByNameMap["KC_ADD"] = OIS::KC_ADD;
    keysByNameMap["KC_NUMPAD1"] = OIS::KC_NUMPAD1;
    keysByNameMap["KC_NUMPAD2"] = OIS::KC_NUMPAD2;
    keysByNameMap["KC_NUMPAD3"] = OIS::KC_NUMPAD3;
    keysByNameMap["KC_NUMPAD0"] = OIS::KC_NUMPAD0;
    keysByNameMap["KC_DECIMAL"] = OIS::KC_DECIMAL;
    keysByNameMap["KC_OEM_102"] = OIS::KC_OEM_102;
    keysByNameMap["KC_F11"] = OIS::KC_F11;
    keysByNameMap["KC_F12"] = OIS::KC_F12;
    keysByNameMap["KC_F13"] = OIS::KC_F13;
    keysByNameMap["KC_F14"] = OIS::KC_F14;
    keysByNameMap["KC_F15"] = OIS::KC_F15;
    keysByNameMap["KC_KANA"] = OIS::KC_KANA;
    keysByNameMap["KC_ABNT_C1"] = OIS::KC_ABNT_C1;
    keysByNameMap["KC_CONVERT"] = OIS::KC_CONVERT;
    keysByNameMap["KC_NOCONVERT"] = OIS::KC_NOCONVERT;
    keysByNameMap["KC_YEN"] = OIS::KC_YEN;
    keysByNameMap["KC_ABNT_C2"] = OIS::KC_ABNT_C2;
    keysByNameMap["KC_NUMPADEQUALS"] = OIS::KC_NUMPADEQUALS;
    keysByNameMap["KC_PREVTRACK"] = OIS::KC_PREVTRACK;
    keysByNameMap["KC_AT"] = OIS::KC_AT;
    keysByNameMap["KC_COLON"] = OIS::KC_COLON;
    keysByNameMap["KC_UNDERLINE"] = OIS::KC_UNDERLINE;
    keysByNameMap["KC_KANJI"] = OIS::KC_KANJI;
    keysByNameMap["KC_STOP"] = OIS::KC_STOP;
    keysByNameMap["KC_AX"] = OIS::KC_AX;
    keysByNameMap["KC_UNLABELED"] = OIS::KC_UNLABELED;
    keysByNameMap["KC_NEXTTRACK"] = OIS::KC_NEXTTRACK;
    keysByNameMap["KC_NUMPADENTER"] = OIS::KC_NUMPADENTER;
    keysByNameMap["KC_RCONTROL"] = OIS::KC_RCONTROL;
    keysByNameMap["KC_MUTE"] = OIS::KC_MUTE;
    keysByNameMap["KC_CALCULATOR"] = OIS::KC_CALCULATOR;
    keysByNameMap["KC_PLAYPAUSE"] = OIS::KC_PLAYPAUSE;
    keysByNameMap["KC_MEDIASTOP"] = OIS::KC_MEDIASTOP;
    keysByNameMap["KC_VOLUMEDOWN"] = OIS::KC_VOLUMEDOWN;
    keysByNameMap["KC_VOLUMEUP"] = OIS::KC_VOLUMEUP;
    keysByNameMap["KC_WEBHOME"] = OIS::KC_WEBHOME;
    keysByNameMap["KC_NUMPADCOMMA"] = OIS::KC_NUMPADCOMMA;
    keysByNameMap["KC_DIVIDE"] = OIS::KC_DIVIDE;
    keysByNameMap["KC_SYSRQ"] = OIS::KC_SYSRQ;
    keysByNameMap["KC_RMENU"] = OIS::KC_RMENU;
    keysByNameMap["KC_PAUSE"] = OIS::KC_PAUSE;
    keysByNameMap["KC_HOME"] = OIS::KC_HOME;
    keysByNameMap["KC_UP"] = OIS::KC_UP;
    keysByNameMap["KC_PGUP"] = OIS::KC_PGUP;
    keysByNameMap["KC_LEFT"] = OIS::KC_LEFT;
    keysByNameMap["KC_RIGHT"] = OIS::KC_RIGHT;
    keysByNameMap["KC_END"] = OIS::KC_END;
    keysByNameMap["KC_DOWN"] = OIS::KC_DOWN;
    keysByNameMap["KC_PGDOWN"] = OIS::KC_PGDOWN;
    keysByNameMap["KC_INSERT"] = OIS::KC_INSERT;
    keysByNameMap["KC_DELETE"] = OIS::KC_DELETE;
    keysByNameMap["KC_LWIN"] = OIS::KC_LWIN;
    keysByNameMap["KC_RWIN"] = OIS::KC_RWIN;
    keysByNameMap["KC_APPS"] = OIS::KC_APPS;
    keysByNameMap["KC_POWER"] = OIS::KC_POWER;
    keysByNameMap["KC_SLEEP"] = OIS::KC_SLEEP;
    keysByNameMap["KC_WAKE"] = OIS::KC_WAKE;
    keysByNameMap["KC_WEBSEARCH"] = OIS::KC_WEBSEARCH;
    keysByNameMap["KC_WEBFAVORITES"] = OIS::KC_WEBFAVORITES;
    keysByNameMap["KC_WEBREFRESH"] = OIS::KC_WEBREFRESH;
    keysByNameMap["KC_WEBSTOP"] = OIS::KC_WEBSTOP;
    keysByNameMap["KC_WEBFORWARD"] = OIS::KC_WEBFORWARD;
    keysByNameMap["KC_WEBBACK"] = OIS::KC_WEBBACK;
    keysByNameMap["KC_MYCOMPUTER"] = OIS::KC_MYCOMPUTER;
    keysByNameMap["KC_MAIL"] = OIS::KC_MAIL;
    keysByNameMap["KC_MEDIASELECT"] = OIS::KC_MEDIASELECT;

    return keysByNameMap;
}

};  // namespace Input
};  // namespace Divide
