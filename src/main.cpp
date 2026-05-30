#include "app.h"
#include "io/file_io.h"
#include "runtime/runtime.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

#ifdef __unix__
#include <unistd.h>
#endif

static void print_usage(const char *prog) {
    std::cerr << "Usage: " << prog << " [--py | --js] [--run] <file>\n"
              << "  --py     start in Python mode (default)\n"
              << "  --js     start in JavaScript mode\n"
              << "  --run    execute script and print output, then exit\n"
              << "  <file>   open file in the editor (default: interactive mode)\n"
              << "\n"
              << "Language is chosen from the file extension unless --py or --js is given.\n";
}

static int run_mode_exec(const std::string &source, midle::runtime::LanguageId language) {
    int stack_top = 0;
    midle::runtime::init(&stack_top, language);
    midle::runtime::run_async(source);

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
                midle::runtime::input(buf);
            }
            midle::runtime::close_stdin();
        });
    } else {
        midle::runtime::close_stdin();
    }

    while (!midle::runtime::done()) {
        std::string out = midle::runtime::take_output();
        if (!out.empty()) std::cout << out << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::string out = midle::runtime::take_output();
    if (!out.empty()) std::cout << out;

    midle::runtime::deinit();
    if (stdin_thread.joinable()) {
        stdin_thread.join();
    }
    return 0;
}

int main(int argc, char *argv[]) {
    bool run_mode = false;
    const char *file = nullptr;
    std::optional<midle::runtime::LanguageId> language_override;
    int cli_language_flags = 0;

    midle::runtime::register_builtin_languages();

    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg == "--run" || arg == "-r") {
            run_mode = true;
        } else if (const midle::runtime::LanguageModule *module =
                       midle::runtime::find_language_by_cli_flag(arg.c_str())) {
            language_override = module->id;
            cli_language_flags++;
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (file == nullptr) {
            file = argv[i];
        } else {
            std::cerr << "Error: unexpected argument: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    if (cli_language_flags > 1) {
        std::cerr << "Error: only one language flag (--py / --js) may be used\n";
        print_usage(argv[0]);
        return 1;
    }

    const std::string file_path = file ? file : "";
    const midle::runtime::LanguageId language =
        midle::runtime::resolve_language(file_path, language_override);

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
        return run_mode_exec(loaded.content, language);
    }

    try {
        midle::App app;
        midle::app_init(app, file, language);

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
