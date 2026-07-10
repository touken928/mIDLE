#include "languages/python/backend.h"
#include "runtime/runtime.h"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <atomic>

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

class PythonBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        int stack_top = 0;
        engine_.reset(languages::python::create_engine());
        engine_->init(&stack_top, 128 * 1024);
    }

    void TearDown() override {
        engine_->deinit();
        engine_.reset();
    }

    std::unique_ptr<runtime::ScriptEngine> engine_;
};

TEST_F(PythonBackendTest, ExecReturnsOutput) {
    EXPECT_EQ(engine_->exec("print(42)"), "42\n");
}

TEST_F(PythonBackendTest, RunAsyncInput) {
    engine_->run_async("name = input('Name: ')\nprint('Hello', name)\n");
    (void)wait_for_output(*engine_, "Name: ");
    engine_->input("World");
    const runtime::RunResult result = wait_for_terminal(*engine_);
    const std::string output = wait_for_output(*engine_, "Hello World");
    EXPECT_EQ(result.state(), runtime::RunState::Succeeded);
    EXPECT_TRUE(output.find("Hello World") != std::string::npos);
}

TEST_F(PythonBackendTest, StopInterruptsExecution) {
    engine_->run_async("while True:\n    pass\n");
    engine_->stop();
    EXPECT_EQ(wait_for_terminal(*engine_).state(), runtime::RunState::Cancelled);
}

TEST_F(PythonBackendTest, StopWhileBlockedInInputCancels) {
    engine_->run_async("input('Name: ')\n");
    ASSERT_NE(wait_for_output(*engine_, "Name: ").find("Name: "), std::string::npos);
    engine_->stop();
    EXPECT_EQ(wait_for_terminal(*engine_).state(), runtime::RunState::Cancelled);
}

TEST_F(PythonBackendTest, KeyboardInterruptInScriptIsFailed) {
    engine_->run_async("raise KeyboardInterrupt\n");
    EXPECT_EQ(wait_for_terminal(*engine_).state(), runtime::RunState::Failed);
}

TEST_F(PythonBackendTest, ExecWhilePromptBlockedIsRejectedAndAsyncRunSurvives) {
    engine_->run_async("name = input('Name: ')\nprint('Hello', name)\n");
    ASSERT_NE(wait_for_output(*engine_, "Name: ").find("Name: "), std::string::npos);
    EXPECT_EQ(engine_->exec("print('sync')\n"), "");
    engine_->input("World");
    EXPECT_EQ(wait_for_terminal(*engine_).state(), runtime::RunState::Succeeded);
    EXPECT_NE(wait_for_output(*engine_, "Hello World").find("Hello World"), std::string::npos);
}

TEST_F(PythonBackendTest, ErrorsAreFailed) {
    engine_->run_async("this is not valid Python\n");
    EXPECT_EQ(wait_for_terminal(*engine_).state(), runtime::RunState::Failed);
}

TEST_F(PythonBackendTest, RunsUseFreshStateAndCanRestartAfterCompletion) {
    engine_->run_async("first_run_value = 42\n");
    EXPECT_EQ(wait_for_terminal(*engine_).state(), runtime::RunState::Succeeded);
    engine_->run_async("print(first_run_value)\n");
    EXPECT_EQ(wait_for_terminal(*engine_).state(), runtime::RunState::Failed);
}

TEST_F(PythonBackendTest, DeinitAfterCompletionIsHarmless) {
    engine_->run_async("print('done')\n");
    EXPECT_EQ(wait_for_terminal(*engine_).state(), runtime::RunState::Succeeded);
    engine_->deinit();
    engine_->deinit();
}

TEST(PythonBackendTestStandalone, DeinitWhileBlockedInputCompletesBoundedly) {
    std::shared_ptr<runtime::ScriptEngine> engine(languages::python::create_engine());
    int stack_top = 0;
    engine->init(&stack_top, 128 * 1024);
    engine->run_async("input('Name: ')\n");
    ASSERT_NE(wait_for_output(*engine, "Name: ").find("Name: "), std::string::npos);

    std::atomic_bool finished{false};
    std::thread deinitializer([engine, &finished] {
        engine->deinit();
        finished.store(true, std::memory_order_release);
    });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!finished.load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    ASSERT_TRUE(finished.load(std::memory_order_acquire));
    deinitializer.join();
}
