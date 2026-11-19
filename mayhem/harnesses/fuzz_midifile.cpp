// libFuzzer harness: choc::midi::File::load (choc/audio/choc_MIDIFile.h) — Standard MIDI File
// header/track parsing, then tempo-mapped event iteration and conversion to a Sequence.
#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include "choc/audio/choc_MIDIFile.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    try
    {
        choc::midi::File file;
        file.load (data, size);

        size_t count = 0;
        file.iterateEvents ([&] (const choc::midi::LongMessage& m, double)
        {
            if (++count < 10000)
                (void) m.getDescription();
        });

        auto seq = file.toSequence();
        (void) seq;
    }
    catch (const std::exception&) {}

    return 0;
}
