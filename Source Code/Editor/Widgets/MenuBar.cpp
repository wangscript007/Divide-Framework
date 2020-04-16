#include "stdafx.h"

#include "Headers/MenuBar.h"

#include "Editor/Headers/Editor.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

#include "Managers/Headers/SceneManager.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/PostFX/Headers/PreRenderOperator.h"

#include <imgui/addons/imguifilesystem/imguifilesystem.h>

namespace Divide {
namespace {
    const stringImpl s_messages[] = {
        "Please wait while saving current scene! App may appear frozen or stuttery for up to 30 seconds ...",
        "Saved scene succesfully",
        "Failed to save the current scene"
    };

    struct SaveSceneParams {
        stringImpl _saveMessage = "";
        U32 _saveElementCount = 0u;
        U32 _saveProgress = 0u;
        bool _closePopup = false;
    } g_saveSceneParams;

    const char* UsageToString(RenderTargetUsage usage) {
        switch (usage) {
            case RenderTargetUsage::EDITOR: return "Editor";
            case RenderTargetUsage::ENVIRONMENT: return "Environment";
            case RenderTargetUsage::HI_Z: return "HI-Z";
            case RenderTargetUsage::HI_Z_REFLECT: return "HI-Z Reflect";
            case RenderTargetUsage::OIT: return "OIT";
            case RenderTargetUsage::OIT_MS: return "OIT_MS";
            case RenderTargetUsage::OIT_REFLECT: return "OIT_REFLECT";
            case RenderTargetUsage::OTHER: return "Other";
            case RenderTargetUsage::REFLECTION_CUBE: return "Cube Reflection";
            case RenderTargetUsage::REFLECTION_PLANAR: return "Planar Reflection";
            case RenderTargetUsage::REFLECTION_PLANAR_BLUR: return "Planar Reflection Blur";
            case RenderTargetUsage::REFRACTION_PLANAR: return "Planar Refraction";
            case RenderTargetUsage::SCREEN: return "Screen";
            case RenderTargetUsage::SCREEN_MS: return "Screen_MS";
            case RenderTargetUsage::SHADOW: return "Shadow";
        };

        return "Unknown";
    }

