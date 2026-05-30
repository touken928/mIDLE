#pragma once

#include <string_view>
#include <vector>

namespace midle {
namespace highlight {

enum class TokenKind {
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
    TokenKind kind = TokenKind::Default;
};

} // namespace highlight
} // namespace midle
