#pragma once
#include "imgui.h"

namespace midle {
namespace ui {

inline void ApplyTheme() {
    auto &s = ImGui::GetStyle();

    s.WindowRounding    = 0.f;
    s.ChildRounding     = 0.f;
    s.FrameRounding     = 0.f;
    s.PopupRounding     = 0.f;
    s.ScrollbarRounding = 0.f;
    s.GrabRounding      = 0.f;
    s.TabRounding       = 0.f;

    s.WindowBorderSize  = 0.f;
    s.ChildBorderSize   = 1.f;
    s.PopupBorderSize   = 0.f;
    s.TabBorderSize     = 0.f;

    s.WindowPadding     = ImVec2(0, 0);
    s.FramePadding      = ImVec2(4, 3);
    s.ItemSpacing       = ImVec2(6, 4);
    s.ItemInnerSpacing  = ImVec2(4, 4);

    auto bg       = ImVec4(0.12f, 0.12f, 0.15f, 1.f);
    auto bg_edit  = ImVec4(0.09f, 0.09f, 0.12f, 1.f);
    auto bg_shell = ImVec4(0.07f, 0.07f, 0.09f, 1.f);
    auto text     = ImVec4(0.80f, 0.84f, 0.96f, 1.f);
    auto text_dim = ImVec4(0.50f, 0.53f, 0.63f, 1.f);
    auto accent   = ImVec4(0.54f, 0.71f, 0.98f, 1.f);
    auto border   = ImVec4(0.27f, 0.27f, 0.35f, 1.f);

    s.Colors[ImGuiCol_Text]             = text;
    s.Colors[ImGuiCol_TextDisabled]     = text_dim;
    s.Colors[ImGuiCol_WindowBg]         = bg;
    s.Colors[ImGuiCol_ChildBg]          = bg_edit;
    s.Colors[ImGuiCol_PopupBg]          = bg;
    s.Colors[ImGuiCol_Border]           = border;
    s.Colors[ImGuiCol_FrameBg]          = bg_edit;
    s.Colors[ImGuiCol_FrameBgHovered]   = bg;
    s.Colors[ImGuiCol_FrameBgActive]    = bg;
    s.Colors[ImGuiCol_TitleBg]          = bg;
    s.Colors[ImGuiCol_TitleBgActive]    = bg;
    s.Colors[ImGuiCol_TitleBgCollapsed] = bg;
    s.Colors[ImGuiCol_MenuBarBg]        = ImVec4(0.10f, 0.10f, 0.13f, 1.f);
    s.Colors[ImGuiCol_ScrollbarBg]      = bg;
    s.Colors[ImGuiCol_ScrollbarGrab]        = border;
    s.Colors[ImGuiCol_ScrollbarGrabHovered] = accent;
    s.Colors[ImGuiCol_ScrollbarGrabActive]  = accent;
    s.Colors[ImGuiCol_Button]           = ImVec4(0.20f, 0.20f, 0.27f, 1.f);
    s.Colors[ImGuiCol_ButtonHovered]    = accent;
    s.Colors[ImGuiCol_ButtonActive]     = ImVec4(0.45f, 0.62f, 0.87f, 1.f);
    s.Colors[ImGuiCol_Header]           = ImVec4(0.54f, 0.71f, 0.98f, 0.25f);
    s.Colors[ImGuiCol_HeaderHovered]    = ImVec4(0.54f, 0.71f, 0.98f, 0.35f);
    s.Colors[ImGuiCol_HeaderActive]     = ImVec4(0.54f, 0.71f, 0.98f, 0.50f);
    s.Colors[ImGuiCol_ResizeGrip]       = border;
    s.Colors[ImGuiCol_ResizeGripHovered]= accent;
    s.Colors[ImGuiCol_ResizeGripActive] = accent;
    s.Colors[ImGuiCol_TextSelectedBg]   = ImVec4(0.54f, 0.71f, 0.98f, 0.35f);
}

// Layout constants
constexpr float kSplitRatio = 0.58f;  // editor width fraction (wide mode)

} // namespace ui
} // namespace midle
