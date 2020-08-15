#ifdef NDEBUG
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

#include "engine/Engine.h"
#include "engine/Debugger.h"

#include <iostream>

#ifdef WIN32
#include <Windows.h>
#endif

using namespace vecs;

int main() {
    Engine app;

    Debugger::setupLogFile("latest.log");

#ifdef WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    // Build string of hardware information
    std::stringstream ss;
    ss << "Hardware Information\n";
    ss << "\tOEM ID: " << sysInfo.dwOemId << "\n";
    ss << "\tNumber of processors: " << sysInfo.dwNumberOfProcessors << "\n";
    ss << "\tPage size: " << sysInfo.dwPageSize << "\n";
    ss << "\tProcessor type: " << sysInfo.dwProcessorType << "\n";
    ss << "\tMinimum application address: " << std::hex << sysInfo.lpMinimumApplicationAddress << "\n";
    ss << "\tMaximum application address: " << std::hex << sysInfo.lpMaximumApplicationAddress << "\n";
    ss << "\tActive processor mask: " << std::dec << sysInfo.dwActiveProcessorMask;

    // Add to debug log
    Debugger::addLog(DEBUG_LEVEL_VERBOSE, ss.str());
#endif


    try {
        app.init();
    } catch (const std::exception & e) {
        Debugger::addLog(DEBUG_LEVEL_ERROR, e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
