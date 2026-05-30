#include "runtime/runtime.h"

#include <gtest/gtest.h>

#include <optional>

using namespace midle;

TEST(RuntimeTest, LanguageFromPath) {
    EXPECT_EQ(runtime::language_from_path("script.py"), runtime::LanguageId::Python);
    EXPECT_EQ(runtime::language_from_path("app.js"), runtime::LanguageId::JavaScript);
    EXPECT_EQ(runtime::language_from_path("main.lua"), runtime::LanguageId::Lua);
    EXPECT_EQ(runtime::language_from_path(""), runtime::LanguageId::Python);
}

TEST(RuntimeTest, ResolveLanguageOverride) {
    EXPECT_EQ(runtime::resolve_language("script.py", runtime::LanguageId::JavaScript),
        runtime::LanguageId::JavaScript);
    EXPECT_EQ(runtime::resolve_language("", runtime::LanguageId::JavaScript),
        runtime::LanguageId::JavaScript);
}

TEST(RuntimeTest, LanguageModuleMetadata) {
    runtime::register_builtin_languages();
    const runtime::LanguageModule *py = runtime::find_language(runtime::LanguageId::Python);
    ASSERT_NE(py, nullptr);
    EXPECT_STREQ(py->cli_flag, "--py");
    EXPECT_NE(py->default_sample, nullptr);
    EXPECT_NE(py->tokenize_line, nullptr);
}

TEST(RuntimeTest, FacadeExec) {
    int stack_top = 0;
    runtime::init(&stack_top, runtime::LanguageId::JavaScript);
    EXPECT_EQ(runtime::exec("print('js');"), "js\n");
    runtime::deinit();
}
