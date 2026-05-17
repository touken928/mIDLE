#pragma once
#include "imgui.h"
#include "../app.h"

namespace midle {
namespace ui {

// mp_running / feed_stdin are in/out params for async execution
void ShellPanel(App &app, const ImVec2 &child_size,
                bool mp_running, bool &feed_stdin);

} // namespace ui
} // namespace midle
