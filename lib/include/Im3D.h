#pragma once
#include "imgui.h"
#include <memory>
/*
    Create a simple 3D view inside Dear Imgui using an immediate-mode API.
    Uses OpenGL for the backend.
    Allows for lightweight rendering of geometric primitives and meshes.
    The idea is to create debug views which integrate well into an ImGui::Image 
    with minimal effort. For now, we'll just assume an OpenGL backend, but try to make
    it possible to isolate those bits in the future.
*/
struct Vec3
{
    float x, y, z;
};

class View3d
{
private:
    struct Impl;
    std::unique_ptr<Impl> impl;

public:
    View3d(const ImVec2& framebufferSize);
    ~View3d();

    void SetBackgroundColor(float r, float g, float b, float a = 1.f);

    void DrawPoints(const Vec3* points, int numPoints);

    void DrawLine(const Vec3 start, const Vec3 end);

    void DrawViewBall();
    /*
    */
    void Render();

    /*
        Equivalent of ImGui::Image(), rendering this view3d to an image.
    */
    void Image(const ImVec2& size);
};