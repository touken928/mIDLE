#include "tui.h"

#include "editor_panel.h"
#include "layout.h"
#include "shell_panel.h"

#include "imgui.h"

#include <algorithm>

namespace midle {
namespace ui {
namespace {

bool CtrlShortcut(int control_code) {
    return ImGui::IsKeyPressed(control_code);
}

void RenderStatusBar(int x, int y, int width, int height, bool mp_running) {
    const Theme &theme = GetTheme();
    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(x), static_cast<float>(y)), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), static_cast<float>(height)), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.sidebar);
    ImGui::Begin("##status", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, theme.muted);
    ImGui::Text("Ctrl+R %s  Esc Exit", mp_running ? "Stop" : "Run");
    ImGui::PopStyleColor();

    ImGui::End();
}

void RenderEditorWindow(App &app, int x, int y, int width, int height) {
    const Theme &theme = GetTheme();
    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(x), static_cast<float>(y)), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), static_cast<float>(height)), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.main);
    ImGui::Begin("mIDLE", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::PopStyleColor();

    const int panel_h = static_cast<int>(ImGui::GetContentRegionAvail().y);
    RenderEditorPanel(app, std::max(width - 2, 8), std::max(panel_h, 4));
    ImGui::End();
}

void RenderConsolePopup(App &app, TuiActions &actions, int screen_w, int screen_h) {
    if (!app.mp_running) {
        return;
    }
    const Theme &theme = GetTheme();
    const int popup_w = std::max(24, screen_w * 1 / 2);
    const int popup_h = std::max(10, screen_h * 2 / 5);
    const int popup_x = screen_w - popup_w;
    const int popup_y = screen_h - popup_h - 1;  // above status bar

    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(popup_x), static_cast<float>(popup_y)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(popup_w), static_cast<float>(popup_h)), ImGuiCond_FirstUseEver);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.panel);
    bool open = true;
    if (!ImGui::Begin("Console", &open)) {
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }
    ImGui::PopStyleColor();

    if (!open) {
        actions.stop = true;
    }

    const int panel_h = static_cast<int>(ImGui::GetContentRegionAvail().y);
    RenderShellPanel(app, actions, std::max(static_cast<int>(ImGui::GetContentRegionAvail().x), 8), std::max(panel_h, 6));

    ImGui::End();
}

} // namespace

void ApplyTheme() {
    const Theme &theme = GetTheme();
    ImGuiStyle &style = ImGui::GetStyle();

    style.WindowRounding = 0.f;
    style.ChildRounding = 0.f;
    style.FrameRounding = 0.f;
    style.PopupRounding = 0.f;
    style.ScrollbarRounding = 0.f;
    style.GrabRounding = 0.f;
    style.TabRounding = 0.f;

    style.WindowBorderSize = 0.f;
    style.ChildBorderSize = 1.f;
    style.PopupBorderSize = 1.f;
    style.WindowPadding = ImVec2(0, 0);
    style.FramePadding = ImVec2(1, 0);
    style.ItemSpacing = ImVec2(1, 0);
    style.ItemInnerSpacing = ImVec2(1, 0);

    style.Colors[ImGuiCol_Text] = theme.text;
    style.Colors[ImGuiCol_TextDisabled] = theme.muted;
    style.Colors[ImGuiCol_WindowBg] = theme.main;
    style.Colors[ImGuiCol_ChildBg] = theme.panel;
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.04f, 0.04f, 0.06f, 1.f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.24f, 0.24f, 0.31f, 1.f);
    style.Colors[ImGuiCol_FrameBg] = theme.field;
    style.Colors[ImGuiCol_FrameBgHovered] = theme.hovered;
    style.Colors[ImGuiCol_FrameBgActive] = theme.selected;
    style.Colors[ImGuiCol_MenuBarBg] = theme.sidebar;
    style.Colors[ImGuiCol_Button] = theme.panel;
    style.Colors[ImGuiCol_ButtonHovered] = theme.hovered;
    style.Colors[ImGuiCol_ButtonActive] = theme.accent;
    style.Colors[ImGuiCol_Header] = theme.selected;
    style.Colors[ImGuiCol_HeaderHovered] = theme.hovered;
    style.Colors[ImGuiCol_HeaderActive] = theme.accent;
    style.Colors[ImGuiCol_ScrollbarBg] = theme.field;
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.22f, 0.25f, 0.34f, 1.f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = theme.accent;
    style.Colors[ImGuiCol_ScrollbarGrabActive] = theme.accent;
    style.Colors[ImGuiCol_TitleBg] = theme.sidebar;
    style.Colors[ImGuiCol_TitleBgActive] = theme.sidebar;
    style.Colors[ImGuiCol_TitleBgCollapsed] = theme.sidebar;
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.32f, 0.56f, 0.92f, 0.35f);
}

TuiActions RenderWorkspace(App &app) {
    TuiActions actions;

    if (CtrlShortcut(18)) {
        if (app.mp_running) {
            actions.stop = true;
        } else {
            actions.run = true;
        }
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
        actions.quit = true;
    }

    const int screen_w = std::max(static_cast<int>(ImGui::GetIO().DisplaySize.x), 40);
    const int screen_h = std::max(static_cast<int>(ImGui::GetIO().DisplaySize.y), 16);
    const int status_h = 1;
    const int body_h = screen_h - status_h;

    RenderEditorWindow(app, 0, 0, screen_w, body_h);
    RenderConsolePopup(app, actions, screen_w, screen_h);
    RenderStatusBar(0, screen_h - status_h, screen_w, status_h, app.mp_running);
    return actions;
}

} // namespace ui
} // namespace midle
