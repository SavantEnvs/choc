// libFuzzer harness: choc::audio::OggAudioFileFormat reader (choc/audio/choc_AudioFileFormat_Ogg.h).
#include "choc/audio/choc_AudioFileFormat_Ogg.h"
#include "audio_reader_common.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    choc::audio::OggAudioFileFormat<false> format;
    return choc_fuzz::fuzzAudioReader (format, data, size);
}
