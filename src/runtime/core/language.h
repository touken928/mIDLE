#pragma once

#include "runtime/core/script_engine.h"
#include "highlight/tokens.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace midle {
namespace runtime {

enum class LanguageId {
    Python,
    JavaScript,
};

struct LanguageModule {
    LanguageId id;
    const char *display_name;
    const char *cli_flag;
    const char *const *file_extensions;
    const char *default_sample;
    const char *ready_status;

    ScriptEngine *(*create_engine)();
    std::vector<highlight::TokenSpan> (*tokenize_line)(std::string_view line);
};

void register_language(const LanguageModule &module);
void register_builtin_languages();

const LanguageModule *find_language(LanguageId id);
const LanguageModule *find_language_by_extension(const std::string &path);
const LanguageModule *find_language_by_cli_flag(const char *flag);
const LanguageModule *default_language();

LanguageId language_from_path(const std::string &path);
LanguageId resolve_language(const std::string &path, std::optional<LanguageId> override_language);
const char *language_name(LanguageId id);

LanguageId active_language();
const LanguageModule *active_language_module();

void init(void *stack_top, LanguageId language, size_t heap_bytes = 128 * 1024);
void deinit();

void run_async(const std::string &source);
std::string take_output();
bool done();
void input(const std::string &text);
void close_stdin();
void stop();
std::string exec(const std::string &source);
void clear_output();

} // namespace runtime
} // namespace midle
