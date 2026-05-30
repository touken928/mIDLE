#include "languages/lua/backend.h"
#include "runtime/runtime.h"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>

using namespace midle;

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
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    engine_->input("World");
    for (int i = 0; i < 100 && !engine_->done(); i++) {
        (void)engine_->take_output();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    const std::string output = engine_->take_output();
    EXPECT_TRUE(engine_->done());
    EXPECT_TRUE(output.find("Hello\tWorld") != std::string::npos ||
                output.find("Hello World") != std::string::npos);
}

TEST_F(LuaBackendTest, StopInterruptsLoop) {
    engine_->run_async("while true do end");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    engine_->stop();
    bool finished = false;
    for (int i = 0; i < 300; i++) {
        (void)engine_->take_output();
        if (engine_->done()) {
            finished = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(finished);
}
