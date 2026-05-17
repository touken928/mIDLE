#pragma once
#include <string>

namespace midle {

struct App {
    bool        running     = true;
    std::string editor_text;
    std::string shell_text;
    std::string stdin_text;         // stdin input field
    bool        scroll_shell = false;
    bool        mp_running   = false; // MicroPython is executing in background
};

void app_init(App &app);
void app_frame(App &app);
void app_shutdown(App &app);

} // namespace midle
