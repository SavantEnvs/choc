// libFuzzer harness: choc::zlib::InflaterStream (choc/containers/choc_zlib.h) for all three
// framings. The first byte selects the framing (zlib / raw deflate / gzip); the rest is the stream.
#include <cstdint>
#include <cstddef>
#include <memory>
#include <sstream>
#include <stdexcept>
#include "choc/containers/choc_zlib.h"

static constexpr size_t maxOutput = 1 << 22;   // bound the work an inflate bomb can make us do

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    if (size < 1)
        return -1;

    using Format = choc::zlib::InflaterStream::FormatType;
    auto format = static_cast<Format> (data[0] % 3);

    try
    {
        auto source = std::make_shared<std::istringstream> (std::string (reinterpret_cast<const char*> (data + 1), size - 1));
        choc::zlib::InflaterStream in (source, format);

        char buffer[8192];
        size_t total = 0;

        while (total < maxOutput)
        {
            in.read (buffer, sizeof (buffer));
            auto got = in.gcount();

            if (got <= 0)
                break;

            total += static_cast<size_t> (got);
        }
    }
    catch (const std::exception&) {}

    return 0;
}
