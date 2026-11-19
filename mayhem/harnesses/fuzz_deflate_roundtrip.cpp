// libFuzzer harness: choc::zlib::DeflaterStream -> InflaterStream round-trip
// (choc/containers/choc_zlib.h). Compresses the payload with fuzzer-chosen level, window size
// and write-chunk size (optionally repeated so writes exceed the deflater's output buffer),
// decompresses it again and aborts if the bytes differ: corruption is the bug class here.
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <stdexcept>
#include "choc/containers/choc_zlib.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    if (size < 3)
        return -1;

    static const choc::zlib::DeflaterStream::CompressionLevel levels[] =
    {
        choc::zlib::DeflaterStream::none,    choc::zlib::DeflaterStream::fastest,
        choc::zlib::DeflaterStream::medium,  choc::zlib::DeflaterStream::best,
        choc::zlib::DeflaterStream::defaultLevel
    };

    auto level = levels[data[0] % 5];
    // windowBits: 0 = default zlib, 9..15 = zlib, negative = raw deflate (zlib's convention).
    // gzip (16+) is left out: InflaterStream's gzip mode fails to decode any gzip stream (even one
    // from Python's gzip module), so every gzip round-trip would report that same one defect.
    static const int windowSizes[] = { 0, 15, 12, 9, -15, -12, -9 };
    int windowBits = windowSizes[(data[1] & 0x0f) % 7];
    size_t repeats = 1u + ((data[1] >> 4) & 0x0f) * 8u;          // up to 121x, to overflow the output buffer
    size_t chunk = 1u + static_cast<size_t> (data[2]) * 257u;    // 1 .. 65536 bytes per write

    std::string payload (reinterpret_cast<const char*> (data + 3), size - 3);
    std::string original;
    original.reserve (payload.size() * repeats);
    for (size_t i = 0; i < repeats; ++i)
        original += payload;

    auto compressed = std::make_shared<std::ostringstream>();

    {
        choc::zlib::DeflaterStream out (compressed, level, windowBits);

        for (size_t pos = 0; pos < original.size(); pos += chunk)
            out.write (original.data() + pos, static_cast<std::streamsize> (std::min (chunk, original.size() - pos)));
    }

    using Format = choc::zlib::InflaterStream::FormatType;
    auto format = windowBits < 0 ? Format::deflate : Format::zlib;

    std::string decoded;

    try
    {
        auto source = std::make_shared<std::istringstream> (compressed->str());
        choc::zlib::InflaterStream in (source, format);
        char buffer[8192];

        while (decoded.size() <= original.size())
        {
            in.read (buffer, sizeof (buffer));
            auto got = in.gcount();
            if (got <= 0)
                break;
            decoded.append (buffer, static_cast<size_t> (got));
        }
    }
    catch (const std::exception& e)
    {
        fprintf (stderr, "inflating our own deflate output threw: %s\n", e.what());
        abort();
    }

    if (decoded != original)
    {
        fprintf (stderr, "deflate round-trip mismatch: %zu bytes in, %zu bytes out\n", original.size(), decoded.size());
        abort();
    }

    return 0;
}
