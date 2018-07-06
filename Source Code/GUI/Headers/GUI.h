/*
   Copyright (c) 2017 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef GUI_H_
#define GUI_H_

#include "GUIInterface.h"
#include "Core/Headers/KernelComponent.h"
#include "Core/Math/Headers/MathMatrices.h"
#include "GUI/CEGUIAddons/Headers/CEGUIInput.h"
#include "Platform/Input/Headers/InputAggregatorInterface.h"

namespace CEGUI {
class Renderer;
};

namespace Divide {

namespace Font {
const static char* DIVIDE_DEFAULT = "DroidSerif-Regular.ttf"; /*"Test.ttf"*/
const static char* BATANG = "Batang.ttf";
const static char* DEJA_VU = "DejaVuSans.ttf";
const static char* DROID_SERIF = "DroidSerif-Regular.ttf";
const static char* DROID_SERIF_ITALIC = "DroidSerif-Italic.ttf";
const static char* DROID_SERIF_BOLD = "DroidSerif-Bold.ttf";
};

class GUIEditor;
class GUIConsole;
class GUIElement;
class ResourceCache;
class PlatformContext;
class RenderStateBlock;

FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

#define CEGUI_DEFAULT_CTX CEGUI::System::getSingleton().getDefaultGUIContext()

class Scene;

/// Graphical User Interface

class SceneGUIElements;
class GUI : public GUIInterface,
            public KernelComponent,
            public Input::InputAggregatorInterface {
public:
    typedef hashMapImpl<I64, SceneGUIElements*> GUIMapPerScene;

public:
    explicit GUI(Kernel& parent);
    ~GUI();

    /// Create the GUI
    bool init(PlatformContext& context, ResourceCache& cache, const vec2<U16>& renderResolution);
    void destroy();

    void onChangeResolution(U16 w, U16 h) override;
    void onChangeScene(Scene* newScene);
    void onUnloadScene(Scene* scene);

    /// Main update call
    void update(const U64 deltaTime);

    template<typename T = GUIElement>
    inline T* getGUIElement(I64 sceneID, U64 elementName) {
        static_assert(std::is_base_of<GUIElement, T>::value,
            "getGuiElement error: Target is not a valid GUI item");

        return static_cast<T*>(getGUIElementImpl(sceneID, elementName, getTypeEnum<T>()));
    }

    template<typename T = GUIElement>
    inline T* getGUIElement(I64 sceneID, I64 elementID) {
        static_assert(std::is_base_of<GUIElement, T>::value,
            "getGuiElement error: Target is not a valid GUI item");

        return static_cast<T*>(getGUIElementImpl(sceneID, elementID, getTypeEnum<T>()));
    }

    /// Get a pointer to our console window
    inline GUIConsole* const getConsole() const { return _console; }
    inline const GUIEditor& getEditor() const { return *_guiEditor; }
    inline CEGUI::Window* rootSheet() const { return _rootSheet; }
    inline const stringImpl& guiScheme() const { return _defaultGUIScheme; }
    /// Used by CEGUI to setup rendering (D3D/OGL/OGRE/etc)
    bool bindRenderer(CEGUI::Renderer& renderer);
    void selectionChangeCallback(Scene* const activeScene, U8 playerIndex);
    /// Return a pointer to the default, general purpose message box
    inline GUIMessageBox* const getDefaultMessageBox() const {
        return _defaultMsgBox;
    }
    /// Used to prevent text updating every frame
    inline void setTextRenderTimer(const U64 renderIntervalUs) {
        _textRenderInterval = renderIntervalUs;
    }
    /// Mouse cursor forced to a certain position
    void setCursorPosition(I32 x, I32 y) const;
    /// Key pressed
    bool onKeyDown(const Input::KeyEvent& key);
    /// Key released
    bool onKeyUp(const Input::KeyEvent& key);
    /// Joystick axis change
    bool joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis);
    /// Joystick direction change
    bool joystickPovMoved(const Input::JoystickEvent& arg, I8 pov);
    /// Joystick button pressed
    bool joystickButtonPressed(const Input::JoystickEvent& arg,
        Input::JoystickButton button);
    /// Joystick button released
    bool joystickButtonReleased(const Input::JoystickEvent& arg,
        Input::JoystickButton button);
    bool joystickSliderMoved(const Input::JoystickEvent& arg, I8 index);
    bool joystickVector3DMoved(const Input::JoystickEvent& arg, I8 index);
    /// Mouse moved
    bool mouseMoved(const Input::MouseEvent& arg);
    /// Mouse button pressed
    bool mouseButtonPressed(const Input::MouseEvent& arg,
        Input::MouseButton button);
    /// Mouse button released
    bool mouseButtonReleased(const Input::MouseEvent& arg,
        Input::MouseButton button);

    Scene* activeScene() {
        return _activeScene;
    }

    const Scene* activeScene() const {
        return _activeScene;
    }

protected:
    GUIElement* getGUIElementImpl(I64 sceneID, U64 elementName, GUIType type) const;
    GUIElement* getGUIElementImpl(I64 sceneID, I64 elementID, GUIType type) const;

protected:
    friend class SceneGUIElements;
    CEGUI::Window* _rootSheet;  //< gui root Window
    stringImpl _defaultGUIScheme;

private:
    void draw(GFXDevice& context) const;

private:
    bool _init;              //< Set to true when the GUI has finished loading
                             /// Toggle CEGUI rendering on/off (e.g. to check raw application rendering
                             /// performance)
    CEGUIInput _ceguiInput;  //< Used to implement key repeat
    GUIConsole* _console;    //< Pointer to the GUIConsole object
    GUIMessageBox* _defaultMsgBox;  //< Pointer to a default message box used for general purpose messages
    GUIEditor*  _guiEditor; //< Pointer to a World Editor type interface
    U64 _textRenderInterval;  //< We should avoid rendering text as fast as possible
                              //for performance reasons
    ShaderProgram_ptr _guiShader;  //<Used to apply colour for text for now

    /// Each scene has its own gui elements! (0 = global)
    Scene* _activeScene;
    bool _enableCEGUIRendering;

    U32 _debugVarCacheCount;
    // GROUP, VAR
    vectorImpl<std::pair<I64, I64>> _debugDisplayEntries;

    /// All the GUI elements created per scene
    GUIMapPerScene _guiStack;
    mutable SharedLock _guiStackLock;
};

};  // namespace Divide
#endif
