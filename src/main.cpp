#include "app.h"

#include <exception>
#include <iostream>

int main() {
    try {
        midle::App app;
        midle::app_init(app);

        while (app.running) {
            midle::app_frame(app);
        }

        midle::app_shutdown(app);
        return 0;
    } catch (const std::exception &ex) {
        std::cerr << "mIDLE failed: " << ex.what() << '\n';
        return 1;
    }
}
