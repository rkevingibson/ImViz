#include "Application.h"

#include <cstdio>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "GL/gl3w.h"
#include "GLFW/glfw3.h"

#include "Im3D.h"

namespace
{
    void glfwErrorCallback(int error, const char* description)
    {
        fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    }

    void SetClipboardText(void* userData, const char* text)
    {
        glfwSetClipboardString((GLFWwindow*)userData, text);
    }

    const char* GetClipboardText(void* userData)
    {
        return glfwGetClipboardString((GLFWwindow*)userData);
    }

    

    void APIENTRY glMessageCallback(GLenum source,
        GLenum type, GLuint id, GLenum severity, GLsizei length,
        const GLchar* message,
        void* userParam)
    {
        printf("%s\n", message);
    }
}

struct Application::ApplicationImpl
{
    GLFWwindow* window;

    bool InitializeGLFW(const AppCreationInfo& info)
    {
        //Initialize GLFW
        glfwSetErrorCallback(glfwErrorCallback);
        if (!glfwInit())
        {
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(info.initialWidth, info.initialHeight, info.title, nullptr, nullptr);
        if (window == NULL)
        {
            fprintf(stderr, "Failed to create window\n");
            return false;
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);//enable vsync

        bool err = gl3wInit() != 0;
        if (err)
        {
            fprintf(stderr, "Failed to initialize OpenGL loader!\n");
            return false;
        }

        glDebugMessageCallback(glMessageCallback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        return true;
    }

};

Application::Application(const AppCreationInfo& info) : impl{ std::make_unique<ApplicationImpl>() }
{
    if (!impl->InitializeGLFW(info))
    {
        // Should throw exception here.
        return;
    }

    //impl->InitializeImGui();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig fontConfig{};
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    float scaleFactor = 2.f;
    
    fontConfig.SizePixels = scaleFactor*13.f;
    
    io.Fonts->AddFontDefault(&fontConfig);
    ImGui::GetStyle().ScaleAllSizes(scaleFactor);

    ImGui_ImplGlfw_InitForOpenGL(impl->window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    AppContext ctx{};

    while (!glfwWindowShouldClose(impl->window))
    {
        // Process user input
        glfwPollEvents();

        //Start the Dear ImGui frame
        //impl->ImGuiNewFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        

        // Run the main loop function
        (*info.loop)(&ctx, info.userData);


        // Rendering
        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(impl->window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(1, 1, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        

        glfwSwapBuffers(impl->window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // Cleanup
    glfwDestroyWindow(impl->window);
    glfwTerminate();
}

Application::~Application() = default;
