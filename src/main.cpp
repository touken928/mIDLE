#include "app.h"
#include "mpy/mpy.h"

#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static std::string load_file(const char *path) {
    std::ifstream f(path);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void print_usage(const char *prog) {
    std::cerr << "Usage: " << prog << " [--run] <file>\n"
              << "  --run    execute script and print output, then exit\n"
              << "  <file>   open file in the editor (default: interactive mode)\n";
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
        std::string source = load_file(file);
        if (source.empty()) {
            std::cerr << "Error: cannot read " << file << "\n";
            return 1;
        }
        int stack_top = 0;
        midle::mpy::init(&stack_top);
        std::string output = midle::mpy::exec(source);
        midle::mpy::deinit();
        std::cout << output;
        return 0;
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
