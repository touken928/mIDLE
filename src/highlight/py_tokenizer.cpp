#include "py_tokenizer.h"

#include <algorithm>
#include <array>
#include <string_view>

namespace midle {
namespace highlight {
namespace {

bool is_ident_start(char c) {
    return c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool is_ident(char c) {
    return is_ident_start(c) || (c >= '0' && c <= '9');
}

bool is_keyword(std::string_view word) {
    static const std::array<std::string_view, 35> keywords = {
        "False", "None", "True", "and", "as", "assert", "async", "await",
        "break", "class", "continue", "def", "del", "elif", "else", "except",
        "finally", "for", "from", "global", "if", "import", "in", "is",
        "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try",
        "while", "with", "yield",
    };
    return std::find(keywords.begin(), keywords.end(), word) != keywords.end();
}

bool is_builtin(std::string_view word) {
    static const std::array<std::string_view, 34> builtins = {
        "abs", "all", "any", "bool", "bytes", "dict", "dir", "enumerate",
        "Exception", "float", "int", "input", "len", "list",
        "map", "max", "min", "object", "open", "print", "range", "repr",
        "reversed", "round", "set", "sorted", "str", "sum", "super", "tuple",
        "type", "ValueError", "zip",
    };
    return std::find(builtins.begin(), builtins.end(), word) != builtins.end();
}

bool is_string_prefix_char(char c) {
    return c == 'f' || c == 'F' || c == 'r' || c == 'R' ||
           c == 'b' || c == 'B' || c == 'u' || c == 'U';
}

} // namespace

std::vector<TokenSpan> tokenize_line(std::string_view line) {
    std::vector<TokenSpan> spans;
    const int n = static_cast<int>(line.size());

    for (int i = 0; i < n;) {
        const char c = line[static_cast<std::size_t>(i)];

        if (c == '#') {
            spans.push_back({i, n, PyTokenKind::Comment});
            break;
        }

        if (c == '@' && i + 1 < n && is_ident_start(line[static_cast<std::size_t>(i + 1)])) {
            int j = i + 2;
            while (j < n && (is_ident(line[static_cast<std::size_t>(j)]) ||
                             line[static_cast<std::size_t>(j)] == '.')) {
                ++j;
            }
            spans.push_back({i, j, PyTokenKind::Decorator});
            i = j;
            continue;
        }

        int prefix_start = i;
        while (i < n && is_string_prefix_char(line[static_cast<std::size_t>(i)])) {
            ++i;
        }
        if (i < n && (line[static_cast<std::size_t>(i)] == '\'' ||
                      line[static_cast<std::size_t>(i)] == '"')) {
            const char quote = line[static_cast<std::size_t>(i)];
            const int start = (i > prefix_start) ? prefix_start : i;
            int j = i + 1;
            while (j < n) {
                if (line[static_cast<std::size_t>(j)] == '\\' && j + 1 < n) {
                    j += 2;
                    continue;
                }
                if (line[static_cast<std::size_t>(j)] == quote) {
                    ++j;
                    break;
                }
                ++j;
            }
            spans.push_back({start, j, PyTokenKind::String});
            i = j;
            continue;
        }
        i = prefix_start;

        if ((c >= '0' && c <= '9') ||
            (c == '.' && i + 1 < n && line[static_cast<std::size_t>(i + 1)] >= '0' &&
             line[static_cast<std::size_t>(i + 1)] <= '9')) {
            int j = i + 1;
            while (j < n) {
                const char d = line[static_cast<std::size_t>(j)];
                if ((d >= '0' && d <= '9') || d == '_' || d == '.' ||
                    d == 'x' || d == 'X' || d == 'o' || d == 'O' || d == 'b' || d == 'B' ||
                    d == 'e' || d == 'E' || d == '+' || d == '-' ||
                    (d >= 'a' && d <= 'f') || (d >= 'A' && d <= 'F')) {
                    ++j;
                    continue;
                }
                break;
            }
            spans.push_back({i, j, PyTokenKind::Number});
            i = j;
            continue;
        }

        if (is_ident_start(c)) {
            int j = i + 1;
            while (j < n && is_ident(line[static_cast<std::size_t>(j)])) {
                ++j;
            }
            const std::string_view word(line.data() + i, static_cast<std::size_t>(j - i));
            if (is_keyword(word)) {
                spans.push_back({i, j, PyTokenKind::Keyword});
            } else if (is_builtin(word)) {
                spans.push_back({i, j, PyTokenKind::Builtin});
            }
            i = j;
            continue;
        }

        ++i;
    }

    return spans;
}

} // namespace highlight
} // namespace midle
