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
#ifndef _EDITOR_GIZMO_H_
#define _EDITOR_GIZMO_H_

#include "Platform/Input/Headers/InputAggregatorInterface.h"

#include <imgui/addons/imguigizmo/ImGuizmo.h>

struct ImGuiContext;

namespace Divide {
    class Editor;
    class Camera;
    class DisplayWindow;
    class SceneGraphNode;

    struct SizeChangeParams;

    namespace Attorney {
        class GizmoEditor;
    };

    struct TransformSettings {
        ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::ROTATE;
        ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;
        bool useSnap = false;
        F32 snap[3] = { 1.f, 1.f, 1.f };
    };

    class Gizmo : public Input::InputAggregatorInterface {
        friend class Attorney::GizmoEditor;

    public:
        explicit Gizmo(Editor& parent, ImGuiContext* mainContext, DisplayWindow* mainWindow);
        ~Gizmo();

        ImGuiContext& getContext();
        const ImGuiContext& getContext() const;

        bool isOver() const;
        bool isUsing() const;
        void enable(bool state);
        bool enabled() const;

    public: //Input
        /// Key pressed: return true if input was consumed
        bool onKeyDown(const Input::KeyEvent& key) override;
        /// Key released: return true if input was consumed
        bool onKeyUp(const Input::KeyEvent& key) override;
        /// Mouse moved: return true if input was consumed
        bool mouseMoved(const Input::MouseEvent& arg) override;
        /// Mouse button pressed: return true if input was consumed
        bool mouseButtonPressed(const Input::MouseEvent& arg, Input::MouseButton button) override;
        /// Mouse button released: return true if input was consumed
        bool mouseButtonReleased(const Input::MouseEvent& arg, Input::MouseButton button) override;

        bool joystickButtonPressed(const Input::JoystickEvent &arg, Input::JoystickButton button) override;
        bool joystickButtonReleased(const Input::JoystickEvent &arg, Input::JoystickButton button) override;
        bool joystickAxisMoved(const Input::JoystickEvent &arg, I8 axis) override;
        bool joystickPovMoved(const Input::JoystickEvent &arg, I8 pov) override;
        bool joystickSliderMoved(const Input::JoystickEvent &arg, I8 index) override;
        bool joystickvector3Moved(const Input::JoystickEvent &arg, I8 index) override;
        bool onSDLInputEvent(SDL_Event event) override;

    protected:
        void update(const U64 deltaTimeUS);
        void render(const Camera& camera);
        void updateSelections(SceneGraphNode* node);
        void setTransformSettings(const TransformSettings& settings);
        const TransformSettings& getTransformSettings() const;
        void onSizeChange(const SizeChangeParams& params, const vec2<U16>& displaySize);

    private:
        Editor& _parent;
        bool _enabled = false;
        bool _visible = false;
        vector<SceneGraphNode*> _selectedNodes;
        ImGuiContext* _imguiContext = nullptr;
        TransformSettings _transformSettings;
    };

    namespace Attorney {
        class GizmoEditor {
        private:
            static void render(Gizmo& gizmo, const Camera& camera) {
                gizmo.render(camera);
            }

            static void updateSelection(Gizmo& gizmo, SceneGraphNode* node) {
                gizmo.updateSelections(node);
            }

            static void update(Gizmo& gizmo, const U64 deltaTimeUS) {
                gizmo.update(deltaTimeUS);
            }

            static void setTransformSettings(Gizmo& gizmo, const TransformSettings& settings) {
                gizmo.setTransformSettings(settings);
            }

            static const TransformSettings& getTransformSettings(Gizmo& gizmo) {
                return gizmo.getTransformSettings();
            }

            static void onSizeChange(Gizmo& gizmo, const SizeChangeParams& params, const vec2<U16>& displaySize) {
                gizmo.onSizeChange(params, displaySize);
            }
            friend class Divide::Editor;
        };
    };
}; //namespace Divide

#endif //_EDITOR_GIZMO_H_