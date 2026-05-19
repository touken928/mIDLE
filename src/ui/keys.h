#pragma once

#include "imgui.h"

namespace midle {
namespace ui {

// Terminal control codes: Ctrl+letter = ASCII(letter) - 64 (ncurses KeysDown indices).
constexpr int kNcursesCtrlR = 18; // Run / Stop
constexpr int kNcursesCtrlS = 19; // Save
constexpr int kNcursesCtrlW = 23; // Save (alternate)

inline bool NcursesCtrlPressed(int control_code) {
    return ImGui::IsKeyPressed(control_code);
}

} // namespace ui
} // namespace midle
