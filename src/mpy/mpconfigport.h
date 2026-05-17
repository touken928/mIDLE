#include "port/mpconfigport_common.h"

#undef MICROPY_MPHALPORT_H
// src/ must be in the include path so "mphalport.h" resolves to src/mphalport.h
#define MICROPY_MPHALPORT_H "mpyhalport.h"

#define MICROPY_CONFIG_ROM_LEVEL          (MICROPY_CONFIG_ROM_LEVEL_MINIMUM)
#define MICROPY_ENABLE_COMPILER           (1)
#define MICROPY_ENABLE_GC                 (1)
#define MICROPY_PY_GC                     (1)
#define MICROPY_PY_SYS                    (1)
#define MICROPY_PY_MATH                   (1)
#define MICROPY_PY_BUILTINS               (1)

#define MICROPY_PY_SYS_PLATFORM           "mIDLE"
