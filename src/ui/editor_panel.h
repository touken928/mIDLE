#pragma once
#include "imgui.h"
#include "../app.h"

namespace midle {
namespace ui {

// Renders the editor panel into the current context (must be inside a window).
// Returns the number of lines in the current editor text.
int EditorPanel(App &app, const ImVec2 &child_size);

} // namespace ui
} // namespace midle
