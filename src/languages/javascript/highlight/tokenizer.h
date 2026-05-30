#pragma once

#include "highlight/tokens.h"

#include <string_view>
#include <vector>

namespace midle {
namespace languages {
namespace javascript {
namespace highlight {

std::vector<midle::highlight::TokenSpan> tokenize_line(std::string_view line);

} // namespace highlight
} // namespace javascript
} // namespace languages
} // namespace midle
