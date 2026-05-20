#pragma once

#include "py_tokenizer.h"

namespace ImTui {
using TColor = unsigned char;
}

namespace midle {
namespace highlight {

ImTui::TColor ansi_color_for(PyTokenKind kind);

} // namespace highlight
} // namespace midle
