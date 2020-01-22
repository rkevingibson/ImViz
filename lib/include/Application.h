#pragma once
#include <memory>

namespace Im3d
{
    struct Context;
}

/*
    We define a very simple way to make a Gui application using some predefined interfaces.
    In short, all the user should have to do is provide a main loop function, and everything else will be handled.
    This function will get passed an AppContext structure which gives access to any data it might need to customize it's main loop,
    including APIs for input and rendering.
*/

struct AppContext
{
    Im3d::Context* im3d;

};

using AppLoopFn = void(*)(const AppContext* ctx, void* userData);

struct AppCreationInfo
{
    AppLoopFn loop{nullptr};
    void* userData{nullptr};
    int initialWidth{1280};
    int initialHeight{720};
    const char* title;
};

class Application
{
    struct ApplicationImpl;
    std::unique_ptr<ApplicationImpl> impl{nullptr};
public:

    Application(const AppCreationInfo& info);
    ~Application(); // = default.
    Application(const Application&) = delete;
    Application(Application&&) = default;
    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&) = default;
};