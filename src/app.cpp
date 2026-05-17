#include "app.h"

#include "mpy/mpy.h"
#include "ui/tui.h"

#include "imtui/imtui-impl-ncurses.h"
#include "imtui/imtui-impl-text.h"
#include "imtui/imtui.h"

namespace midle {
namespace {

ImTui::TScreen *g_screen = nullptr;

void append_output(App &app) {
    std::string out = mpy::take_output();
    if (!out.empty()) {
        app.shell_text += out;
        app.scroll_shell = true;
    }
}

} // namespace

void app_init(App &app) {
    ImGui::CreateContext();
    g_screen = ImTui_ImplNcurses_Init(true);
    ImTui_ImplText_Init();

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;

    int stack_top = 0;
    mpy::init(&stack_top);

    app.editor_text =
        "# Welcome to mIDLE\n"
        "name = input('Your name: ')\n"
        "print('Hello,', name)\n";
    app.shell_text.clear();
    app.status_text = "Ready";
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
            app.status_text = "Finished";
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
        mpy::run_async(app.editor_text);
        app.mp_running = true;
        app.focus_stdin = true;
        app.status_text = "Running";
    }

    if (actions.stop && app.mp_running) {
        mpy::stop();
        app.status_text = "Stopping";
    }

    if (actions.quit) {
        app.running = false;
    }

    ImGui::Render();
    ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), g_screen);
    ImTui_ImplNcurses_DrawScreen(true);
}

} // namespace midle
