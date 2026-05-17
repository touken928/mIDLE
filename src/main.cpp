// mIDLE — MicroPython IDLE (ImTUI + μPython)
#include "app.h"

int main() {
    midle::App app;
    midle::app_init(app);

    while (app.running) {
        midle::app_frame(app);
    }

    midle::app_shutdown(app);
    return 0;
}
