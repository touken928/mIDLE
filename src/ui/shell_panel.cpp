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
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme.popup);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.popup);
    ImGui::PushStyleColor(ImGuiCol_Text, theme.text);
    ImGui::BeginChild("console_io", ImVec2(static_cast<float>(safe_w), static_cast<float>(safe_h)), false,
        ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::TextUnformatted(app.shell_text.c_str());

    if (app.run_finished) {
        ImGui::PushStyleColor(ImGuiCol_Text, theme.accent);
        ImGui::TextUnformatted("--- Press any key to exit ---");
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, app.executing ? theme.title : theme.muted);
        ImGui::TextUnformatted(app.executing ? ">" : "$");
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
        if (!app.executing) {
            flags |= ImGuiInputTextFlags_ReadOnly;
        }
        if (app.focus_stdin || (app.executing && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))) {
            ImGui::SetKeyboardFocusHere();
            app.focus_stdin = false;
        }
        ImGui::SetNextItemWidth(-1.f);
        if (ImGui::InputText("##stdin_inline", &app.stdin_text, flags)) {
            actions.feed_stdin = true;
        }
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
