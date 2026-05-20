#include "ansi_colors.h"

#include <cmath>

namespace midle {
namespace highlight {
namespace {

ImTui::TColor rgb_to_ansi256(int r, int g, int b) {
    if (r == g && g == b) {
        if (r < 8) {
            return 16;
        }
        if (r > 248) {
            return 231;
        }
        return static_cast<ImTui::TColor>(
            std::round((static_cast<float>(r - 8) / 247.0f) * 24.0f) + 232);
    }

    return static_cast<ImTui::TColor>(
        16 + (36 * std::round((static_cast<float>(r) / 255.0f) * 5.0f)) +
        (6 * std::round((static_cast<float>(g) / 255.0f) * 5.0f)) +
        std::round((static_cast<float>(b) / 255.0f) * 5.0f));
}

} // namespace

ImTui::TColor ansi_color_for(PyTokenKind kind) {
    switch (kind) {
    case PyTokenKind::Comment:
        return rgb_to_ansi256(122, 133, 153);
    case PyTokenKind::String:
        return rgb_to_ansi256(140, 199, 115);
    case PyTokenKind::Number:
        return rgb_to_ansi256(217, 166, 102);
    case PyTokenKind::Keyword:
        return rgb_to_ansi256(82, 143, 235);
    case PyTokenKind::Builtin:
        return rgb_to_ansi256(158, 204, 255);
    case PyTokenKind::Decorator:
        return rgb_to_ansi256(201, 164, 255);
    case PyTokenKind::Default:
    default:
        return 0;
    }
}

} // namespace highlight
} // namespace midle
