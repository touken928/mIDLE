#include "languages/lua/backend.h"
#include "runtime/runtime.h"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>

using namespace midle;

namespace {
runtime::RunResult wait_for_terminal(runtime::ScriptEngine &engine) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline) {
        if (engine.done()) return engine.result();
        const runtime::RunResult result = engine.result();
        if (result.terminal()) return result;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return engine.result();
}
std::string wait_for_output(runtime::ScriptEngine &engine, const std::string &needle) {
    std::string output;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline) {
        output += engine.take_output();
        if (output.find(needle) != std::string::npos) return output;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return output;
}
} // namespace

class LuaBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        int stack_top = 0;
        engine_.reset(languages::lua::create_engine());
        engine_->init(&stack_top, 128 * 1024);
    }

    void TearDown() override {
        engine_->deinit();
        engine_.reset();
    }

    std::unique_ptr<runtime::ScriptEngine> engine_;
};

TEST_F(LuaBackendTest, ExecPrint) {
    EXPECT_EQ(engine_->exec("print(42)"), "42\n");
}

TEST_F(LuaBackendTest, RunAsyncPrompt) {
    engine_->run_async(
        "local name = prompt('Name: ')\n"
        "print('Hello', name)\n"
    );
    (void)wait_for_output(*engine_, "Name: ");
    engine_->input("World");
    const runtime::RunResult result = wait_for_terminal(*engine_);
    const std::string output = wait_for_output(*engine_, "Hello");
    EXPECT_EQ(result.state(), runtime::RunState::Succeeded);
    EXPECT_TRUE(output.find("Hello\tWorld") != std::string::npos ||
                output.find("Hello World") != std::string::npos);
}

TEST_F(LuaBackendTest, StopInterruptsLoop) {
    engine_->run_async("while true do end");
    engine_->stop();
    EXPECT_EQ(wait_for_terminal(*engine_).state(), runtime::RunState::Cancelled);
}

TEST_F(LuaBackendTest, StopWhileBlockedInPromptCancels) {
    engine_->run_async("prompt('Name: ')\n");
    ASSERT_NE(wait_for_output(*engine_, "Name: ").find("Name: "), std::string::npos);
    engine_->stop();
    EXPECT_EQ(wait_for_terminal(*engine_).state(), runtime::RunState::Cancelled);
}

TEST_F(LuaBackendTest, CancelledPromptDoesNotRunFollowingCode) {
    engine_->run_async("prompt('Name: ')\nprint('after')\n");
    ASSERT_NE(wait_for_output(*engine_, "Name: ").find("Name: "), std::string::npos);
    engine_->stop();
    EXPECT_EQ(wait_for_terminal(*engine_).state(), runtime::RunState::Cancelled);
    EXPECT_EQ(engine_->take_output().find("after"), std::string::npos);
}

TEST_F(LuaBackendTest, ExecWhilePromptBlockedIsRejectedAndAsyncRunSurvives) {
    engine_->run_async("local name = prompt('Name: ')\nprint('Hello', name)\n");
    ASSERT_NE(wait_for_output(*engine_, "Name: ").find("Name: "), std::string::npos);
    EXPECT_EQ(engine_->exec("print('sync')\n"), "");
    engine_->input("World");
    EXPECT_EQ(wait_for_terminal(*engine_).state(), runtime::RunState::Succeeded);
    const std::string output = wait_for_output(*engine_, "Hello");
    EXPECT_TRUE(output.find("Hello\tWorld") != std::string::npos ||
                output.find("Hello World") != std::string::npos);
}
TEST_F(LuaBackendTest, ErrorsAreFailed) {
    engine_->run_async("this is not valid Lua");
    EXPECT_EQ(wait_for_terminal(*engine_).state(), runtime::RunState::Failed);
}
TEST_F(LuaBackendTest, RunsUseFreshStateAndCanRestartAfterCompletion) {
    engine_->run_async("local first_run_value = 42\n");
    EXPECT_EQ(wait_for_terminal(*engine_).state(), runtime::RunState::Succeeded);
    engine_->run_async("print(first_run_value)\n");
    EXPECT_EQ(wait_for_terminal(*engine_).state(), runtime::RunState::Succeeded);
    EXPECT_NE(engine_->take_output().find("nil"), std::string::npos);
}
TEST_F(LuaBackendTest, DeinitAfterCompletionIsHarmless) {
    engine_->run_async("print('done')\n");
    EXPECT_EQ(wait_for_terminal(*engine_).state(), runtime::RunState::Succeeded);
    engine_->deinit();
    engine_->deinit();
}
