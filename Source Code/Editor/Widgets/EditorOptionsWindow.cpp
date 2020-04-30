#include "stdafx.h"

#include "Headers/UndoManager.h"

#include "Editor/Headers/Editor.h"
#include "Headers/EditorOptionsWindow.h"
#include "Core/Headers/PlatformContext.h"

namespace Divide {
    EditorOptionsWindow::EditorOptionsWindow(PlatformContext& context)
        : PlatformContextComponent(context),
          _fileOpenDialog(false, true)
    {
        //ImGuiFs::Dialog::ExtraWindowFlags |= ImGuiWindowFlags_Tooltip;
    }

    EditorOptionsWindow::~EditorOptionsWindow()
    {
    }

    void EditorOptionsWindow::update(const U64 deltaTimeUS) {
        ACKNOWLEDGE_UNUSED(deltaTimeUS);
    }

    void EditorOptionsWindow::draw(bool& open) {
        if (!open) {
            return;
        }
        const ImVec2 CenterPos(ImGui::GetMainViewport()->Pos.x + ImGui::GetIO().DisplaySize.x * 0.5f,
                               ImGui::GetMainViewport()->Pos.y + ImGui::GetIO().DisplaySize.y * 0.5f);
        const ImVec2 FileDialogPos(ImGui::GetMainViewport()->Pos.x + 20, 
                                   ImGui::GetMainViewport()->Pos.y + 20);

        ImGui::SetNextWindowPos(CenterPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        U32 flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking;
        if (!_openDialog) {
            flags |= ImGuiWindowFlags_Tooltip;
        } else {
            flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
        }

        if (!ImGui::Begin("Editor options", NULL, flags))
        {
            ImGui::End();
            return;
        }
        
        static UndoEntry<I32> undo = {};
        const I32 crtThemeIdx = to_I32(Attorney::EditorOptionsWindow::getTheme(_context.editor()));
        I32 selection = crtThemeIdx;
        bool openDialog = false;
        if (ImGui::Combo("Editor Theme", &selection, ImGui::GetDefaultStyleNames(), ImGuiStyle_Count)) {

            ImGui::ResetStyle(static_cast<ImGuiStyleEnum>(selection));
            Attorney::EditorOptionsWindow::setTheme(_context.editor(), static_cast<ImGuiStyleEnum>(selection));

            undo._type = GFX::PushConstantType::INT;
            undo._name = "Theme Selection";
            undo.oldVal = crtThemeIdx;
            undo.newVal = selection;
            undo._dataSetter = [this](const void* data) {
                const ImGuiStyleEnum style = *(ImGuiStyleEnum*)data;
                ImGui::ResetStyle(style);
                Attorney::EditorOptionsWindow::setTheme(_context.editor(), style);
            };
            _context.editor().registerUndoEntry(undo);
            ++_changeCount;
        }

        ImGui::Separator();

        const stringImpl& externalTextEditorPath = Attorney::EditorOptionsWindow::externalTextEditorPath(_context.editor());
        ImGui::InputText("Text Editor", const_cast<char*>(externalTextEditorPath.c_str()), externalTextEditorPath.size(), ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();
        openDialog = ImGui::Button("Select");
        if (openDialog) {
            ImGuiFs::Dialog::WindowLTRBOffsets.x = FileDialogPos.x;
            ImGuiFs::Dialog::WindowLTRBOffsets.y = FileDialogPos.y;
            _openDialog = true;
        }
        ImGui::Separator();

        ImGui::Separator();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            open = false;
            for (U16 i = 0; i < _changeCount; ++i) {
                _context.editor().Undo();
            }
            _changeCount = 0u;
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Save", ImVec2(120, 0))) {
            open = false;
            _changeCount = 0u;
            _context.editor().saveToXML();
        }
        ImGui::SameLine();
        if (ImGui::Button("Defaults", ImVec2(120, 0))) {
            _changeCount = 0u;
            Attorney::EditorOptionsWindow::setTheme(_context.editor(), ImGuiStyleEnum::ImGuiStyle_DarkCodz01);
            ImGui::ResetStyle(ImGuiStyleEnum::ImGuiStyle_DarkCodz01);
        }
        ImGui::End();

        if (_openDialog) {
            const char* chosenPath = _fileOpenDialog.chooseFileDialog(openDialog, NULL, NULL, "Choose text editor", ImVec2(-1, -1), FileDialogPos);
            if (strlen(chosenPath) > 0) {
                Attorney::EditorOptionsWindow::externalTextEditorPath(_context.editor(), chosenPath);
            }
            if (_fileOpenDialog.hasUserJustCancelledDialog()) {
                _openDialog = false;
            }
        };
    }
};