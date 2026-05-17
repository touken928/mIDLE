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

bool FlatButton(const char *label, int width, const ImVec4 &bg, const ImVec4 &fg, bool selected = false) {
    const Theme &theme = GetTheme();
    ImGui::PushStyleColor(ImGuiCol_Button, selected ? theme.selected : bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, theme.hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme.accent);
    ImGui::PushStyleColor(ImGuiCol_Text, fg);
    bool pressed = ImGui::Button(label, ImVec2(static_cast<float>(std::max(width, 1)), 1.f));
    ImGui::PopStyleColor(4);
    return pressed;
}

void RenderToolbarWindow(App &app, TuiActions &actions, int x, int y, int width, int height) {
    const Theme &theme = GetTheme();
    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(x), static_cast<float>(y)), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), static_cast<float>(height)), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.sidebar);
    ImGui::Begin("mIDLE", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::PopStyleColor();

    if (FlatButton(app.mp_running ? "Stop" : "Run", 10, theme.run, theme.title, app.mp_running)) {
        if (app.mp_running) {
            actions.stop = true;
        } else {
            actions.run = true;
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("| Ctrl+R Run  Esc Exit");

    ImGui::End();
}

void RenderEditorWindow(App &app, int x, int y, int width, int height) {
    const Theme &theme = GetTheme();
    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(x), static_cast<float>(y)), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), static_cast<float>(height)), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.main);
    ImGui::Begin("Editor", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::PopStyleColor();

    const int panel_h = static_cast<int>(ImGui::GetContentRegionAvail().y);
    RenderEditorPanel(app, std::max(width - 2, 8), std::max(panel_h, 4));
    ImGui::End();
}

void RenderConsoleWindow(App &app, TuiActions &actions, int x, int y, int width, int height) {
    const Theme &theme = GetTheme();
    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(x), static_cast<float>(y)), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), static_cast<float>(height)), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.panel);
    ImGui::Begin("Console", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::PopStyleColor();
    const int panel_h = static_cast<int>(ImGui::GetContentRegionAvail().y);
    RenderShellPanel(app, actions, std::max(width - 2, 8), std::max(panel_h, 6));
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
    const int toolbar_h = 3;
    const int body_y = toolbar_h;
    const int body_h = std::max(screen_h - toolbar_h, 10);

    RenderToolbarWindow(app, actions, 0, 0, screen_w, toolbar_h);

    if (screen_w < kNarrowColumns) {
        const int editor_h = std::max(6, body_h * 3 / 5);
        const int console_h = std::max(6, body_h - editor_h);
        RenderEditorWindow(app, 0, body_y, screen_w, editor_h);
        RenderConsoleWindow(app, actions, 0, body_y + editor_h, screen_w, console_h);
    } else {
        const int editor_w = std::max(24, screen_w * 2 / 3);
        const int console_w = std::max(16, screen_w - editor_w);
        RenderEditorWindow(app, 0, body_y, editor_w, body_h);
        RenderConsoleWindow(app, actions, editor_w, body_y, console_w, body_h);
    }
    return actions;
}

} // namespace ui
} // namespace midle
