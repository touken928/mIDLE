#pragma once

#include "tokens.h"

namespace ImTui {
using TColor = unsigned char;
}

namespace midle {
namespace highlight {

ImTui::TColor ansi_color_for(TokenKind kind);

} // namespace highlight
} // namespace midle
