#include "tokenizer.h"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

namespace midle {
namespace languages {
namespace lua {
namespace highlight {
namespace {

bool is_ident_start(char c) {
    return c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool is_ident(char c) {
    return is_ident_start(c) || (c >= '0' && c <= '9');
}

bool is_keyword(std::string_view word) {
    static const std::array<std::string_view, 22> keywords = {
        "and", "break", "do", "else", "elseif", "end", "false", "for",
        "function", "goto", "if", "in", "local", "nil", "not", "or",
        "repeat", "return", "then", "true", "until", "while",
    };
    return std::find(keywords.begin(), keywords.end(), word) != keywords.end();
}

bool is_builtin(std::string_view word) {
    static const std::array<std::string_view, 12> builtins = {
        "assert", "error", "ipairs", "math", "pairs", "print", "prompt",
        "string", "table", "tonumber", "tostring", "type",
    };
    return std::find(builtins.begin(), builtins.end(), word) != builtins.end();
}

int scan_line_comment(std::string_view line, int i) {
    return static_cast<int>(line.size());
}

int scan_bracket_literal(std::string_view line, int i) {
    const int n = static_cast<int>(line.size());
    if (i + 1 >= n || line[static_cast<std::size_t>(i)] != '[') {
        return i + 1;
    }
    int j = i + 1;
    int eq = 0;
    while (j < n && line[static_cast<std::size_t>(j)] == '=') {
        ++eq;
        ++j;
    }
    if (j >= n || line[static_cast<std::size_t>(j)] != '[') {
        return i + 1;
    }
    ++j;
    const std::string close = std::string("]") + std::string(eq, '=') + "]";
    while (j < n) {
        if (static_cast<std::size_t>(j) + close.size() <= line.size() &&
            line.substr(static_cast<std::size_t>(j), close.size()) == close) {
            return j + static_cast<int>(close.size());
        }
        ++j;
    }
    return n;
}

int scan_string(std::string_view line, int i, char quote) {
    int j = i + 1;
    const int n = static_cast<int>(line.size());
    while (j < n) {
        const char c = line[static_cast<std::size_t>(j)];
        if (c == '\\' && j + 1 < n) {
            j += 2;
            continue;
        }
        if (c == quote) {
            return j + 1;
        }
        ++j;
    }
    return n;
}

int scan_number(std::string_view line, int i) {
    int j = i;
    const int n = static_cast<int>(line.size());
    if (j + 1 < n && line[static_cast<std::size_t>(j)] == '0' &&
        (line[static_cast<std::size_t>(j + 1)] == 'x' ||
         line[static_cast<std::size_t>(j + 1)] == 'X')) {
        j += 2;
        while (j < n) {
            const char d = line[static_cast<std::size_t>(j)];
            if ((d >= '0' && d <= '9') || (d >= 'a' && d <= 'f') ||
                (d >= 'A' && d <= 'F')) {
                ++j;
                continue;
            }
            break;
        }
        return j;
    }
    while (j < n) {
        const char d = line[static_cast<std::size_t>(j)];
        if ((d >= '0' && d <= '9') || d == '.') {
            ++j;
            continue;
        }
        break;
    }
    return j;
}

} // namespace

std::vector<midle::highlight::TokenSpan> tokenize_line(std::string_view line) {
    std::vector<midle::highlight::TokenSpan> spans;
    const int n = static_cast<int>(line.size());

    for (int i = 0; i < n;) {
        const char c = line[static_cast<std::size_t>(i)];

        if (c == '-' && i + 1 < n && line[static_cast<std::size_t>(i + 1)] == '-') {
            if (i + 2 < n && line[static_cast<std::size_t>(i + 2)] == '[') {
                spans.push_back({i, scan_bracket_literal(line, i + 2), midle::highlight::TokenKind::Comment});
                i = spans.back().end;
                continue;
            }
            spans.push_back({i, scan_line_comment(line, i), midle::highlight::TokenKind::Comment});
            break;
        }

        if (c == '[' && i + 1 < n &&
            (line[static_cast<std::size_t>(i + 1)] == '[' ||
             line[static_cast<std::size_t>(i + 1)] == '=')) {
            spans.push_back({i, scan_bracket_literal(line, i), midle::highlight::TokenKind::String});
            i = spans.back().end;
            continue;
        }

        if (c == '\'' || c == '"') {
            spans.push_back({i, scan_string(line, i, c), midle::highlight::TokenKind::String});
            i = spans.back().end;
            continue;
        }

        if ((c >= '0' && c <= '9') ||
            (c == '.' && i + 1 < n && line[static_cast<std::size_t>(i + 1)] >= '0' &&
             line[static_cast<std::size_t>(i + 1)] <= '9')) {
            spans.push_back({i, scan_number(line, i), midle::highlight::TokenKind::Number});
            i = spans.back().end;
            continue;
        }

        if (is_ident_start(c)) {
            int j = i + 1;
            while (j < n && is_ident(line[static_cast<std::size_t>(j)])) {
                ++j;
            }
            const std::string_view word(line.data() + i, static_cast<std::size_t>(j - i));
            if (is_keyword(word)) {
                spans.push_back({i, j, midle::highlight::TokenKind::Keyword});
            } else if (is_builtin(word)) {
                spans.push_back({i, j, midle::highlight::TokenKind::Builtin});
            }
            i = j;
            continue;
        }

        ++i;
    }

    return spans;
}

} // namespace highlight
} // namespace lua
} // namespace languages
} // namespace midle
