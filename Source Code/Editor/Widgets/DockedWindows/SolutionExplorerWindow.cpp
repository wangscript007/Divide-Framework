#include "stdafx.h"

#include "Headers/SolutionExplorerWindow.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Managers/Headers/SceneManager.h"

namespace Divide {
    namespace {
        boost::circular_buffer<F32> _framerateBuffer(256);
    };

    SolutionExplorerWindow::SolutionExplorerWindow(PanelManager& parent, PlatformContext& context)
        : DockedWindow(parent, "Solution Explorer"),
          PlatformContextComponent(context)
    {

    }

    SolutionExplorerWindow::~SolutionExplorerWindow()
    {

    }

    void printSceneGraphNode(Scene& activeScene, SceneGraphNode& sgn) {
        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        if (sgn.getSelectionFlag() == SceneGraphNode::SelectionFlag::SELECTION_SELECTED) {
            node_flags |= ImGuiTreeNodeFlags_Selected;
        }

        if (!sgn.hasChildren()) {
            node_flags |= ImGuiTreeNodeFlags_Leaf /*| ImGuiTreeNodeFlags_NoTreePushOnOpen*/; // ImGuiTreeNodeFlags_Bullet
        }
        bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)sgn.getGUID(), node_flags, sgn.getName().c_str());
        if (ImGui::IsItemClicked()) {
            activeScene.resetSelection(0);
            activeScene.setSelected(0, sgn);
        }
        if (node_open) {
            sgn.forEachChild([&activeScene](SceneGraphNode& child) {
                printSceneGraphNode(activeScene, child);
            });
            ImGui::TreePop();
        }
        
        
    }

    void SolutionExplorerWindow::draw() {
        //show_test_window ^= ImGui::Button("Test Window");
        //show_another_window ^= ImGui::Button("Another Window");

        SceneManager& sceneManager = context().kernel().sceneManager();
        Scene& activeScene = sceneManager.getActiveScene();
        SceneGraphNode& root = activeScene.sceneGraph().getRoot();
        
        if (ImGui::TreeNode(activeScene.getName().c_str()))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, ImGui::GetFontSize() * 3); // Increase spacing to differentiate leaves from expanded contents.
            printSceneGraphNode(activeScene, root);
            ImGui::PopStyleVar();
            ImGui::TreePop();
        }

        const vectorImpl<stringImpl>& scenes = sceneManager.sceneNameList();
        for (const stringImpl& scene : scenes) {
            if (scene != activeScene.getName()) {
                if (ImGui::TreeNodeEx(scene.c_str(), ImGuiTreeNodeFlags_Leaf)) {
                    ImGui::TreePop();
                }
            }
        }



        // Calculate and show framerate
        static F32 ms_per_frame[120] = { 0 };
        static I32 ms_per_frame_idx = 0;
        static F32 ms_per_frame_accum = 0.0f;
        ms_per_frame_accum -= ms_per_frame[ms_per_frame_idx];
        ms_per_frame[ms_per_frame_idx] = ImGui::GetIO().DeltaTime * 1000.0f;
        ms_per_frame_accum += ms_per_frame[ms_per_frame_idx];
        ms_per_frame_idx = (ms_per_frame_idx + 1) % 120;
        const F32 ms_per_frame_avg = ms_per_frame_accum / 120;
        _framerateBuffer.push_back(ms_per_frame_avg);

        vectorImpl<F32> data(_framerateBuffer.begin(), _framerateBuffer.end());
        ImGui::PlotHistogram("ms/frame", data.data(), to_I32(data.size()), 0, NULL, 0.0f, 1.0f, ImVec2(0, 80));

        ImGui::Text("%.3f ms/frame (%.1f FPS)", ms_per_frame_avg, 1000.0f / ms_per_frame_avg);

    }
};