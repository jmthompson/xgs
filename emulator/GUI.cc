// ImGui SDL2 binding with OpenGL3
// In this binding, ImTextureID is used to store an OpenGL 'GLuint' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// https://github.com/ocornut/imgui

#include <SDL.h>
#include <SDL_syswm.h>

#include "imgui.h"
#include "backends/imgui_impl_sdl.h"
#include "backends/imgui_impl_opengl3.h"

#include "Emulator.h"
#include "GUI.h"
#include "Video.h"

#include "disks/IWM.h"

namespace GUI {

static const char* glsl_version = "#version 100";

// Data
static double       g_Time = 0.0f;
static bool         g_MousePressed[3] = { false, false, false };
static float        g_MouseWheel = 0.0f;
static GLuint       g_FontTexture = 0;
static int          g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
static int          g_AttribLocationTex = 0;
static int          g_AttribLocationColor = 0;
static unsigned int g_VboHandle = 0, g_ElementsHandle = 0;

static const char* getClipboardText(void *)
{
    return SDL_GetClipboardText();
}

static void setClipboardText(void *, const char *text)
{
    SDL_SetClipboardText(text);
}

void newFrame(SDL_Window *window)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window);
    ImGui::NewFrame();
}

void render()
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void initialize(SDL_Window *window, SDL_GLContext context)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
    io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
    io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
    io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = SDLK_a;
    io.KeyMap[ImGuiKey_C] = SDLK_c;
    io.KeyMap[ImGuiKey_V] = SDLK_v;
    io.KeyMap[ImGuiKey_X] = SDLK_x;
    io.KeyMap[ImGuiKey_Y] = SDLK_y;
    io.KeyMap[ImGuiKey_Z] = SDLK_z;

    io.SetClipboardTextFn = setClipboardText;
    io.GetClipboardTextFn = getClipboardText;
    io.ClipboardUserData = NULL;
}

void shutdown()
{
    if (g_VboHandle) glDeleteBuffers(1, &g_VboHandle);
    if (g_ElementsHandle) glDeleteBuffers(1, &g_ElementsHandle);

    glDetachShader(g_ShaderHandle, g_VertHandle);
    glDeleteShader(g_VertHandle);
    g_VertHandle = 0;

    glDetachShader(g_ShaderHandle, g_FragHandle);
    glDeleteShader(g_FragHandle);
    g_FragHandle = 0;

    glDeleteProgram(g_ShaderHandle);
    g_ShaderHandle = 0;

    if (g_FontTexture)
    {
        glDeleteTextures(1, &g_FontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture = 0;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void processEvent(SDL_Event& event)
{
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void drawStatusBar(Emulator& emulator)
{
    Video *video = emulator.getVideo();

    const unsigned int bar_height = 34;
    string speed = (format("%0.1f MHz") % emulator.getSpeed()).str();

    string version = (format("XGS v%0d.%0d") % kVersionMajor % kVersionMinor).str();
    bool s5d1 = false;
    bool s5d2 = false;
    bool s6d1 = false;
    bool s6d2 = false;

    switch (emulator.getIwm()->getMotorState()) {
        case 0x50: s5d1 = true; break;
        case 0x51: s5d2 = true; break;
        case 0x60: s6d1 = true; break;
        case 0x61: s6d2 = true; break;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    ImGui::SetNextWindowPos(ImVec2(video->frame_left, video->frame_bottom - bar_height + 1));
    ImGui::SetNextWindowSize(ImVec2(video->frame_width, bar_height));

    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoScrollbar;

    ImGui::Begin("Status", NULL, window_flags);
    ImGui::Text(version.c_str());
    ImGui::SameLine();
    ImGui::Text(speed.c_str());
    ImGui::SameLine();
    ImGui::Checkbox("S5D1", &s5d1);
    ImGui::SameLine();
    ImGui::Checkbox("S5D2", &s5d2);
    ImGui::SameLine();
    ImGui::Checkbox("S6D1", &s6d1);
    ImGui::SameLine();
    ImGui::Checkbox("S6D1", &s6d2);
    ImGui::End();

    ImGui::PopStyleVar();
}

void drawMenu(Emulator& emulator)
{
    Video *video = emulator.getVideo();
    float max_speed = emulator.getMaxSpeed();
    
    ImGui::SetNextWindowPos(ImVec2(video->frame_left + 10, video->frame_top + 10));
    ImGui::SetNextWindowSize(ImVec2(350, 55));
    ImGui::Begin("CPU Speed", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::SliderFloat("Max Speed", &max_speed, 2.8, 32.0);
    ImGui::End();

    emulator.setMaxSpeed(max_speed);
}

} // namespace GUI
