#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void script_io_write(const char *str, size_t len);
void script_io_clear(void);
char *script_io_take(void);

void script_io_stdin_reset(void);
void script_io_stdin_feed(const char *str, size_t len);
void script_io_stdin_close(void);
int script_io_read_char(void);

#ifdef __cplusplus
}
#endif
