#pragma once
#include "imgui.h"
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

    struct Vec3
    {
        float x, y, z;
    };

    enum class DrawType
    {
        Points,
        Lines,
        Triangles
    };

    struct DrawCmd
    {
        DrawType type;
        unsigned int count;
        unsigned int offset;
        
        ImVec4 clipRect;

    };

    struct DrawVert
    {
        Vec3 pos;
        Vec3 normal;
        ImU32 col;
    };

    struct DrawList
    {
        ImVector<DrawCmd> cmdBuffer;
        ImVector<DrawVert> vertexBuffer;
        ImVector<unsigned int> indexBuffer;
    };

    struct DrawData
    {
        
        DrawList** drawLists;
        int drawListsCount;
        ImVec2 displayPos;
        ImVec2 displaySize;
    };

    Context* CreateContext();
    void NewFrame(Context*);
    void Render(Context*);
    DrawData* GetDrawList(Context* ctx);


    // Create a 3D window with name "name", and 
    bool Begin(Context* ctx, const char* name, const ImVec2& size);
    void End(Context*);

    void DrawPoints(Context* ctx, const Vec3* points, int numPoints);

    void RenderDrawDataOpenGL(const DrawData* drawList);
}