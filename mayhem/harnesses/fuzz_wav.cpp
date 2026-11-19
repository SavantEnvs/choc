// libFuzzer harness: choc::audio::WAVAudioFileFormat reader (choc/audio/choc_AudioFileFormat_WAV.h).
#include "choc/audio/choc_AudioFileFormat_WAV.h"
#include "audio_reader_common.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    choc::audio::WAVAudioFileFormat<false> format;
    return choc_fuzz::fuzzAudioReader (format, data, size);
}
