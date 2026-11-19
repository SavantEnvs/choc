// Shared body for the choc audio-file reader harnesses: open the input with the given
// AudioFileFormat, read its properties/metadata, then decode a bounded number of frames
// sequentially and once from a non-zero frame index (exercising seeking).
#pragma once
#include <cstdint>
#include <cstddef>
#include <memory>
#include <sstream>
#include <stdexcept>
#include "choc/audio/choc_AudioFileFormat.h"
#include "choc/text/choc_JSON.h"

namespace choc_fuzz
{
static constexpr uint32_t maxChannels = 64;        // harness's own buffer budget, not a format rule
static constexpr uint64_t maxFramesToRead = 1 << 16;
static constexpr uint32_t blockSize = 4096;

inline int fuzzAudioReader (choc::audio::AudioFileFormat& format, const uint8_t* data, size_t size)
{
    try
    {
        auto stream = std::make_shared<std::istringstream> (std::string (reinterpret_cast<const char*> (data), size));

        auto reader = format.createReader (stream);

        if (reader == nullptr)
            return 0;

        auto& props = reader->getProperties();
        (void) props.getDescription();
        (void) choc::json::toString (props.metadata, false);

        if (props.numChannels == 0 || props.numChannels > maxChannels)
            return 0;

        choc::buffer::ChannelArrayBuffer<float> buffer (choc::buffer::Size::create (props.numChannels, blockSize));
        auto framesToRead = std::min<uint64_t> (props.numFrames, maxFramesToRead);

        for (uint64_t pos = 0; pos < framesToRead; pos += blockSize)
            if (! reader->readFrames (pos, buffer.getView()))
                break;

        if (props.numFrames > 1)
        {
            choc::buffer::ChannelArrayBuffer<double> dbuffer (choc::buffer::Size::create (props.numChannels, 256u));
            (void) reader->readFrames (props.numFrames / 2, dbuffer.getView());
            (void) reader->readFrames (props.numFrames - 1, dbuffer.getView());
        }
    }
    catch (const std::exception&) {}

    return 0;
}
}
