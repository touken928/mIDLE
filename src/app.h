#pragma once

#include <string>

namespace midle {

struct App {
    bool running = true;
    bool mp_running = false;
    bool mp_finished = false;
    bool stop_requested = false;
    bool scroll_shell = false;
    bool focus_stdin = false;

    std::string editor_text;
    std::string shell_text;
    std::string stdin_text;
    std::string status_text;
    std::string file_path;
};

void app_init(App &app, const char *file_path = nullptr);
void app_frame(App &app);
void app_shutdown(App &app);

} // namespace midle
