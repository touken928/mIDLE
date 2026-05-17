#pragma once

#include "../app.h"

namespace midle {
namespace ui {

struct TuiActions {
    bool run = false;
    bool stop = false;
    bool feed_stdin = false;
    bool quit = false;
};

void ApplyTheme();
TuiActions RenderWorkspace(App &app);

} // namespace ui
} // namespace midle
