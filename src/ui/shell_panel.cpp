#include "shell_panel.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "layout.h"

#include <algorithm>

namespace midle {
namespace ui {

void RenderShellPanel(App &app, TuiActions &actions, int width, int height) {
    const Theme &theme = GetTheme();
    const int safe_w = std::max(width, 8);
    const int safe_h = std::max(height, 6);

    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme.panel);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.panel);
    ImGui::PushStyleColor(ImGuiCol_Text, theme.text);
    ImGui::BeginChild("console_io", ImVec2(static_cast<float>(safe_w), static_cast<float>(safe_h)), false,
        ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::TextUnformatted(app.shell_text.c_str());
    ImGui::PushStyleColor(ImGuiCol_Text, app.mp_running ? theme.title : theme.muted);
    ImGui::TextUnformatted(app.mp_running ? "> " : "$ ");
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
    if (!app.mp_running) {
        flags |= ImGuiInputTextFlags_ReadOnly;
    }
    if (app.focus_stdin) {
        ImGui::SetKeyboardFocusHere();
        app.focus_stdin = false;
    }
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::InputText("##stdin_inline", &app.stdin_text, flags)) {
        actions.feed_stdin = true;
    }

    if (app.scroll_shell) {
        ImGui::SetScrollHereY(1.0f);
        app.scroll_shell = false;
    }

    ImGui::EndChild();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}

} // namespace ui
} // namespace midle
