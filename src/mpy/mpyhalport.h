#ifndef MICROPY_INCLUDED_MPYHALPORT_H
#define MICROPY_INCLUDED_MPYHALPORT_H

#define mp_hal_pin_obj_t void*

#ifdef __cplusplus
extern "C" {
#endif

const char *mp_hal_get_output(void);
void mp_hal_clear_output(void);
char *mp_hal_take_output(void);         // returns malloced copy, caller frees

void mpy_stdin_feed(const char *str, size_t len);
void mpy_stdin_close(void);

#ifdef __cplusplus
}
#endif

#endif
