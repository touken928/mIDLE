#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void script_io_write(const char *str, size_t len);
void script_io_clear(void);
char *script_io_take(void);

void script_io_stdin_reset(void);
uint64_t script_io_stdin_generation(void);
void script_io_stdin_feed(const char *str, size_t len);
void script_io_stdin_close(void);
void script_io_stdin_cancel(void);
enum script_io_read_result {
    SCRIPT_IO_READ_BYTE = 1,
    SCRIPT_IO_READ_EOF = 0,
    SCRIPT_IO_READ_CANCELLED = -2
};
int script_io_read_char_generation(uint64_t generation, unsigned char *out);
int script_io_read_char(void);

#ifdef __cplusplus
}
#endif
