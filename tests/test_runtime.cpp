#include "runtime/runtime.h"
#include "runtime/core/async_runner.h"

#include <gtest/gtest.h>

#include <optional>
#include <chrono>
#include <thread>

using namespace midle;

namespace {
runtime::RunResult wait_for_terminal() {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline) {
        if (runtime::done()) return runtime::result();
        const runtime::RunResult result = runtime::result();
        if (result.terminal()) return result;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return runtime::result();
}

runtime::RunResult wait_for_terminal(runtime::AsyncRunner &runner) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline) {
        if (runner.done()) return runner.result();
        const runtime::RunResult result = runner.result();
        if (result.terminal()) return result;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return runner.result();
}
} // namespace

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

TEST(RuntimeTest, FacadeExposesAsyncStateAndResult) {
    int stack_top = 0;
    runtime::init(&stack_top, runtime::LanguageId::JavaScript);
    runtime::run_async("print('facade');");
    const runtime::RunResult result = wait_for_terminal();
    EXPECT_EQ(runtime::state(), result.state());
    EXPECT_EQ(result.state(), runtime::RunState::Succeeded);
    EXPECT_TRUE(result.terminal());
    runtime::deinit();
}

TEST(RuntimeTest, AsyncRunnerStopAfterTerminalDoesNotBecomeStopping) {
    runtime::AsyncRunner runner;
    runner.set_task([](const std::string &, const runtime::StopToken &) {
        return runtime::RunResult(runtime::RunState::Succeeded);
    });
    runner.run_async("first");
    EXPECT_EQ(wait_for_terminal(runner).state(), runtime::RunState::Succeeded);
    runner.request_stop();
    EXPECT_EQ(runner.state(), runtime::RunState::Succeeded);
    EXPECT_EQ(runner.result().state(), runtime::RunState::Succeeded);
}

TEST(RuntimeTest, AsyncRunnerNormalizesNonterminalTaskResultToFailed) {
    runtime::AsyncRunner runner;
    runner.set_task([](const std::string &, const runtime::StopToken &) {
        return runtime::RunResult(runtime::RunState::Running);
    });
    runner.run_async("nonterminal");
    const runtime::RunResult result = wait_for_terminal(runner);
    EXPECT_EQ(result.state(), runtime::RunState::Failed);
    EXPECT_NE(result.diagnostic().find("nonterminal"), std::string::npos);
}

TEST(RuntimeTest, AsyncRunnerReapsCompletedGenerationBeforeNextStart) {
    runtime::AsyncRunner runner;
    runner.set_task([](const std::string &source, const runtime::StopToken &) {
        return runtime::RunResult(source == "one" || source == "two"
                                      ? runtime::RunState::Succeeded
                                      : runtime::RunState::Failed);
    });
    runner.run_async("one");
    EXPECT_EQ(wait_for_terminal(runner).state(), runtime::RunState::Succeeded);
    EXPECT_TRUE(runner.try_run_async("two"));
    EXPECT_EQ(wait_for_terminal(runner).state(), runtime::RunState::Succeeded);
}

TEST(RuntimeTest, AsyncRunnerTerminalStateAndResultStayCoherent) {
    runtime::AsyncRunner runner;
    runner.set_task([](const std::string &, const runtime::StopToken &) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        return runtime::RunResult(runtime::RunState::Succeeded);
    });
    runner.run_async("coherent");

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    bool terminal = false;
    while (std::chrono::steady_clock::now() < deadline) {
        const runtime::RunResult result = runner.result();
        const runtime::RunState state = runner.state();
        if (result.terminal()) {
            EXPECT_TRUE(state == runtime::RunState::Succeeded ||
                        state == runtime::RunState::Failed ||
                        state == runtime::RunState::Cancelled);
            EXPECT_EQ(state, result.state());
            terminal = true;
            break;
        }
        EXPECT_FALSE(state == runtime::RunState::Succeeded ||
                     state == runtime::RunState::Failed ||
                     state == runtime::RunState::Cancelled);
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
    ASSERT_TRUE(terminal);
    EXPECT_TRUE(runner.done());
    EXPECT_EQ(runner.state(), runner.result().state());
}

TEST(RuntimeTest, AsyncRunnerStopRaceLeavesEveryGenerationTerminal) {
    runtime::AsyncRunner runner;
    runner.set_task([](const std::string &, const runtime::StopToken &) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        return runtime::RunResult(runtime::RunState::Succeeded);
    });

    for (int iteration = 0; iteration < 100; ++iteration) {
        ASSERT_TRUE(runner.try_run_async("race"));
        runner.request_stop();
        const runtime::RunResult result = wait_for_terminal(runner);
        ASSERT_TRUE(result.terminal()) << "iteration " << iteration;
        EXPECT_TRUE(runner.state() == runtime::RunState::Succeeded ||
                    runner.state() == runtime::RunState::Cancelled);
        EXPECT_EQ(runner.state(), result.state()) << "iteration " << iteration;
    }

    ASSERT_TRUE(runner.try_run_async("next-generation"));
    EXPECT_TRUE(wait_for_terminal(runner).terminal());
    EXPECT_EQ(runner.state(), runner.result().state());
}
