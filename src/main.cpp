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
        Debugger::addLog(DEBUG_LEVEL_ERROR, e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
