#include "languages/javascript/highlight/tokenizer.h"
#include "languages/python/highlight/tokenizer.h"

#include <gtest/gtest.h>

using namespace midle;

TEST(HighlightTest, PythonComment) {
    const auto spans = languages::python::highlight::tokenize_line("# comment");
    ASSERT_EQ(spans.size(), 1u);
    EXPECT_EQ(spans[0].kind, highlight::TokenKind::Comment);
}

TEST(HighlightTest, JavaScriptLineComment) {
    const auto spans = languages::javascript::highlight::tokenize_line("// comment");
    ASSERT_EQ(spans.size(), 1u);
    EXPECT_EQ(spans[0].kind, highlight::TokenKind::Comment);
}

TEST(HighlightTest, JavaScriptBuiltin) {
    const auto spans = languages::javascript::highlight::tokenize_line("print('hi');");
    bool has_builtin = false;
    for (const auto &span : spans) {
        if (span.kind == highlight::TokenKind::Builtin) {
            has_builtin = true;
        }
    }
    EXPECT_TRUE(has_builtin);
}
