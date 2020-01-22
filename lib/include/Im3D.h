#pragma once

/*
    Create a simple 3D view inside Dear Imgui using an immediate-mode API.
    Uses OpenGL for the backend.
    Allows for lightweight rendering of geometric primitives and meshes.
    The idea is to create debug views which integrate well into an ImGui::Image 
    with minimal effort. For now, we'll just assume an OpenGL backend, but try to make
    it possible to isolate those bits in the future.
*/

namespace Im3d
{
    struct Context;

    Context* CreateContext();
    void NewFrame(Context*);
    void Render(Context*);

    // Create a 3D window with name "name", and 
    bool Begin(Context* ctx, const char* name, bool* p_open = nullptr);
    void End(Context*);

    void DrawPoints(Context* ctx, float* points, int numPoints);
}