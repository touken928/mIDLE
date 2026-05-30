#include "languages/register.h"
#include "runtime/core/language.h"

namespace midle {
namespace runtime {

void register_builtin_languages() {
    static bool registered = false;
    if (registered) {
        return;
    }
    register_language(languages::python::language_module());
    register_language(languages::javascript::language_module());
    registered = true;
}

} // namespace runtime
} // namespace midle
