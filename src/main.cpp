#include "app.h"
#include "io/file_io.h"
#include "mpy/mpy.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>
#include <thread>

#ifdef __unix__
#include <unistd.h>
#endif

static void print_usage(const char *prog) {
    std::cerr << "Usage: " << prog << " [--run] <file>\n"
              << "  --run    execute script and print output, then exit\n"
              << "  <file>   open file in the editor (default: interactive mode)\n";
}

static int run_mode_exec(const std::string &source) {
    int stack_top = 0;
    midle::mpy::init(&stack_top);
    midle::mpy::run_async(source);

    const bool stdin_tty =
#ifdef __unix__
        isatty(STDIN_FILENO);
#else
        false;
#endif

    std::thread stdin_thread;
    if (stdin_tty) {
        stdin_thread = std::thread([]{
            char buf[4096];
            while (fgets(buf, sizeof(buf), stdin)) {
                size_t len = std::strlen(buf);
                if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
                midle::mpy::input(buf);
            }
            midle::mpy::close_stdin();
        });
    } else {
        midle::mpy::close_stdin();
    }

    while (!midle::mpy::done()) {
        std::string out = midle::mpy::take_output();
        if (!out.empty()) std::cout << out << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::string out = midle::mpy::take_output();
    if (!out.empty()) std::cout << out;

    midle::mpy::deinit();
    if (stdin_thread.joinable()) {
        stdin_thread.join();
    }
    return 0;
}

int main(int argc, char *argv[]) {
    bool run_mode = false;
    const char *file = nullptr;

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--run" || std::string(argv[i]) == "-r") {
            run_mode = true;
        } else if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (file == nullptr) {
            file = argv[i];
        }
    }

    if (run_mode) {
        if (!file) {
            std::cerr << "Error: --run requires a file argument\n";
            print_usage(argv[0]);
            return 1;
        }
        midle::FileLoadResult loaded = midle::load_file(file);
        if (loaded.status == midle::FileLoadStatus::NotFound) {
            std::cerr << "Error: file not found: " << file << "\n";
            return 1;
        }
        if (loaded.status == midle::FileLoadStatus::IOError) {
            std::cerr << "Error: cannot read " << file << "\n";
            return 1;
        }
        return run_mode_exec(loaded.content);
    }

    try {
        midle::App app;
        midle::app_init(app, file);

        while (app.running) {
            midle::app_frame(app);
        }

        midle::app_shutdown(app);
        return 0;
    } catch (const std::exception &ex) {
        std::cerr << "mIDLE failed: " << ex.what() << '\n';
        return 1;
    }
}
