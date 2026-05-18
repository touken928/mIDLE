#include "app.h"

#include "mpy/mpy.h"
#include "ui/tui.h"

#include "imtui/imtui-impl-ncurses.h"
#include "imtui/imtui-impl-text.h"
#include "imtui/imtui.h"

#include <fstream>
#include <sstream>

namespace midle {
namespace {

ImTui::TScreen *g_screen = nullptr;

std::string load_file(const char *path) {
    std::ifstream f(path);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void save_file(const std::string &path, const std::string &content) {
    std::ofstream f(path);
    f << content;
}

void append_output(App &app) {
    std::string out = mpy::take_output();
    if (!out.empty()) {
        app.shell_text += out;
        app.scroll_shell = true;
    }
}

} // namespace

void app_init(App &app, const char *file_path) {
    ImGui::CreateContext();
    g_screen = ImTui_ImplNcurses_Init(true);
    ImTui_ImplText_Init();

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;

    int stack_top = 0;
    mpy::init(&stack_top);

    if (file_path && file_path[0]) {
        app.file_path = file_path;
        std::string content = load_file(file_path);
        if (!content.empty()) {
            app.editor_text = content;
            app.status_text = std::string("Loaded ") + file_path;
        } else {
            app.editor_text.clear();
            app.status_text = std::string("New ") + file_path;
        }
    } else {
        app.editor_text =
            "# Welcome to mIDLE\n"
            "name = input('Your name: ')\n"
            "print('Hello,', name)\n";
        app.status_text = "Ready";
    }
    app.shell_text.clear();
}

void app_shutdown(App &) {
    mpy::close_stdin();
    mpy::deinit();
    ImTui_ImplText_Shutdown();
    ImTui_ImplNcurses_Shutdown();
    ImGui::DestroyContext();
}

void app_frame(App &app) {
    ImTui_ImplNcurses_NewFrame();
    ImTui_ImplText_NewFrame();
    ImGui::NewFrame();

    ui::ApplyTheme();

    if (app.mp_running) {
        append_output(app);
        if (mpy::done()) {
            append_output(app);
            app.scroll_shell = true;
            app.mp_running = false;
            if (!app.stop_requested) {
                app.mp_finished = true;
            }
            app.stop_requested = false;
        }
    }

    ui::TuiActions actions = ui::RenderWorkspace(app);

    if (actions.feed_stdin && app.mp_running) {
        app.shell_text += app.stdin_text + "\n";
        mpy::input(app.stdin_text);
        app.stdin_text.clear();
        app.scroll_shell = true;
        app.focus_stdin = true;
        app.status_text = "Input sent";
    }

    if (actions.run && !app.mp_running) {
        app.shell_text.clear();
        app.stdin_text.clear();
        app.mp_finished = false;
        app.stop_requested = false;
        mpy::run_async(app.editor_text);
        app.mp_running = true;
        app.focus_stdin = true;
        app.status_text = "Running";
    }

    if (actions.stop && app.mp_running) {
        app.stop_requested = true;
        mpy::stop();
        app.status_text = "Stopping";
    }

    if (actions.quit) {
        app.running = false;
    }

    if (actions.dismiss_console) {
        app.mp_finished = false;
    }

    if (actions.save && !app.file_path.empty()) {
        save_file(app.file_path, app.editor_text);
        app.status_text = std::string("Saved ") + app.file_path;
    }

    ImGui::Render();
    ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), g_screen);
    ImTui_ImplNcurses_DrawScreen(true);
}

} // namespace midle