    const char* EdgeMethodName(PreRenderBatch::EdgeDetectionMethod method) {
        switch (method) {
            case PreRenderBatch::EdgeDetectionMethod::Depth: return "Depth";
            case PreRenderBatch::EdgeDetectionMethod::Luma: return "Luma";
            case PreRenderBatch::EdgeDetectionMethod::Colour: return "Colour";
        };
        return "Unknown";
    }
};


MenuBar::MenuBar(PlatformContext& context, bool mainMenu)
    : PlatformContextComponent(context),
      _isMainMenu(mainMenu),
      _quitPopup(false),
      _closePopup(false)
{
}

MenuBar::~MenuBar()
{
}

void MenuBar::draw() {
    if ((ImGui::BeginMenuBar()))
    {
        drawFileMenu();
        drawEditMenu();
        drawProjectMenu();
        drawObjectMenu();
        drawToolsMenu();
        drawWindowsMenu();
        drawPostFXMenu();
        drawDebugMenu();
        drawHelpMenu();

        ImGui::EndMenuBar();

       for (vectorSTD<Texture_ptr>::iterator it = std::begin(_previewTextures); it != std::end(_previewTextures); ) {
            if (Attorney::EditorGeneralWidget::modalTextureView(_context.editor(), Util::StringFormat("Image Preview: %s", (*it)->resourceName().c_str()).c_str(), *it, vec2<F32>(512, 512), true, false)) {
                it = _previewTextures.erase(it);
            } else {
                ++it;
            }
        }

        if (_closePopup) {
            ImGui::OpenPopup("Confirm Close");

            if (ImGui::BeginPopupModal("Confirm Close", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Are you sure you want to close the editor? You have unsaved items!");
                ImGui::Separator();

                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    _closePopup = false;
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Yes", ImVec2(120, 0))) {

                    ImGui::CloseCurrentPopup();
                    _closePopup = false;
                    _context.editor().toggle(false);
                }
                ImGui::EndPopup();
            }
        }

        if (!_errorMsg.empty()) {
            ImGui::OpenPopup("Error!");
            if (ImGui::BeginPopupModal("Error!", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiViewportFlags_TopMost)) {
                ImGui::Text(_errorMsg.c_str());
                if (ImGui::Button("Ok")) {
                    ImGui::CloseCurrentPopup();
                    _errorMsg.clear();
                }
                ImGui::EndPopup();
            }
        }

        if (_quitPopup) {
            ImGui::OpenPopup("Confirm Quit");
            if (ImGui::BeginPopupModal("Confirm Quit", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiViewportFlags_TopMost)) {
                ImGui::Text("Are you sure you want to quit?");
                ImGui::Separator();

                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    _quitPopup = false;
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Yes", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    _quitPopup = false;
                    context().app().RequestShutdown();
                }
                ImGui::EndPopup();
            }
        }
        if (_savePopup) {
            ImGui::OpenPopup("Saving Scene");
            if (ImGui::BeginPopupModal("Saving Scene", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiViewportFlags_TopMost))
            {
                constexpr U32 maxSize = 40u;
                const U32 ident = MAP(g_saveSceneParams._saveProgress, 0u, g_saveSceneParams._saveElementCount, 0u, maxSize - 5u /*overestimate a bit*/);

                ImGui::Text("Saving Scene!\n\n%s", g_saveSceneParams._saveMessage.c_str());
                ImGui::Separator();

                stringImpl progress = "";
                for (U32 i = 0; i < maxSize; ++i) {
                    progress.append(i < ident ? "=" : " ");
                }
                ImGui::Text("[%s]", progress.c_str());
                ImGui::Separator();

                if (g_saveSceneParams._closePopup) {
                    _savePopup = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
    }
}

void MenuBar::drawFileMenu() {
    bool showFileOpenDialog = false;

    if (ImGui::BeginMenu("File"))
    {
        const bool hasUnsavedElements = Attorney::EditorGeneralWidget::hasUnsavedSceneChanges(_context.editor());

        if (ImGui::MenuItem("New", "Ctrl+N", false, false))
        {
        }

        showFileOpenDialog = ImGui::MenuItem("Open", "Ctrl+O");
        if (ImGui::BeginMenu("Open Recent"))
        {
            ImGui::Text("Empty");
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem(hasUnsavedElements ? "Save Scene*" : "Save Scene")) {
            _savePopup = true;
            g_saveSceneParams._closePopup = false;
            g_saveSceneParams._saveProgress = 0u;
            g_saveSceneParams._saveElementCount = Attorney::EditorGeneralWidget::saveItemCount(_context.editor());

            const auto messageCbk = [](const char* msg) {
                g_saveSceneParams._saveMessage = msg;
                ++g_saveSceneParams._saveProgress;
            };

            const auto closeDialog = [this](bool success) {
                Attorney::EditorGeneralWidget::showStatusMessage(_context.editor(), s_messages[success ? 1 : 2], Time::SecondsToMilliseconds<F32>(6));
                g_saveSceneParams._closePopup = true;
            };

            Attorney::EditorGeneralWidget::showStatusMessage(_context.editor(), s_messages[0], Time::SecondsToMilliseconds<F32>(6));
            if (!Attorney::EditorGeneralWidget::saveSceneChanges(_context.editor(), messageCbk, closeDialog)) {
                _errorMsg.append("Error occured while saving the current scene!\n Try again or check the logs for errors!\n");
            }
        }

        ImGui::Separator();
        if (ImGui::BeginMenu("Options"))
        {
            GFXDevice& gfx = _context.gfx();
            const Configuration& config = _context.config();

            if (ImGui::BeginMenu("MSAA"))
            {
                const U8 maxMSAASamples = gfx.gpuState().maxMSAASampleCount();
                for (U8 i = 0; (1 << i) <= maxMSAASamples; ++i) {
                    const U8 sampleCount = i == 0u ? 0u : 1 << i;
                    if (sampleCount % 2 == 0) {
                        bool msaaEntryEnabled = config.rendering.MSAAsamples == sampleCount;
                        if (ImGui::MenuItem(Util::StringFormat("%dx", to_U32(sampleCount)).c_str(), "", &msaaEntryEnabled))
                        {
                            gfx.setScreenMSAASampleCount(sampleCount);
                        }
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("CSM MSAA"))
            {
                const U8 maxMSAASamples = gfx.gpuState().maxMSAASampleCount();
                for (U8 i = 0; (1 << i) <= maxMSAASamples; ++i) {
                    const U8 sampleCount = i == 0u ? 0u : 1 << i;
                    if (sampleCount % 2 == 0) {
                        bool msaaEntryEnabled = config.rendering.shadowMapping.MSAAsamples == sampleCount;
                        if (ImGui::MenuItem(Util::StringFormat("%dx", to_U32(sampleCount)).c_str(), "", &msaaEntryEnabled))
                        {
                            gfx.setShadowMSAASampleCount(sampleCount);
                        }
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Close Editor"))
        {
            if (hasUnsavedElements) {
                _closePopup = true;
            } else {
                _context.editor().toggle(false);
            }
        }

        if (ImGui::MenuItem("Quit", "Alt+F4"))
        {
            _quitPopup = true;
        }

        ImGui::EndMenu();
    }

    static ImGuiFs::Dialog s_fileOpenDialog(true, true);
    s_fileOpenDialog.chooseFileDialog(showFileOpenDialog);

    if (strlen(s_fileOpenDialog.getChosenPath()) > 0) {
        ImGui::Text("Chosen file: \"%s\"", s_fileOpenDialog.getChosenPath());
    }
}

void MenuBar::drawEditMenu() {
    if (ImGui::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Undo", "CTRL+Z"))
        {
            _context.editor().Undo();
        }

        if (ImGui::MenuItem("Redo", "CTRL+Y"))
        {
            _context.editor().Redo();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Cut", "CTRL+X", false, false))
        {
        }

        if (ImGui::MenuItem("Copy", "CTRL+C", false, false))
        {
        }

        if (ImGui::MenuItem("Paste", "CTRL+V", false, false))
        {
        }

        ImGui::Separator();
        ImGui::EndMenu();
    }
}

void MenuBar::drawProjectMenu() {
    if (ImGui::BeginMenu("Project"))
    {
        if(ImGui::MenuItem("Configuration", "", false, false))
        {
        }

        ImGui::EndMenu();
    }
}
void MenuBar::drawObjectMenu() {
    if (ImGui::BeginMenu("Object"))
    {
        if(ImGui::MenuItem("New Node", "", false, false))
        {
        }

        ImGui::EndMenu();
    }

}
void MenuBar::drawToolsMenu() {
    if (ImGui::BeginMenu("Tools"))
    {
        bool memEditorEnabled = Attorney::EditorMenuBar::memoryEditorEnabled(_context.editor());
        if (ImGui::MenuItem("Memory Editor", NULL, memEditorEnabled)) {
            Attorney::EditorMenuBar::toggleMemoryEditor(_context.editor(), !memEditorEnabled);
            _context.editor().saveToXML();
        }

        if (ImGui::BeginMenu("Render Targets"))
        {
            const GFXRTPool& pool = _context.gfx().renderTargetPool();
            for (U8 i = 0; i < to_U8(RenderTargetUsage::COUNT); ++i) {
                const RenderTargetUsage usage = static_cast<RenderTargetUsage>(i);
                const auto& rTargets = pool.renderTargets(usage);

                if (rTargets.empty()) {
                    if (ImGui::MenuItem(UsageToString(usage), "", false, false))
                    {
                    }
                } else {
                    if (ImGui::BeginMenu(UsageToString(usage)))
                    {
                        for (const auto& rt : rTargets) {
                            if (rt == nullptr) {
                                continue;
                            }

                            if (ImGui::BeginMenu(rt->name().c_str()))
                            {
                                for (U8 j = 0; j < to_U8(RTAttachmentType::COUNT); ++j) {
                                    const RTAttachmentType type = static_cast<RTAttachmentType>(j);
                                    const U8 count = rt->getAttachmentCount(type);

                                    for (U8 k = 0; k < count; ++k) {
                                        const RTAttachment& attachment = rt->getAttachment(type, k);
                                        const Texture_ptr& tex = attachment.texture();
                                        if (tex == nullptr) {
                                            continue;
                                        }

                                        if (ImGui::MenuItem(tex->resourceName().c_str()))
                                        {
                                            _previewTextures.push_back(tex);
                                        }
                                    }
                                }

                                ImGui::EndMenu();
                            }
                        }
                        ImGui::EndMenu();
                    }
                }
            }

            const Texture_ptr& prevDepthBufferTex = _context.gfx().getPrevDepthBuffer();
            if (prevDepthBufferTex != nullptr) {
                if (ImGui::BeginMenu("Miscellaneous"))
                {
                    if (ImGui::MenuItem(prevDepthBufferTex->resourceName().c_str()))
                    {
                        _previewTextures.push_back(prevDepthBufferTex);
                    }
                    ImGui::EndMenu();
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
}

void MenuBar::drawWindowsMenu() {
    if (ImGui::BeginMenu("Window"))
    {
        bool& sampleWindowEnabled = Attorney::EditorMenuBar::sampleWindowEnabled(_context.editor());
        if (ImGui::MenuItem("Sample Window", NULL, &sampleWindowEnabled)) {
            
        }
        ImGui::EndMenu();
    }

}

void MenuBar::drawPostFXMenu() {
    if (ImGui::BeginMenu("PostFX"))
    {
        PostFX& postFX = _context.gfx().getRenderer().postFX();
        for (U16 i = 1; i < to_base(FilterType::FILTER_COUNT); ++i) {
            const FilterType f = static_cast<FilterType>(toBit(i));

            const bool filterEnabled = postFX.getFilterState(f);
            if (ImGui::MenuItem(PostFX::FilterName(f), NULL, &filterEnabled))
            {
                if (filterEnabled) {
                    postFX.pushFilter(f, true);
                } else {
                    postFX.popFilter(f, true);
                }
            }
        }
        ImGui::EndMenu();
    }
}

void MenuBar::drawDebugMenu() {
    if (ImGui::BeginMenu("Debug"))
    {
        GFXDevice& gfx = _context.gfx();
        Configuration& config = _context.config();

        if (ImGui::BeginMenu("BRDF Settings")) {
            const GFXDevice::MaterialDebugFlag debugFlag = _context.gfx().materialDebugFlag();
            bool debug = debugFlag == GFXDevice::MaterialDebugFlag::DEBUG_ALBEDO;
            if (ImGui::MenuItem("Debug albedo", "", &debug)) {
                _context.gfx().materialDebugFlag(debug ? GFXDevice::MaterialDebugFlag::DEBUG_ALBEDO : GFXDevice::MaterialDebugFlag::COUNT);
            }
            debug = debugFlag == GFXDevice::MaterialDebugFlag::DEBUG_EMISSIVE;
            if (ImGui::MenuItem("Debug emissive", "", &debug)) {
                _context.gfx().materialDebugFlag(debug ? GFXDevice::MaterialDebugFlag::DEBUG_EMISSIVE : GFXDevice::MaterialDebugFlag::COUNT);
            }
            debug = debugFlag == GFXDevice::MaterialDebugFlag::DEBUG_ROUGHNESS;
            if (ImGui::MenuItem("Debug roughness", "", &debug)) {
                _context.gfx().materialDebugFlag(debug ? GFXDevice::MaterialDebugFlag::DEBUG_ROUGHNESS : GFXDevice::MaterialDebugFlag::COUNT);
            }
            debug = debugFlag == GFXDevice::MaterialDebugFlag::DEBUG_METALLIC;
            if (ImGui::MenuItem("Debug metallic", "", &debug)) {
                _context.gfx().materialDebugFlag(debug ? GFXDevice::MaterialDebugFlag::DEBUG_METALLIC : GFXDevice::MaterialDebugFlag::COUNT);
            }
            debug = debugFlag == GFXDevice::MaterialDebugFlag::DEBUG_NORMALS;
            if (ImGui::MenuItem("Debug normals", "", &debug)) {
                _context.gfx().materialDebugFlag(debug ? GFXDevice::MaterialDebugFlag::DEBUG_NORMALS : GFXDevice::MaterialDebugFlag::COUNT);
            }
            debug = debugFlag == GFXDevice::MaterialDebugFlag::DEBUG_SHADOW_MAPS;
            if (ImGui::MenuItem("Debug shadow maps", "", &debug)) {
                _context.gfx().materialDebugFlag(debug ? GFXDevice::MaterialDebugFlag::DEBUG_SHADOW_MAPS : GFXDevice::MaterialDebugFlag::COUNT);
            }
            debug = debugFlag == GFXDevice::MaterialDebugFlag::DEBUG_LIGHT_TILES;
            if (ImGui::MenuItem("Debug Fwd+ Tiles", "", &debug)) {
                _context.gfx().materialDebugFlag(debug ? GFXDevice::MaterialDebugFlag::DEBUG_LIGHT_TILES : GFXDevice::MaterialDebugFlag::COUNT);
            }
            ImGui::EndMenu();
        }

        LightPool& pool = Attorney::EditorGeneralWidget::getActiveLightPool(_context.editor());
        if (ImGui::BeginMenu("Toggle Light Types")) {
            for (U8 i = 0; i < to_U8(LightType::COUNT); ++i) {
                const LightType type = static_cast<LightType>(i);

                bool state = pool.lightTypeEnabled(type);
                if (ImGui::MenuItem(TypeUtil::LightTypeToString(type), "", &state)) {
                    pool.toggleLightType(type, state);
                }
            }
            ImGui::EndMenu();
        }

        bool& showCSMSplits = config.debug.showShadowCascadeSplits;
        if (ImGui::MenuItem("Enable CSM Split View", "", &showCSMSplits))
        {
            config.changed(true);
        }

        bool shadowDebug = pool.isDebugLight(LightType::DIRECTIONAL, 0);
        if (ImGui::MenuItem("Debug Main CSM", "", &shadowDebug))
        {
            pool.setDebugLight(shadowDebug ? LightType::DIRECTIONAL : LightType::COUNT, 0u);
        }

        bool lightImpostors = pool.lightImpostorsEnabled();
        if (ImGui::MenuItem("Draw Light Impostors", "", &lightImpostors))
        {
            pool.lightImpostorsEnabled(lightImpostors);
        }

        if (ImGui::BeginMenu("Debug Gizmos")) {
            SceneManager* sceneManager = context().kernel().sceneManager();
            SceneRenderState& renderState = sceneManager->getActiveScene().state().renderState();

            bool temp = renderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_AABB);
            if (ImGui::MenuItem("Show AABBs", "", &temp)) {
                Console::d_printfn(Locale::get(_ID("TOGGLE_SCENE_BOUNDING_BOXES")), (temp ? "On" : "Off"));
                renderState.toggleOption(SceneRenderState::RenderOptions::RENDER_AABB, temp);
            }
            temp = renderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_BSPHERES);
            if (ImGui::MenuItem("Show bounding spheres", "", &temp)) {
                Console::d_printfn(Locale::get(_ID("TOGGLE_SCENE_BOUNDING_SPHERES")), (temp ? "On" : "Off"));
                renderState.toggleOption(SceneRenderState::RenderOptions::RENDER_BSPHERES, temp);
            }
            temp = renderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_SKELETONS);
            if (ImGui::MenuItem("Show skeletons", "", &temp)) {
                Console::d_printfn(Locale::get(_ID("TOGGLE_SCENE_SKELETONS")), (temp ? "On" : "Off"));
                renderState.toggleOption(SceneRenderState::RenderOptions::RENDER_SKELETONS, temp);
            }
            temp = renderState.isEnabledOption(SceneRenderState::RenderOptions::SCENE_GIZMO);
            if (ImGui::MenuItem("Show scene axis", "", &temp)) {
                Console::d_printfn(Locale::get(_ID("TOGGLE_SCENE_AXIS_GIZMO")));
                renderState.toggleOption(SceneRenderState::RenderOptions::SCENE_GIZMO, temp);
            }
            temp = renderState.isEnabledOption(SceneRenderState::RenderOptions::SELECTION_GIZMO);
            if (ImGui::MenuItem("Show selection axis", "", &temp)) {
                renderState.toggleOption(SceneRenderState::RenderOptions::SELECTION_GIZMO, temp);
            }
            temp = renderState.isEnabledOption(SceneRenderState::RenderOptions::ALL_GIZMOS);
            if (ImGui::MenuItem("Show all axis", "", &temp)) {
                renderState.toggleOption(SceneRenderState::RenderOptions::ALL_GIZMOS, temp);
            }
            temp = renderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY);
            if (ImGui::MenuItem("Render geometry", "", &temp)) {
                renderState.toggleOption(SceneRenderState::RenderOptions::RENDER_GEOMETRY, temp);
            }
            temp = renderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_WIREFRAME);
            if (ImGui::MenuItem("Render wireframe", "", &temp)) {
                renderState.toggleOption(SceneRenderState::RenderOptions::RENDER_WIREFRAME, temp);
            }
            temp = renderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_OCTREE_REGIONS);
            if (ImGui::MenuItem("Render octree regions", "", &temp)) {
                renderState.toggleOption(SceneRenderState::RenderOptions::RENDER_OCTREE_REGIONS, temp);
            }
            temp = renderState.isEnabledOption(SceneRenderState::RenderOptions::RENDER_CUSTOM_PRIMITIVES);
            if (ImGui::MenuItem("Render custom gismos", "", &temp)) {
                renderState.toggleOption(SceneRenderState::RenderOptions::RENDER_CUSTOM_PRIMITIVES, temp);
            }
            temp = renderState.isEnabledOption(SceneRenderState::RenderOptions::PLAY_ANIMATIONS);
            if (ImGui::MenuItem("Play animations", "", &temp)) {
                renderState.toggleOption(SceneRenderState::RenderOptions::PLAY_ANIMATIONS, temp);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edge Detection Method")) {

            PreRenderBatch* batch = _context.gfx().getRenderer().postFX().getFilterBatch();
            bool noneSelected = batch->edgeDetectionMethod() == PreRenderBatch::EdgeDetectionMethod::COUNT;
            if (ImGui::MenuItem("None", "", &noneSelected)) {
                batch->edgeDetectionMethod(PreRenderBatch::EdgeDetectionMethod::COUNT);
            }

            for (U8 i = 0; i < to_U8(PreRenderBatch::EdgeDetectionMethod::COUNT) + 1; ++i) {
                PreRenderBatch::EdgeDetectionMethod method = static_cast<PreRenderBatch::EdgeDetectionMethod>(i);

                bool selected = batch->edgeDetectionMethod() == method;
                if (ImGui::MenuItem(EdgeMethodName(method), "", &selected)) {
                    batch->edgeDetectionMethod(method);
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug Views"))
        {
            vectorEASTL<std::tuple<stringImpl, I16, bool>> viewNames = {};
            gfx.getDebugViewNames(viewNames);

            for (auto[name, index, enabled] : viewNames) {
                if (ImGui::MenuItem(name.c_str(), "", &enabled))
                {
                    gfx.toggleDebugView(index, enabled);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
}

void MenuBar::drawHelpMenu() {
    if (ImGui::BeginMenu("Help"))
    {
        ImGui::Text("Copyright(c) 2018 DIVIDE - Studio");
        ImGui::Text("Copyright(c) 2009 Ionut Cava");
        ImGui::EndMenu();
    }
}
}; //namespace Divide