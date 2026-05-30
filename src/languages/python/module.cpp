#include "backend.h"
#include "highlight/tokenizer.h"
#include "runtime/core/language.h"

namespace midle {
namespace languages {
namespace python {

namespace {

static const char *kExtensions[] = {".py", ".pyw", nullptr};

static const char *kDefaultSample =
    "# Welcome to mIDLE\n"
    "name = input('Your name: ')\n"
    "print('Hello,', name)\n";

static const runtime::LanguageModule kModule = {
    runtime::LanguageId::Python,
    "Python",
    "--py",
    kExtensions,
    kDefaultSample,
    "Ready (Python)",
    create_engine,
    languages::python::highlight::tokenize_line,
};

} // namespace

const runtime::LanguageModule &language_module() {
    return kModule;
}

} // namespace python
} // namespace languages
} // namespace midle
