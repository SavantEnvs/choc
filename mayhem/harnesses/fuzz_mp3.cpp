// libFuzzer harness: choc::audio::MP3AudioFileFormat reader (choc/audio/choc_AudioFileFormat_MP3.h,
// the embedded minimp3 decoder).
#include "choc/audio/choc_AudioFileFormat_MP3.h"
#include "audio_reader_common.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    choc::audio::MP3AudioFileFormat format;
    return choc_fuzz::fuzzAudioReader (format, data, size);
}
