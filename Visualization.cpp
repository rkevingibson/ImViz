#include <iostream>

#include "Application.h"
#include "imgui.h"
#include "Im3D.h"

struct AppData
{
	View3d* view3d{ nullptr };
};

void testLoop(const AppContext* ctx, void* userData)
{
	AppData* appData = (AppData*)userData;
	if (appData->view3d == nullptr)
	{
		appData->view3d = new View3d(ImVec2(600, 400));
	}


	appData->view3d->SetBackgroundColor(0, 0, 0, 1.f);
	ImGui::ShowDemoWindow();

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

		appData->view3d->Image(ImVec2(600, 400));
	}
	ImGui::End();


	appData->view3d->Render();
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
