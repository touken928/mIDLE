#pragma once

#include <string_view>
#include <vector>

namespace midle {
namespace highlight {

enum class PyTokenKind {
    Default,
    Comment,
    String,
    Number,
    Keyword,
    Builtin,
    Decorator,
};

struct TokenSpan {
    int start = 0;
    int end = 0;
    PyTokenKind kind = PyTokenKind::Default;
};

std::vector<TokenSpan> tokenize_line(std::string_view line);

} // namespace highlight
} // namespace midle
