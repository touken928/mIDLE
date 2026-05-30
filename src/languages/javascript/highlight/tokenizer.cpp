#include "tokenizer.h"

#include <algorithm>
#include <array>
#include <string_view>

namespace midle {
namespace languages {
namespace javascript {
namespace highlight {
namespace {

bool is_ident_start(char c) {
    return c == '_' || c == '$' ||
           (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool is_ident(char c) {
    return is_ident_start(c) || (c >= '0' && c <= '9');
}

bool is_keyword(std::string_view word) {
    static const std::array<std::string_view, 39> keywords = {
        "async", "await", "break", "case", "catch", "class", "const",
        "continue", "debugger", "default", "delete", "do", "else", "export",
        "extends", "false", "finally", "for", "function", "if", "import",
        "in", "instanceof", "let", "new", "null", "return", "super",
        "switch", "this", "throw", "true", "try", "typeof", "var", "void",
        "while", "with", "yield",
    };
    return std::find(keywords.begin(), keywords.end(), word) != keywords.end();
}

bool is_builtin(std::string_view word) {
    static const std::array<std::string_view, 18> builtins = {
        "Array", "Boolean", "console", "Error", "Infinity", "JSON", "Map",
        "Math", "NaN", "Number", "Object", "print", "prompt", "Promise",
        "Set", "String", "Symbol", "undefined",
    };
    return std::find(builtins.begin(), builtins.end(), word) != builtins.end();
}

int scan_line_comment(std::string_view line, int i) {
    return static_cast<int>(line.size());
}

int scan_block_comment(std::string_view line, int i) {
    int j = i + 2;
    const int n = static_cast<int>(line.size());
    while (j + 1 < n) {
        if (line[static_cast<std::size_t>(j)] == '*' &&
            line[static_cast<std::size_t>(j + 1)] == '/') {
            return j + 2;
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
        if (quote == '`' && c == '$' && j + 1 < n &&
            line[static_cast<std::size_t>(j + 1)] == '{') {
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
    if (j + 1 < n && line[static_cast<std::size_t>(j)] == '0') {
        const char next = line[static_cast<std::size_t>(j + 1)];
        if (next == 'x' || next == 'X' || next == 'b' || next == 'B' ||
            next == 'o' || next == 'O') {
            j += 2;
            while (j < n) {
                const char d = line[static_cast<std::size_t>(j)];
                if ((d >= '0' && d <= '9') || (d >= 'a' && d <= 'f') ||
                    (d >= 'A' && d <= 'F') || d == '_') {
                    ++j;
                    continue;
                }
                break;
            }
            return j;
        }
    }
    while (j < n) {
        const char d = line[static_cast<std::size_t>(j)];
        if ((d >= '0' && d <= '9') || d == '_') {
            ++j;
            continue;
        }
        break;
    }
    if (j < n && line[static_cast<std::size_t>(j)] == '.' &&
        j + 1 < n && line[static_cast<std::size_t>(j + 1)] >= '0' &&
        line[static_cast<std::size_t>(j + 1)] <= '9') {
        ++j;
        while (j < n) {
            const char d = line[static_cast<std::size_t>(j)];
            if ((d >= '0' && d <= '9') || d == '_') {
                ++j;
                continue;
            }
            break;
        }
    }
    if (j < n && (line[static_cast<std::size_t>(j)] == 'e' ||
                  line[static_cast<std::size_t>(j)] == 'E')) {
        int k = j + 1;
        if (k < n && (line[static_cast<std::size_t>(k)] == '+' ||
                      line[static_cast<std::size_t>(k)] == '-')) {
            ++k;
        }
        while (k < n && line[static_cast<std::size_t>(k)] >= '0' &&
               line[static_cast<std::size_t>(k)] <= '9') {
            ++k;
        }
        if (k > j + 1) {
            j = k;
        }
    }
    return j;
}

} // namespace

std::vector<midle::highlight::TokenSpan> tokenize_line(std::string_view line) {
    std::vector<midle::highlight::TokenSpan> spans;
    const int n = static_cast<int>(line.size());

    for (int i = 0; i < n;) {
        const char c = line[static_cast<std::size_t>(i)];

        if (c == '/' && i + 1 < n) {
            const char next = line[static_cast<std::size_t>(i + 1)];
            if (next == '/') {
                spans.push_back({i, scan_line_comment(line, i), midle::highlight::TokenKind::Comment});
                break;
            }
            if (next == '*') {
                spans.push_back({i, scan_block_comment(line, i), midle::highlight::TokenKind::Comment});
                i = spans.back().end;
                continue;
            }
        }

        if (c == '\'' || c == '"' || c == '`') {
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
} // namespace javascript
} // namespace languages
} // namespace midle
