// libFuzzer harness: choc::zip::ZipFile (choc/containers/choc_ZipFile.h). Scans the central
// directory, then reads every entry (stored or deflated) through the entry's istream three ways:
// streambuf iterators, block read(), and a seek from the end.
#include <cstdint>
#include <cstddef>
#include <memory>
#include <sstream>
#include <iterator>
#include <stdexcept>
#include "choc/containers/choc_ZipFile.h"

static constexpr size_t maxBytesPerEntry = 1 << 20;   // bound the work a zip bomb can make us do

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    try
    {
        auto stream = std::make_shared<std::istringstream> (std::string (reinterpret_cast<const char*> (data), size));
        choc::zip::ZipFile zip (stream);

        for (auto& item : zip.items)
        {
            (void) item.filename;
            (void) item.getFileType();

            try
            {
                if (auto r = item.createReader())
                {
                    size_t n = 0;
                    for (std::istreambuf_iterator<char> it (*r), end; it != end && n < maxBytesPerEntry; ++it)
                        ++n;
                }

                if (auto r = item.createReader())
                {
                    char buffer[4096];
                    size_t total = 0;

                    while (total < maxBytesPerEntry && r->read (buffer, sizeof (buffer)))
                        total += sizeof (buffer);
                }

                if (auto r = item.createReader())
                {
                    r->seekg (-16, std::ios::end);
                    char buffer[32];
                    r->read (buffer, sizeof (buffer));
                    r->clear();
                    r->seekg (0, std::ios::beg);
                    r->read (buffer, sizeof (buffer));
                }
            }
            catch (const std::exception&) {}
        }
    }
    catch (const std::exception&) {}

    return 0;
}
