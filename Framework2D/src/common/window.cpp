#include "common/window.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <array>
#include <filesystem>
#include <iostream>

namespace USTC_CG
{
Window::Window(const std::string& window_name) : name_(window_name)
{
    if (!init_glfw())
    {  // Initialize GLFW and check for failure
        throw std::runtime_error("Failed to initialize GLFW!");
    }

    window_ =
        glfwCreateWindow(width_, height_, name_.c_str(), nullptr, nullptr);
    if (window_ == nullptr)
    {
        glfwTerminate();  // Ensure GLFW is cleaned up before throwing
        throw std::runtime_error("Failed to create GLFW window!");
    }

    if (!init_gui())
    {  // Initialize the GUI and check for failure
        glfwDestroyWindow(window_);
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GUI!");
    }
}

Window::~Window()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window_);
    glfwTerminate();
}

bool Window::init()
{
    // Placeholder for additional initialization if needed.
    return true;
}

void Window::run()
{
    glfwShowWindow(window_);

    while (!glfwWindowShouldClose(window_))
    {
        if (!glfwGetWindowAttrib(window_, GLFW_VISIBLE) ||
            glfwGetWindowAttrib(window_, GLFW_ICONIFIED))
            glfwWaitEvents();
        else
        {
            glfwPollEvents();
            render();
        }
    }
}

void Window::draw()
{
    // Placeholder for custom draw logic, should be overridden in derived
    // classes.
    ImGui::ShowDemoWindow();
}

void Window::post_render()
{
}

bool Window::init_glfw()
{
    glfwSetErrorCallback(
        [](int error, const char* desc)
        { fprintf(stderr, "GLFW Error %d: %s\n", error, desc); });

    if (!glfwInit())
    {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    return true;
}

bool Window::init_gui()
{
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);  // Enable vsync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io; 
    float xscale, yscale;
    glfwGetWindowContentScale(window_, &xscale, &yscale);

#if defined(_WIN32)
    bool font_loaded = false;
    const std::array<const char*, 4> font_candidates = {
        "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/msyh.ttf",
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/simsun.ttc",
    };
    for (const auto* font_path : font_candidates)
    {
        if (std::filesystem::exists(font_path))
        {
            if (
                io.Fonts->AddFontFromFileTTF(
                    font_path,
                    16.0f * xscale,
                    nullptr,
                    io.Fonts->GetGlyphRangesChineseFull()) != nullptr)
            {
                font_loaded = true;
                break;
            }
        }
    }

    if (font_loaded)
    {
        io.FontGlobalScale = 1.0f;
    }
    else
    {
        io.Fonts->AddFontDefault();
        io.FontGlobalScale = xscale;
    }
#else
    io.Fonts->AddFontDefault();
    io.FontGlobalScale = xscale;
#endif

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
#if defined(__APPLE__)
    std::cout << "Using Apple: Supported OpenGL version = " << glGetString(GL_VERSION) << std::endl;
    ImGui_ImplOpenGL3_Init("#version 100");
#else
    ImGui_ImplOpenGL3_Init("#version 130");
#endif
    

    return true;
}

void Window::render()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    draw();

    ImGui::Render();

    glfwGetFramebufferSize(window_, &width_, &height_);
    glViewport(0, 0, width_, height_);
    glClearColor(0.35f, 0.45f, 0.50f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    post_render();

    glfwSwapBuffers(window_);
}

}  // namespace USTC_CG