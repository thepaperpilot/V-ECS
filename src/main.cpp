#ifdef NDEBUG
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

#include "engine/Engine.h"
#include "engine/Debugger.h"

#include <iostream>

using namespace vecs;

int main() {
    Engine app;

    Debugger::setupLogFile("latest.log");

    try {
        app.init();
    } catch (const std::exception & e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
