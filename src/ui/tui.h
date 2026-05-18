#pragma once

#include "../app.h"

namespace midle {
namespace ui {

struct TuiActions {
    bool run = false;
    bool stop = false;
    bool feed_stdin = false;
    bool quit = false;
    bool dismiss_console = false;
    bool save = false;
};

void ApplyTheme();
TuiActions RenderWorkspace(App &app);

} // namespace ui
} // namespace midle
