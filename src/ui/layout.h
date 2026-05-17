#pragma once

#include "imgui.h"

namespace midle {
namespace ui {

struct Theme {
    ImVec4 text = ImVec4(0.78f, 0.80f, 0.86f, 1.f);
    ImVec4 title = ImVec4(0.96f, 0.97f, 1.00f, 1.f);
    ImVec4 muted = ImVec4(0.48f, 0.52f, 0.60f, 1.f);
    ImVec4 sidebar = ImVec4(0.11f, 0.10f, 0.16f, 1.f);
    ImVec4 main = ImVec4(0.06f, 0.07f, 0.09f, 1.f);
    ImVec4 panel = ImVec4(0.08f, 0.09f, 0.12f, 1.f);
    ImVec4 field = ImVec4(0.03f, 0.04f, 0.06f, 1.f);
    ImVec4 selected = ImVec4(0.18f, 0.28f, 0.46f, 1.f);
    ImVec4 hovered = ImVec4(0.19f, 0.20f, 0.28f, 1.f);
    ImVec4 accent = ImVec4(0.32f, 0.56f, 0.92f, 1.f);
    ImVec4 run = ImVec4(0.18f, 0.42f, 0.26f, 1.f);
    ImVec4 danger = ImVec4(0.54f, 0.20f, 0.22f, 1.f);
};

inline const Theme &GetTheme() {
    static const Theme theme;
    return theme;
}

constexpr int kNarrowColumns = 72;
constexpr int kMinSidebarWidth = 22;
constexpr int kMaxSidebarWidth = 30;

} // namespace ui
} // namespace midle
