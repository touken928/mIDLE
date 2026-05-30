#ifndef MICROPY_INCLUDED_MIDLE_PORT_H
#define MICROPY_INCLUDED_MIDLE_PORT_H

#define mp_hal_pin_obj_t void*

#ifdef __cplusplus
extern "C" {
#endif

void mp_hal_set_interrupt_char(int c);
int port_readline(struct _vstr_t *line, const char *prompt);

#ifdef __cplusplus
}
#endif

#endif
