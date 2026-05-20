#include "editor_panel.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "layout.h"

#include <algorithm>

namespace midle {
namespace ui {

void RenderEditorPanel(App &app, int width, int height) {
    const Theme &theme = GetTheme();
    const float panel_w = static_cast<float>(std::max(width, 8));
    const float panel_h = static_cast<float>(std::max(height, 4));

    // Solid field background: theme.main maps to transparent ANSI 16 and makes text hard to see.
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.field);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, theme.hovered);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, theme.selected);
    ImGui::PushStyleColor(ImGuiCol_Text, theme.title);
    ImGui::InputTextMultiline("##editor", &app.editor_text, ImVec2(panel_w, panel_h),
        ImGuiInputTextFlags_AllowTabInput);
    ImGui::PopStyleColor(4);
}

} // namespace ui
} // namespace midle
