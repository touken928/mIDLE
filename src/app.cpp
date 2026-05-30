#include "app.h"

#include "highlight/screen.h"
#include "io/file_io.h"
#include "runtime/runtime.h"
#include "ui/tui.h"

#include "imtui/imtui-impl-ncurses.h"
#include "imtui/imtui-impl-text.h"
#include "imtui/imtui.h"

#ifdef __unix__
#include <termios.h>
#include <unistd.h>
#endif

namespace midle {
namespace {

ImTui::TScreen *g_screen = nullptr;

void append_output(App &app) {
    std::string out = runtime::take_output();
    if (!out.empty()) {
        append_shell_text(app.shell_text, out);
        app.scroll_shell = true;
    }
}

} // namespace

void append_shell_text(std::string &shell, const std::string &chunk) {
    if (chunk.empty()) return;
    shell += chunk;
    if (shell.size() > kShellTextMax) {
        shell.erase(0, shell.size() - kShellTextMax);
    }
}

void app_init(App &app, const char *file_path, runtime::LanguageId language) {
    ImGui::CreateContext();
    g_screen = ImTui_ImplNcurses_Init(true);
    ImTui_ImplText_Init();

#ifdef __unix__
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_iflag &= ~(IXON | IXOFF);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
#endif

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;

    int stack_top = 0;
    runtime::init(&stack_top, language);

    runtime::register_builtin_languages();
    const runtime::LanguageModule *module = runtime::find_language(language);

    if (file_path && file_path[0]) {
        app.file_path = file_path;
        FileLoadResult loaded = load_file(file_path);
        switch (loaded.status) {
        case FileLoadStatus::Ok:
            app.editor_text = std::move(loaded.content);
            app.status_text = loaded.content.empty()
                ? std::string("Loaded empty ") + file_path
                : std::string("Loaded ") + file_path;
            break;
        case FileLoadStatus::NotFound:
            app.editor_text.clear();
            app.status_text = std::string("Cannot open ") + file_path;
            break;
        case FileLoadStatus::IOError:
            app.editor_text.clear();
            app.status_text = std::string("Cannot read ") + file_path;
            break;
        }
    } else if (module && module->default_sample) {
        app.editor_text = module->default_sample;
        app.status_text = module->ready_status ? module->ready_status : "Ready";
    }
    app.shell_text.clear();
}

void app_shutdown(App &) {
    runtime::close_stdin();
    runtime::deinit();
    ImTui_ImplText_Shutdown();
    ImTui_ImplNcurses_Shutdown();
    ImGui::DestroyContext();
}

void app_frame(App &app) {
    ImTui_ImplNcurses_NewFrame();
    ImTui_ImplText_NewFrame();
    ImGui::NewFrame();

    ui::ApplyTheme();

    if (app.executing) {
        append_output(app);
        if (runtime::done()) {
            append_output(app);
            app.scroll_shell = true;
            app.executing = false;
            if (!app.stop_requested) {
                app.run_finished = true;
            }
            app.stop_requested = false;
        }
    }

    ui::TuiActions actions = ui::RenderWorkspace(app);

    if (actions.feed_stdin && app.executing) {
        append_shell_text(app.shell_text, app.stdin_text + "\n");
        runtime::input(app.stdin_text);
        app.stdin_text.clear();
        app.scroll_shell = true;
        app.focus_stdin = true;
        app.status_text = "Input sent";
    }

    if (actions.run && !app.executing) {
        app.shell_text.clear();
        app.stdin_text.clear();
        app.run_finished = false;
        app.stop_requested = false;
        runtime::run_async(app.editor_text);
        app.executing = true;
        app.focus_stdin = true;
        app.status_text = std::string("Running (") + runtime::language_name(runtime::active_language()) + ")";
    }

    if (actions.stop && app.executing) {
        app.stop_requested = true;
        runtime::stop();
        app.status_text = "Stopping";
    }

    if (actions.quit) {
        app.running = false;
    }

    if (actions.dismiss_console) {
        app.run_finished = false;
    }

    if (actions.save) {
        if (!app.file_path.empty()) {
            save_file(app.file_path, app.editor_text);
            app.status_text = std::string("Saved ") + app.file_path;
        } else {
            app.status_text = "No file to save. Run: midle <file>";
        }
    }

    ImGui::Render();
    ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), g_screen);
    highlight::ApplyEditorHighlight(g_screen, runtime::active_language());
    ImTui_ImplNcurses_DrawScreen(true);
}

} // namespace midle
