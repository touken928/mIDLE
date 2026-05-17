#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "py/misc.h"
#include "py/mphal.h"
#include "shared/readline/readline.h"

#ifndef __EMSCRIPTEN__
#  include <pthread.h>
#endif

// ── Shared mutex ─────────────────────────────────────────────
static pthread_mutex_t g_mtx = PTHREAD_MUTEX_INITIALIZER;

#define LOCK()   pthread_mutex_lock(&g_mtx)
#define UNLOCK() pthread_mutex_unlock(&g_mtx)

// ── Output buffer ────────────────────────────────────────────
#define OUTPUT_CAP (256 * 1024)
static char   output_buf[OUTPUT_CAP];
static size_t output_len;

void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    LOCK();
    size_t avail = OUTPUT_CAP - 1 - output_len;
    size_t n = len < avail ? len : avail;
    if (n > 0) {
        memcpy(output_buf + output_len, str, n);
        output_len += n;
        output_buf[output_len] = '\0';
    }
    UNLOCK();
}

const char *mp_hal_get_output(void) {
    return output_buf;              // hold LOCK() before calling
}

void mp_hal_clear_output(void) {
    LOCK();
    output_len = 0;
    output_buf[0] = '\0';
    UNLOCK();
}

// Thread-safe: returns a copy of the current output
char *mp_hal_take_output(void) {
    LOCK();
    size_t len = output_len;
    char *out = (char *)malloc(len + 1);
    if (out) {
        memcpy(out, output_buf, len);
        out[len] = '\0';
    }
    output_len = 0;
    output_buf[0] = '\0';
    UNLOCK();
    return out;
}

// ── Stdin ring buffer + condvar ──────────────────────────────
#define STDIN_CAP 4096
static char        stdin_buf[STDIN_CAP];
static size_t      stdin_rd;
static size_t      stdin_wr;
static int         stdin_closed;
static pthread_cond_t stdin_cv = PTHREAD_COND_INITIALIZER;

void mpy_stdin_reset(void) {
    LOCK();
    stdin_rd = 0;
    stdin_wr = 0;
    stdin_closed = 0;
    pthread_cond_signal(&stdin_cv);
    UNLOCK();
}

void mpy_stdin_feed(const char *str, size_t len) {
    LOCK();
    for (size_t i = 0; i < len && !stdin_closed; i++) {
        size_t next = (stdin_wr + 1) % STDIN_CAP;
        if (next == stdin_rd) break;
        stdin_buf[stdin_wr] = str[i];
        stdin_wr = next;
    }
    pthread_cond_signal(&stdin_cv);
    UNLOCK();
}

void mpy_stdin_close(void) {
    LOCK();
    stdin_closed = 1;
    pthread_cond_signal(&stdin_cv);
    UNLOCK();
}

int mp_hal_stdin_rx_chr(void) {
    LOCK();
    while (stdin_rd == stdin_wr && !stdin_closed)
        pthread_cond_wait(&stdin_cv, &g_mtx);
    if (stdin_closed && stdin_rd == stdin_wr) {
        UNLOCK();
        return -1;
    }
    char c = stdin_buf[stdin_rd];
    stdin_rd = (stdin_rd + 1) % STDIN_CAP;
    UNLOCK();
    return (unsigned char)c;
}

int mpy_hal_readline(vstr_t *line, const char *prompt) {
    (void)prompt;
    for (;;) {
        int c = mp_hal_stdin_rx_chr();
        if (c < 0) {
            return CHAR_CTRL_D;
        }
        if (c == CHAR_CTRL_C) {
            return CHAR_CTRL_C;
        }
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            return 0;
        }
        vstr_add_byte(line, (byte)c);
    }
}
