// Runs upstream's own unit tests (tests/choc_tests.h) for the oracle. Every test function upstream's
// runner calls is kept except these, each for the reason given:
//  - testThreading: its TaskThread test asserts that callbacks arrive within a sleep-and-poll
//    wall-clock window, so it depends on host speed. This also drops the deterministic
//    ThreadSafeFunctor test in the same function.
//  - testFileUtilities: its ProcFileReading test opens /proc/self/status with std::ios::ate and
//    failbit exceptions (choc/text/choc_Files.h); procfs rejects SEEK_END, so it throws on any Linux
//    host and the uncaught exception aborts the runner. This also drops the deterministic
//    WildcardPattern test in the same function.
//  - testFileWatcher: inotify event timing.
//  - testWebview: needs a display.
//  - testHTTPServer: needs Boost (off upstream too).
#include "../tests/choc_tests.h"

int main()
{
    choc::test::TestProgress progress;
    choc::messageloop::initialise();

    std::function<void(choc::test::TestProgress&)> testFunctions[] =
    {
        choc_unit_tests::testArgumentList,
        choc_unit_tests::testZLIB,
        choc_unit_tests::testZipFile,
        choc_unit_tests::testPlatform,
        choc_unit_tests::testContainerUtils,
        choc_unit_tests::testStringUtilities,
        choc_unit_tests::testValues,
        choc_unit_tests::testJSON,
        choc_unit_tests::testMIDI,
        choc_unit_tests::testAudioBuffers,
        choc_unit_tests::testIntToFloat,
        choc_unit_tests::testFIFOs,
        choc_unit_tests::testMIDIFiles,
        choc_unit_tests::testAudioMIDIBlockDispatcher,
        choc_unit_tests::testJavascript,
        choc_unit_tests::testCOM,
        choc_unit_tests::testStableSort,
        choc_unit_tests::testAudioFileFormat,
        choc_unit_tests::testTimers,
        choc_unit_tests::testExecute,
    };

    auto t = std::thread ([&]
    {
        for (auto& fn : testFunctions)
            fn (progress);

        choc::messageloop::stop();
    });

    choc::messageloop::run();
    t.join();

    progress.printReport();
    return progress.numFails == 0 ? 0 : 1;
}

#include "../choc/javascript/choc_javascript_Duktape.h"
#include "../choc/javascript/choc_javascript_QuickJS.h"
