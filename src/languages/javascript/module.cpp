#include "backend.h"
#include "highlight/tokenizer.h"
#include "runtime/core/language.h"

namespace midle {
namespace languages {
namespace javascript {

namespace {

static const char *kExtensions[] = {".js", ".mjs", nullptr};

static const char *kDefaultSample =
    "// Welcome to mIDLE\n"
    "const name = prompt('Your name: ');\n"
    "print('Hello,', name);\n";

static const runtime::LanguageModule kModule = {
    runtime::LanguageId::JavaScript,
    "JavaScript",
    "--js",
    kExtensions,
    kDefaultSample,
    "Ready (JavaScript)",
    create_engine,
    languages::javascript::highlight::tokenize_line,
};

} // namespace

const runtime::LanguageModule &language_module() {
    return kModule;
}

} // namespace javascript
} // namespace languages
} // namespace midle
