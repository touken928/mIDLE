#include "shell_panel.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

namespace midle {
namespace ui {

void ShellPanel(App &app, const ImVec2 &child_size,
                bool mp_running, bool &feed_stdin)
{
    ImVec2 sz = child_size;
    if (sz.x <= 0) sz.x = 30;
    if (sz.y <= 0) sz.y = 6;

    const float stdin_h = ImGui::GetFrameHeightWithSpacing();

    // ── Output area ──────────────────────────────────────
    float out_h = sz.y - stdin_h;
    if (out_h < 2.f) out_h = 2.f;

    ImGui::BeginChild("shell_out", ImVec2(sz.x, out_h), true,
        ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::TextUnformatted(app.shell_text.c_str());

    if (app.scroll_shell) {
        ImGui::SetScrollHereY(1.0f);
        app.scroll_shell = false;
    }
    ImGui::EndChild();

    // ── Stdin input line ─────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.09f, 0.09f, 0.12f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_Text,    ImVec4(0.80f, 0.84f, 0.96f, 1.f));
    ImGui::SetNextItemWidth(sz.x - 8.f);
    if (ImGui::InputText("##stdin", &app.stdin_text,
            ImGuiInputTextFlags_EnterReturnsTrue)) {
        feed_stdin = true;
    }
    ImGui::PopStyleColor(2);

    // Focus the stdin field when MicroPython is running (needs input)
    if (mp_running)
        ImGui::SetKeyboardFocusHere(-1);
}

} // namespace ui
} // namespace midle
