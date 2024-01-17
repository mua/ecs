#include "Application.h"

#ifdef EMSCRIPTEN
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#include <emscripten.h>
#include <GLES2/gl2.h>
#include "emscripten.h"
#include <stdio.h>
#else
#include <gl/glew.h>
#endif // EMSCRIPTEN



#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "engine.h"
#include "loguru.hpp"
#include <chrono>

GLFWwindow *window;

static void glfw_error_callback(int error, const char *description)
{
    printf("Glfw Error %d: %s\n", error, description);
}

Application *Application::_instance = nullptr;

void main_loop(void *);

Application::Application(/* args */)
{
    _instance = this;
}

void main_loop(void *arg)
{
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    Engine::instance()->loop();
    ImGui::Render();

    auto clearColor = Application::instance()->clearColor;
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // ImGuiIO &io = ImGui::GetIO(); (void)io;
    // if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    // {
    //     GLFWwindow *backup_current_context = glfwGetCurrentContext();
    //     ImGui::UpdatePlatformWindows();
    //     ImGui::RenderPlatformWindowsDefault();
    //     glfwMakeContextCurrent(backup_current_context);
    // }

    glfwSwapBuffers(window);
}

Application::~Application()
{
}

void Application::onWindowResized(GLFWwindow *window, int width, int height)
{
    if (width == 0 || height == 0)
        return;

    Application *app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
    app->saveSetting("size", std::array{width, height});
}

void Application::run()
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return;
    const char *glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_SAMPLES, 4);
    //glEnable(GL_MULTISAMPLE);

#ifdef EMSCRIPTEN
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glEnable(GL_MULTISAMPLE);
#else
#endif // EMSCRIPTEN
    auto size = loadSetting("size", std::array{1280, 720});
    auto time = std::chrono::high_resolution_clock::now();

    window = glfwCreateWindow(100, 100, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    auto time2 = std::chrono::high_resolution_clock::now();
    LOG_F(INFO, "Window init took: %d ms", std::chrono::duration_cast<std::chrono::milliseconds>(time2 - time));

    if (window == NULL)
        return;
    glfwSetWindowSizeCallback(window, Application::onWindowResized);
    glfwSetWindowUserPointer(window, this);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    const GLubyte *vendor = glGetString(GL_VENDOR);     // Returns the vendor
    const GLubyte *renderer = glGetString(GL_RENDERER); // Returns a hint to the model
    LOG_F(INFO, "OpenGL Context created: %s - %s", vendor, renderer);

#ifndef EMSCRIPTEN
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        fprintf(stderr, "Glew Error: %s\n", glewGetErrorString(err));
    }
#endif                         // !EMSCRIPTEN

    //IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    auto pio = ImGui::GetPlatformIO();
    pio.Monitors[0].MainSize = ImVec2(100, 100);
    pio.Monitors[0].WorkSize = ImVec2(100, 100);
    //io.IniFilename =  "ui.ini";
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    Engine::instance()->start();
#ifdef EMSCRIPTEN
    io.IniFilename = NULL;
    emscripten_set_main_loop_arg(main_loop, NULL, 0, true);
#else
    while (!glfwWindowShouldClose(window))
    {
        main_loop(nullptr);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
#endif
}
