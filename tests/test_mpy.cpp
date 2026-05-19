#include "mpy/mpy.h"

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <thread>

using namespace midle;

class MpyTest : public ::testing::Test {
protected:
    void SetUp() override {
        int stack_top = 0;
        mpy::init(&stack_top);
    }

    void TearDown() override {
        mpy::deinit();
    }
};

TEST_F(MpyTest, InitDeinit) {
    SUCCEED();
}

TEST_F(MpyTest, ExecReturnsOutput) {
    std::string out = mpy::exec("print(42)");
    EXPECT_EQ(out, "42\n");
}

TEST_F(MpyTest, ExecMath) {
    std::string out = mpy::exec("print(3 + 4 * 5)");
    EXPECT_EQ(out, "23\n");
}

TEST_F(MpyTest, ExecMultiLine) {
    std::string out = mpy::exec(
        "for i in range(3):\n"
        "    print(i)\n"
    );
    EXPECT_EQ(out, "0\n1\n2\n");
}

TEST_F(MpyTest, ExecVariable) {
    std::string out = mpy::exec(
        "x = 10\n"
        "print(x * 2)\n"
    );
    EXPECT_EQ(out, "20\n");
}

TEST_F(MpyTest, ClearOutputClearsBuffer) {
    mpy::clear_output();
    std::string out = mpy::take_output();
    EXPECT_TRUE(out.empty());
}

TEST_F(MpyTest, RunAsyncCompletes) {
    mpy::run_async("print('async')");
    std::string output;
    for (int i = 0; i < 100 && !mpy::done(); i++) {
        std::string chunk = mpy::take_output();
        output += chunk;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::string chunk = mpy::take_output();
    output += chunk;
    EXPECT_TRUE(mpy::done());
    EXPECT_EQ(output, "async\n");
}

TEST_F(MpyTest, RunAsyncInput) {
    mpy::run_async("name = input('Name: ')\nprint('Hello', name)\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    mpy::input("World");
    std::string output;
    for (int i = 0; i < 100 && !mpy::done(); i++) {
        std::string chunk = mpy::take_output();
        output += chunk;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::string chunk = mpy::take_output();
    output += chunk;
    EXPECT_TRUE(mpy::done());
    EXPECT_TRUE(output.find("Hello World") != std::string::npos);
}

TEST_F(MpyTest, RunAsyncCloseStdin) {
    mpy::run_async("name = input('Name: ')\nprint('after', name)\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    mpy::close_stdin();
    std::string output;
    for (int i = 0; i < 100 && !mpy::done(); i++) {
        std::string chunk = mpy::take_output();
        output += chunk;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::string chunk = mpy::take_output();
    output += chunk;
    EXPECT_TRUE(mpy::done());
}

TEST_F(MpyTest, StopInterruptsExecution) {
    mpy::run_async("while True:\n    pass\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    mpy::stop();
    bool finished = false;
    for (int i = 0; i < 300; i++) {
        (void)mpy::take_output();
        if (mpy::done()) {
            finished = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(finished) << "expected stop() to end execution";
}

TEST_F(MpyTest, RunAsyncSequential) {
    mpy::run_async("print('first')");
    for (int i = 0; i < 100 && !mpy::done(); i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(mpy::done());

    mpy::run_async("print('second')");
    for (int i = 0; i < 100 && !mpy::done(); i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::string out = mpy::take_output();
    EXPECT_TRUE(mpy::done());
    EXPECT_EQ(out, "second\n");
}

TEST_F(MpyTest, ExecWithJsonModule) {
    std::string out = mpy::exec(
        "import json\n"
        "print(json.dumps({'a': 1}))\n"
    );
    EXPECT_EQ(out, "{\"a\": 1}\n");
}

TEST_F(MpyTest, ExecWithRandomModule) {
    std::string out = mpy::exec(
        "import random\n"
        "print(random.randint(1, 1))\n"
    );
    EXPECT_EQ(out, "1\n");
}

TEST_F(MpyTest, ExecWithStructModule) {
    std::string out = mpy::exec(
        "import struct\n"
        "x = struct.pack('i', 42)\n"
        "print(len(x))\n"
    );
    EXPECT_EQ(out, "4\n");
}

TEST_F(MpyTest, ExecWithHeapq) {
    std::string out = mpy::exec(
        "import heapq\n"
        "h = [3, 1, 2]\n"
        "heapq.heapify(h)\n"
        "print(heapq.heappop(h))\n"
    );
    EXPECT_EQ(out, "1\n");
}
