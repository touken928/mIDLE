#include "backend.h"
#include "highlight/tokenizer.h"
#include "runtime/core/language.h"

namespace midle {
namespace languages {
namespace lua {

namespace {

static const char *kExtensions[] = {".lua", nullptr};

static const char *kDefaultSample =
    "-- Welcome to mIDLE\n"
    "local name = prompt('Your name: ')\n"
    "print('Hello,', name)\n";

static const runtime::LanguageModule kModule = {
    runtime::LanguageId::Lua,
    "Lua",
    "--lua",
    kExtensions,
    kDefaultSample,
    "Ready (Lua)",
    create_engine,
    languages::lua::highlight::tokenize_line,
};

} // namespace

const runtime::LanguageModule &language_module() {
    return kModule;
}

} // namespace lua
} // namespace languages
} // namespace midle
