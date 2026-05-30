#pragma once

#include "runtime/runtime.h"

namespace ImTui {
struct TScreen;
}

namespace midle {
namespace highlight {

void ApplyEditorHighlight(ImTui::TScreen *screen, runtime::LanguageId language);

} // namespace highlight
} // namespace midle
