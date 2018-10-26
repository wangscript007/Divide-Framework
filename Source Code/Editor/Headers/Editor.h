/*
Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _DIVIDE_EDITOR_H_
#define _DIVIDE_EDITOR_H_

#include "Core/Math/Headers/MathVectors.h"
#include "Core/Time/Headers/ProfileTimer.h"
#include "Core/Headers/PlatformContextComponent.h"

#include "Rendering/Headers/FrameListener.h"

#include "Editor/Widgets/Headers/Gizmo.h"

#include "Platform/Headers/DisplayWindow.h"
#include "Platform/Input/Headers/InputAggregatorInterface.h"

#include <imgui/addons/imguistyleserializer/imguistyleserializer.h>

struct ImDrawData;
namespace Divide {

namespace Attorney {
    class EditorGizmo;
    class EditorPanelManager;
    class EditorOutputWindow;
    class EditorWindowManager;
    class EditorPropertyWindow;
    class EditorSceneViewWindow;
    class EditorSolutionExplorerWindow;
};

class Gizmo;
class Camera;
class MenuBar;
class DockedWindow;
class OutputWindow;
class PanelManager;
class DisplayWindow;
class PropertyWindow;
class SceneGraphNode;
class SceneViewWindow;
class PlatformContext;
class ApplicationOutput;
class SolutionExplorerWindow;

FWD_DECLARE_MANAGED_CLASS(Texture);
FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

struct SizeChangeParams;
struct TransformSettings;

class Editor : public PlatformContextComponent,
               public FrameListener,
               public Input::InputAggregatorInterface {

    friend class Attorney::EditorGizmo;
    friend class Attorney::EditorPanelManager;
    friend class Attorney::EditorOutputWindow;
    friend class Attorney::EditorWindowManager;
    friend class Attorney::EditorPropertyWindow;
    friend class Attorney::EditorSceneViewWindow;
    friend class Attorney::EditorSolutionExplorerWindow;

  public:
    enum class WindowType : U8 {
        SolutionExplorer = 0,
        Properties,
        Output,
        SceneView,
        COUNT
    };

  public:
    explicit Editor(PlatformContext& context,
                    ImGuiStyleEnum theme = ImGuiStyleEnum::ImGuiStyle_GrayCodz01,
                    ImGuiStyleEnum dimmedTheme = ImGuiStyleEnum::ImGuiStyle_EdinBlack);
    ~Editor();

    bool init(const vec2<U16>& renderResolution);
    void close();
    void idle();
    void update(const U64 deltaTimeUS);
    bool needInput() const;

    void toggle(const bool state);
    bool running() const;

    bool shouldPauseSimulation() const;

    void onSizeChange(const SizeChangeParams& params);
    void selectionChangeCallback(PlayerIndex idx, SceneGraphNode* node);

    bool simulationPauseRequested() const;

    void setTransformSettings(const TransformSettings& settings);
    const TransformSettings& getTransformSettings() const;

  protected: //frame listener
    bool frameStarted(const FrameEvent& evt);
    bool framePreRenderStarted(const FrameEvent& evt);
    bool framePreRenderEnded(const FrameEvent& evt);
    bool frameSceneRenderEnded(const FrameEvent& evt);
    bool frameRenderingQueued(const FrameEvent& evt);
    bool framePostRenderStarted(const FrameEvent& evt);
    bool framePostRenderEnded(const FrameEvent& evt);
    bool frameEnded(const FrameEvent& evt);

  public: // input
    /// Key pressed: return true if input was consumed
    bool onKeyDown(const Input::KeyEvent& key);
    /// Key released: return true if input was consumed
    bool onKeyUp(const Input::KeyEvent& key);
    /// Mouse moved: return true if input was consumed
    bool mouseMoved(const Input::MouseEvent& arg);
    /// Mouse button pressed: return true if input was consumed
    bool mouseButtonPressed(const Input::MouseEvent& arg, Input::MouseButton button);
    /// Mouse button released: return true if input was consumed
    bool mouseButtonReleased(const Input::MouseEvent& arg, Input::MouseButton button);

    bool joystickButtonPressed(const Input::JoystickEvent &arg, Input::JoystickButton button) override;
    bool joystickButtonReleased(const Input::JoystickEvent &arg, Input::JoystickButton button) override;
    bool joystickAxisMoved(const Input::JoystickEvent &arg, I8 axis) override;
    bool joystickPovMoved(const Input::JoystickEvent &arg, I8 pov) override;
    bool joystickSliderMoved(const Input::JoystickEvent &arg, I8 index) override;
    bool joystickvector3Moved(const Input::JoystickEvent &arg, I8 index) override;
    bool onSDLInputEvent(SDL_Event event) override;
        
  protected:
    bool renderMinimal(const U64 deltaTime);
    bool renderFull(const U64 deltaTime);
    void updateMousePosAndButtons();

  protected: // attorney
    void renderDrawList(ImDrawData* pDrawData, bool overlayOnScene, I64 windowGUID);
    void drawMenuBar();
    void showSampleWindow(bool state);
    bool showSampleWindow() const;
    void setSelectedCamera(Camera* camera);
    Camera* getSelectedCamera() const;
    bool hasUnsavedElements() const;
    void saveElement(I64 elementGUID);

  private:
    ImGuiStyleEnum _currentTheme;
    ImGuiStyleEnum _currentDimmedTheme;

    std::unique_ptr<MenuBar> _menuBar;
    std::unique_ptr<Gizmo> _gizmo;

    bool              _running;
    bool              _sceneHovered;
    bool              _scenePreviewFocused;
    bool              _showSampleWindow;
    Camera*           _selectedCamera;
    DisplayWindow*    _mainWindow;
    ImGuiContext*     _imguiContext;
    Texture_ptr       _fontTexture;
    ShaderProgram_ptr _imguiProgram;
    Time::ProfileTimer& _editorUpdateTimer;
    Time::ProfileTimer& _editorRenderTimer;

    std::vector<I64> _unsavedElements;
    std::array<DockedWindow*, to_base(WindowType::COUNT)> _dockedWindows;
}; //Editor

namespace Attorney {
    class EditorGizmo {
    private:
        static void renderDrawList(Editor& editor, ImDrawData* pDrawData, bool overlayOnScene, I64 windowGUID) {
            editor.renderDrawList(pDrawData, overlayOnScene, windowGUID);
        }
        friend class Divide::Gizmo;
    };

    class EditorSceneViewWindow {
    private:
        static bool editorEnabledGizmo(Editor& editor) {
            return editor._gizmo->enabled();
        }

        static void editorEnableGizmo(Editor& editor, bool state) {
            editor._gizmo->enable(state);
        }

        friend class Divide::SceneViewWindow;
    };

    class EditorSolutionExplorerWindow {
    private :
        static void setSelectedCamera(Editor& editor, Camera* camera) {
            editor.setSelectedCamera(camera);
        }

        static Camera* getSelectedCamera(Editor& editor) {
            return editor.getSelectedCamera();
        }

        static bool editorEnableGizmo(Editor& editor) {
            return editor._gizmo->enabled();
        }

        static void editorEnableGizmo(Editor& editor, bool state) {
            editor._gizmo->enable(state);
        }

        friend class Divide::SolutionExplorerWindow;
    };

    class EditorPropertyWindow {
    private :

        static void setSelectedCamera(Editor& editor, Camera* camera) {
            editor.setSelectedCamera(camera);
        }

        static Camera* getSelectedCamera(Editor& editor) {
            return editor.getSelectedCamera();
        }

        friend class Divide::PropertyWindow;
    };

    class EditorPanelManager {
        //private:
        public: //ToDo: fix this -Ionut
        static void setTransformSettings(Editor& editor, const TransformSettings& settings) {
            editor.setTransformSettings(settings);
        }

        static const TransformSettings& getTransformSettings(const Editor& editor) {
            return editor.getTransformSettings();
        }
        static void showSampleWindow(Editor& editor, bool state) {
            editor.showSampleWindow(state);
        }
        static void enableGizmo(Editor& editor, bool state) {
            return editor._gizmo->enable(state);
        }
        static bool showSampleWindow(const Editor& editor) {
            return editor.showSampleWindow();
        }
        static bool enableGizmo(const Editor& editor) {
            return editor._gizmo->enabled();
        }
        static bool hasUnsavedElements(const Editor& editor) {
            return editor.hasUnsavedElements();
        }
        static void saveElement(Editor& editor, I64 elementGUID = -1) {
            editor.saveElement(elementGUID);
        }

        friend class Divide::MenuBar;
        friend class Divide::PanelManager;
    };
};


}; //namespace Divide

#endif //_DIVIDE_EDITOR_H_