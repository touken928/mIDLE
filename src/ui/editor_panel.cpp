#include "editor_panel.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

namespace midle {
namespace ui {

int EditorPanel(App &app, const ImVec2 &child_size) {
    ImVec2 sz = child_size;
    if (sz.x <= 0) sz.x = 30;
    if (sz.y <= 0) sz.y = 8;

    ImGui::BeginChild("editor", sz, true, ImGuiWindowFlags_HorizontalScrollbar);

    // Transparent frame, light text
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.09f, 0.09f, 0.12f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_Text,    ImVec4(0.80f, 0.84f, 0.96f, 1.f));

    float input_h = ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing();
    if (input_h < 2.f) input_h = 2.f;

    ImGui::InputTextMultiline("##code", &app.editor_text,
        ImVec2(-1.f, input_h), ImGuiInputTextFlags_AllowTabInput);

    ImGui::PopStyleColor(2);

    // Line count
    int lines = 1;
    for (size_t i = 0; i < app.editor_text.size(); i++)
        if (app.editor_text[i] == '\n') lines++;
    ImGui::TextDisabled("Ln %d  Ch %zu", lines, app.editor_text.size());

    ImGui::EndChild();
    return lines;
}

} // namespace ui
} // namespace midle
