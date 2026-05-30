#pragma once

#include "highlight/tokens.h"

#include <string_view>
#include <vector>

namespace midle {
namespace languages {
namespace lua {
namespace highlight {

std::vector<midle::highlight::TokenSpan> tokenize_line(std::string_view line);

} // namespace highlight
} // namespace lua
} // namespace languages
} // namespace midle
