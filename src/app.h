#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include "runtime/runtime.h"

namespace midle {

struct App {
    bool running = true;
    bool executing = false;
    bool run_finished = false;
    bool stop_requested = false;
    bool scroll_shell = false;
    bool focus_stdin = false;

    std::string editor_text;
    std::string shell_text;
    std::string stdin_text;
    std::string status_text;
    std::string file_path;
};

constexpr std::size_t kShellTextMax = 256 * 1024;

void append_shell_text(std::string &shell, const std::string &chunk);

void app_init(App &app, const char *file_path, runtime::LanguageId language);
void app_frame(App &app);
void app_shutdown(App &app);

} // namespace midle
