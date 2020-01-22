#include <iostream>

#include "Application.h"
#include "imgui.h"
#include "Im3D.h"


void testLoop(const AppContext* ctx, void*)
{
	ImGui::ShowDemoWindow();

	if (ImGui::Begin("Test"))
	{
		Im3d::Begin(ctx->im3d, "Test Image");

		Im3d::DrawPoints(ctx->im3d, nullptr, 0);

		Im3d::End(ctx->im3d);
	}
	ImGui::End();
}


int main()
{
	AppCreationInfo info{};
	info.title = "Test App";
	info.loop = testLoop;
	

	Application app(info);
	return 0;
}
