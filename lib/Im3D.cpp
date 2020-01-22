#include "include\Im3D.h"
#include "include\Im3D.h"
#include "Im3D.h"
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"


namespace Im3d
{
    struct View3d
    {
        ImGuiID id;
        ImRect bb;

        ImVector<Vec3> points;

        ImDrawList
    };

    struct Context
    {
        ImGuiStorage views;
        View3d* currentView;
    };



    Context* CreateContext()
    {
        Context* ctx = IM_NEW(Context)();
        ctx->currentView = nullptr;
        return ctx;
    }

    void NewFrame(Context* ctx)
    {

    }

    void Render(Context* ctx)
    {

    }

    DrawList* GetDrawList(Context* ctx)
    {
        return nullptr;
    }

    static View3d* GetViewByName(Context* ctx, const char* name)
    {
        ImGuiID id = ImHashStr(name);
        return (View3d*)ctx->views.GetVoidPtr(id);
    }

    static View3d* CreateNewView(Context* ctx, const char* name)
    {
        View3d* view = IM_NEW(View3d)();
        view->id = ImHashStr(name);

        ctx->views.SetVoidPtr(view->id, view);

        return view;
    }

    bool Begin(Context* ctx, const char* name, const ImVec2& size)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) // Window is collapsed
        {
            return false;
        }
        
        View3d* view = GetViewByName(ctx, name);
        if (view == nullptr)
        {
            view = CreateNewView(ctx, name);
        }
        ctx->currentView = view;

        ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
        view->bb = bb;

        // Reserve size for this view in the imgui window.
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, 0)) //Are we clipped?
        {
            return false;
        }

        return true;
    }

    void End(Context* ctx)
    {
        ctx->currentView = nullptr;
    }

    void DrawPoints(Context* ctx, const Vec3* points, int numPoints)
    {
        IM_ASSERT(ctx->currentView != nullptr);
        View3d* v = ctx->currentView;
        v->points.resize(v->points.size() + numPoints);

        for (int i = 0, pi = v->points.size(); i < numPoints; ++i, ++pi)
        {
            v->points[pi] = points[i];
        }
    }

}