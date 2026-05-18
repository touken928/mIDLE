#include "port/mpconfigport_common.h"

#undef MICROPY_MPHALPORT_H
// src/ must be in the include path so "mphalport.h" resolves to src/mphalport.h
#define MICROPY_MPHALPORT_H "mpyhalport.h"

#define MICROPY_CONFIG_ROM_LEVEL          (MICROPY_CONFIG_ROM_LEVEL_CORE_FEATURES)
#define MICROPY_ENABLE_COMPILER           (1)
#define MICROPY_ENABLE_GC                 (1)
#define MICROPY_PY_GC                     (1)
#define MICROPY_FLOAT_IMPL                (MICROPY_FLOAT_IMPL_DOUBLE)
#define MICROPY_PY_SYS                    (1)
#define MICROPY_PY_MATH                   (1)
#define MICROPY_PY_CMATH                  (1)
#define MICROPY_PY_BUILTINS               (1)
#define MICROPY_PY_BUILTINS_INPUT         (1)
#define MICROPY_PY_JSON                   (1)
#define MICROPY_PY_RANDOM                 (1)
#define MICROPY_PY_RANDOM_EXTRA_FUNCS     (1)
#define MICROPY_PY_HEAPQ                  (1)
#define MICROPY_PY_BINASCII               (1)
#define MICROPY_PY_HASHLIB                (1)
#define MICROPY_PY_COLLECTIONS_DEQUE      (1)
#define MICROPY_KBD_EXCEPTION             (1)
#define MICROPY_ENABLE_EXTERNAL_IMPORT    (0)
#define MICROPY_PY_IO                     (1)

#define mp_hal_readline                   mpy_hal_readline

#define MICROPY_PY_SYS_PLATFORM           "mIDLE"
