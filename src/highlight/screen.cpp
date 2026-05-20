#include "screen.h"

#include "ansi_colors.h"
#include "py_tokenizer.h"

#include "imtui/imtui.h"

#include <algorithm>
#include <cstdint>
#include <string>

namespace midle {
namespace highlight {
namespace {

char cell_char(ImTui::TCell cell) {
    const uint16_t c = cell & 0x0000FFFF;
    return c > 0 && c < 128 ? static_cast<char>(c) : ' ';
}

void set_cell_fg(ImTui::TCell &cell, ImTui::TColor fg) {
    cell &= 0xFF00FFFF;
    cell |= (static_cast<ImTui::TCell>(fg) << 16);
}

void highlight_row(ImTui::TScreen *screen, int y) {
    std::string line;
    line.reserve(static_cast<std::size_t>(screen->nx));

    for (int x = 0; x < screen->nx; ++x) {
        line.push_back(cell_char(screen->data[y * screen->nx + x]));
    }

    const auto spans = tokenize_line(line);
    for (const auto &span : spans) {
        const ImTui::TColor color = ansi_color_for(span.kind);
        if (color == 0) {
            continue;
        }
        for (int x = std::max(0, span.start); x < std::min(screen->nx, span.end); ++x) {
            if (line[static_cast<std::size_t>(x)] != ' ') {
                set_cell_fg(screen->data[y * screen->nx + x], color);
            }
        }
    }
}

} // namespace

void ApplyPythonEditorHighlight(ImTui::TScreen *screen) {
    if (!screen || !screen->data || screen->nx <= 0 || screen->ny <= 2) {
        return;
    }

    // Row 0: title bar; last row: status bar.
    for (int y = 1; y < screen->ny - 1; ++y) {
        highlight_row(screen, y);
    }
}

} // namespace highlight
} // namespace midle
