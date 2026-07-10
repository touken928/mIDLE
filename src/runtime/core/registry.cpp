#include "language.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace midle {
namespace runtime {
namespace {

std::vector<const LanguageModule *> g_languages;
LanguageId s_active = LanguageId::Python;
std::unique_ptr<ScriptEngine> s_engine;
bool s_initialized = false;

bool extension_matches(const std::string &pattern, const std::string &ext_lower) {
    return ext_lower == pattern;
}

const LanguageModule *find_by_extension_lower(const std::string &ext_lower) {
    for (const LanguageModule *module : g_languages) {
        for (const char *const *ext = module->file_extensions; ext && *ext; ++ext) {
            std::string pattern = *ext;
            std::transform(pattern.begin(), pattern.end(), pattern.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (extension_matches(pattern, ext_lower)) {
                return module;
            }
        }
    }
    return nullptr;
}

ScriptEngine *active_engine() {
    return s_engine.get();
}

} // namespace

void register_language(const LanguageModule &module) {
    g_languages.push_back(&module);
}

const LanguageModule *find_language(LanguageId id) {
    for (const LanguageModule *module : g_languages) {
        if (module->id == id) {
            return module;
        }
    }
    return nullptr;
}

const LanguageModule *find_language_by_extension(const std::string &path) {
    const auto dot = path.rfind('.');
    if (dot == std::string::npos || dot + 1 >= path.size()) {
        return nullptr;
    }
    std::string ext = path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return find_by_extension_lower(ext);
}

const LanguageModule *find_language_by_cli_flag(const char *flag) {
    register_builtin_languages();
    for (const LanguageModule *module : g_languages) {
        if (module->cli_flag && flag && std::string(module->cli_flag) == flag) {
            return module;
        }
    }
    return nullptr;
}

const LanguageModule *default_language() {
    return find_language(LanguageId::Python);
}

LanguageId language_from_path(const std::string &path) {
    register_builtin_languages();
    if (const LanguageModule *module = find_language_by_extension(path)) {
        return module->id;
    }
    return LanguageId::Python;
}

LanguageId resolve_language(const std::string &path, std::optional<LanguageId> override_language) {
    register_builtin_languages();
    if (override_language.has_value()) {
        return *override_language;
    }
    return language_from_path(path);
}

const char *language_name(LanguageId id) {
    register_builtin_languages();
    if (const LanguageModule *module = find_language(id)) {
        return module->display_name;
    }
    return "Script";
}

LanguageId active_language() {
    return s_active;
}

const LanguageModule *active_language_module() {
    register_builtin_languages();
    return find_language(s_active);
}

void init(void *stack_top, LanguageId language, size_t heap_bytes) {
    register_builtin_languages();
    if (s_initialized) {
        deinit();
    }
    const LanguageModule *module = find_language(language);
    if (!module) {
        module = default_language();
        language = module->id;
    }
    s_active = language;
    s_engine.reset(module->create_engine());
    s_engine->init(stack_top, heap_bytes);
    s_initialized = true;
}

void deinit() {
    if (!s_initialized || !s_engine) {
        return;
    }
    s_engine->deinit();
    s_engine.reset();
    s_initialized = false;
}

void run_async(const std::string &source) {
    if (active_engine()) {
        active_engine()->run_async(source);
    }
}

std::string take_output() {
    return active_engine() ? active_engine()->take_output() : std::string();
}

bool done() {
    return active_engine() ? active_engine()->done() : true;
}

RunState state() {
    return active_engine() ? active_engine()->state() : RunState::Idle;
}

RunResult result() {
    return active_engine() ? active_engine()->result() : RunResult();
}

void input(const std::string &text) {
    if (active_engine()) {
        active_engine()->input(text);
    }
}

void close_stdin() {
    if (active_engine()) {
        active_engine()->close_stdin();
    }
}

void stop() {
    if (active_engine()) {
        active_engine()->stop();
    }
}

std::string exec(const std::string &source) {
    return active_engine() ? active_engine()->exec(source) : std::string();
}

void clear_output() {
    if (active_engine()) {
        active_engine()->clear_output();
    }
}

} // namespace runtime
} // namespace midle
