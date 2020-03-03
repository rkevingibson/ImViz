#include <iostream>

#include "Application.h"
#include "imgui.h"
#include "Im3D.h"
#include "imgui_internal.h"


struct AppData
{
    View3d* view3d{ nullptr };
};

void testLoop(const AppContext* ctx, void* userData)
{
    AppData* appData = (AppData*)userData;
    if (appData->view3d == nullptr)
    {
        appData->view3d = new View3d(ImVec2(1200, 800));
    }

    ImGuiID dockspaceId = ImGui::GetID("MyDockspace");

    // Dock space stuff.
    auto* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetWorkPos());
    ImGui::SetNextWindowSize(viewport->GetWorkSize());
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    //windowFlags |= ImGuiWindowFlags_NoBackground;

    ImGui::Begin("DockSpace Demo", nullptr, windowFlags);
    ImGui::PopStyleVar(3);
    
    ImGui::DockSpace(dockspaceId);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("Connect...");
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    ImGui::End();


    appData->view3d->SetBackgroundColor(0, 0, 0, 1.f);
    //ImGui::ShowDemoWindow();

    if (ImGui::Begin("Test"))
    {
        Vec3 tri[3] = {
            { 0.0f, -0.5f, 0.f},
            {-0.5f, 0.5f,  0.f},
            { 0.5f, 0.5f,  0.f}
        };

        appData->view3d->DrawPoints(tri, 3);

        appData->view3d->DrawLine(tri[0], tri[1]);
        appData->view3d->DrawLine(tri[1], tri[2]);

        appData->view3d->DrawViewBall();

        appData->view3d->Render();
        appData->view3d->Image(ImVec2(1200, 800));
    }
    ImGui::End();


}


int main()
{
    AppCreationInfo info{};
    info.title = "Test App";
    info.loop = testLoop;
    info.initialHeight = 1580;
    info.initialWidth = 2520;
    AppData appData;

    info.userData = &appData;


    Application app(info);
    return 0;
}
