#include "script_io.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define OUTPUT_CAP (256 * 1024)
#define STDIN_CAP 4096

static pthread_mutex_t g_mtx = PTHREAD_MUTEX_INITIALIZER;

static char   output_buf[OUTPUT_CAP];
static size_t output_len;

static char        stdin_buf[STDIN_CAP];
static size_t      stdin_rd;
static size_t      stdin_wr;
static int         stdin_closed;
static pthread_cond_t stdin_cv = PTHREAD_COND_INITIALIZER;

#define LOCK()   pthread_mutex_lock(&g_mtx)
#define UNLOCK() pthread_mutex_unlock(&g_mtx)

void script_io_write(const char *str, size_t len) {
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

void script_io_clear(void) {
    LOCK();
    output_len = 0;
    output_buf[0] = '\0';
    UNLOCK();
}

char *script_io_take(void) {
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

void script_io_stdin_reset(void) {
    LOCK();
    stdin_rd = 0;
    stdin_wr = 0;
    stdin_closed = 0;
    pthread_cond_signal(&stdin_cv);
    UNLOCK();
}

void script_io_stdin_feed(const char *str, size_t len) {
    LOCK();
    for (size_t i = 0; i < len && !stdin_closed; i++) {
        size_t next = (stdin_wr + 1) % STDIN_CAP;
        if (next == stdin_rd) {
            break;
        }
        stdin_buf[stdin_wr] = str[i];
        stdin_wr = next;
    }
    pthread_cond_signal(&stdin_cv);
    UNLOCK();
}

void script_io_stdin_close(void) {
    LOCK();
    stdin_closed = 1;
    pthread_cond_signal(&stdin_cv);
    UNLOCK();
}

int script_io_read_char(void) {
    LOCK();
    while (stdin_rd == stdin_wr && !stdin_closed) {
        pthread_cond_wait(&stdin_cv, &g_mtx);
    }
    if (stdin_closed && stdin_rd == stdin_wr) {
        UNLOCK();
        return -1;
    }
    char c = stdin_buf[stdin_rd];
    stdin_rd = (stdin_rd + 1) % STDIN_CAP;
    UNLOCK();
    return (unsigned char)c;
}
