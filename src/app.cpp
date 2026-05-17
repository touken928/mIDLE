#include "app.h"
#include "mpy.h"
#include "ui/layout.h"
#include "ui/editor_panel.h"
#include "ui/shell_panel.h"

#include "imtui/imtui.h"
#include "imtui/imtui-impl-ncurses.h"
#include "imtui/imtui-impl-text.h"

#include <algorithm>

namespace midle {

// ── ImTUI screen ─────────────────────────────────────────────
static ImTui::TScreen *g_screen = nullptr;

constexpr float kNarrowCols = 70.f;
constexpr float kMinPanelW  = 30.f;
constexpr float kMinEditorH = 6.f;
constexpr float kMinShellH  = 4.f;

// ═════════════════════════════════════════════════════════════
void app_init(App &app) {
    ImGui::CreateContext();
    g_screen = ImTui_ImplNcurses_Init(true);
    ImTui_ImplText_Init();

    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr;

    int stack_top;
    mpy::init(&stack_top);

    app.editor_text =
        "# Welcome to mIDLE\n"
        "print('Hello from MicroPython!')\n";

    app.shell_text =
        "mIDLE  —  MicroPython IDLE\n"
        "  Run ▸ Run Module  or  F5\n"
        ">>> \n";
}

void app_shutdown(App &) {
    mpy::deinit();
    ImTui_ImplText_Shutdown();
    ImTui_ImplNcurses_Shutdown();
}

// ═════════════════════════════════════════════════════════════
void app_frame(App &app) {
    ImTui_ImplNcurses_NewFrame();
    ImTui_ImplText_NewFrame();
    ImGui::NewFrame();

    ui::ApplyTheme();

    auto &io   = ImGui::GetIO();
    auto  disp = io.DisplaySize;

    bool run_code    = false;
    bool clear_shell = false;
    bool feed_stdin  = false;

    // ── Poll async output ────────────────────────────────
    if (app.mp_running) {
        std::string out = mpy::take_output();
        if (!out.empty()) {
            app.shell_text += out;
            app.scroll_shell = true;
        }
        if (mpy::done()) {
            app.shell_text += ">>> \n";
            app.scroll_shell = true;
            app.mp_running = false;
        }
    }

    // ── Root window ──────────────────────────────────────
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(disp, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("mIDLE", nullptr,
        ImGuiWindowFlags_NoTitleBar    | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove        | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse    | ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

    // ── Menu bar ─────────────────────────────────────────
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit", "Esc")) app.running = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Run")) {
            if (!app.mp_running && ImGui::MenuItem("Run Module", "F5"))
                run_code = true;
            if (ImGui::MenuItem("Close stdin")) {
                mpy::close_stdin();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Shell")) {
            if (ImGui::MenuItem("Clear")) clear_shell = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("About")) {
            ImGui::TextDisabled("mIDLE v0.1");
            ImGui::Separator();
            ImGui::BulletText("ImTUI + MicroPython");
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // ── Panel layout ─────────────────────────────────────
    const float status_h = ImGui::GetFrameHeight();
    ImVec2      content  = ImGui::GetContentRegionAvail();
    const float panel_h  = std::max(content.y - status_h, kMinEditorH + kMinShellH);
    const bool  narrow   = (content.x < kNarrowCols);

    if (narrow) {
        float ed_h = std::max(panel_h * ui::kSplitRatio, kMinEditorH);
        float sh_h = panel_h - ed_h;
        if (sh_h < kMinShellH) { ed_h = panel_h - kMinShellH; sh_h = kMinShellH; }

        ui::EditorPanel(app, ImVec2(content.x, ed_h));
        ui::ShellPanel(app, ImVec2(content.x, sh_h), app.mp_running, feed_stdin);
    } else {
        float ed_w = std::max(content.x * ui::kSplitRatio, kMinPanelW);
        float sh_w = content.x - ed_w;
        if (sh_w < kMinPanelW) { ed_w = content.x - kMinPanelW; sh_w = kMinPanelW; }

        ui::EditorPanel(app, ImVec2(ed_w, panel_h));
        ImGui::SameLine(0.f, 0.f);
        ui::ShellPanel(app, ImVec2(sh_w, panel_h), app.mp_running, feed_stdin);
    }

    // ── Status bar ───────────────────────────────────────
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 2));
        ImGui::BeginChild("##status",
            ImVec2(content.x, status_h), false,
            ImGuiWindowFlags_NoScrollbar);
        ImGui::PopStyleVar();

        size_t lc = 1;
        for (size_t i = 0; i < app.editor_text.size(); i++)
            if (app.editor_text[i] == '\n') lc++;
        if (app.mp_running)
            ImGui::TextDisabled("[running]  Ln %zu", lc);
        else
            ImGui::TextDisabled("Ln %zu  Ch %zu", lc, app.editor_text.size());

        float bw = 50.f;
        ImGui::SameLine(content.x - (bw + 4.f) * 2 - 8.f);
        if (ImGui::Button("Run", ImVec2(bw, 0))) run_code = true;
        ImGui::SameLine(0.f, 4.f);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.30f, 0.15f, 0.15f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.50f, 0.20f, 0.20f, 1.f));
        if (ImGui::Button("Clr", ImVec2(bw, 0))) clear_shell = true;
        ImGui::PopStyleColor(2);

        ImGui::EndChild();
    }

    // ── Feed stdin ───────────────────────────────────────
    if (feed_stdin && !app.stdin_text.empty()) {
        mpy::input(app.stdin_text);
        app.shell_text += app.stdin_text + "\n";
        app.stdin_text.clear();
        app.scroll_shell = true;
    }

    // ── Handle actions ──────────────────────────────────
    if (run_code) {
        if (!app.mp_running) {
            app.shell_text += ">>> Running...\n";
            app.scroll_shell = true;
            mpy::run_async(app.editor_text);
            app.mp_running = true;
        }
    }
    if (clear_shell) {
        app.shell_text = "mIDLE  —  MicroPython IDLE\n>>> \n";
        app.scroll_shell = false;
    }

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
        app.running = false;

    ImGui::End();

    ImGui::Render();
    ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), g_screen);
    ImTui_ImplNcurses_DrawScreen();
}

} // namespace midle
