// libFuzzer harness: choc::text UTF-8 helpers (choc/text/choc_UTF8.h) on arbitrary
// null-terminated text. The input is copied into an exactly-sized heap buffer plus its
// terminator so ASan sees any read past the terminator (e.g. on a truncated multi-byte sequence).
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include "choc/text/choc_UTF8.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    (void) choc::text::findInvalidUTF8Data (data, size);

    std::unique_ptr<char[]> buffer (new char[size + 1]);
    memcpy (buffer.get(), data, size);
    buffer[size] = 0;

    choc::text::UTF8Pointer p (buffer.get());
    (void) p.length();
    (void) p.findEndOfLine();
    (void) p.find ("\xc3\xa9");
    (void) p.startsWith ("a");
    (void) choc::text::isValidCESU8 (std::string_view (buffer.get(), size));

    uint32_t sum = 0;
    for (auto c : p)
        sum += c;

    auto q = p;
    while (! q.empty())
        sum += q.popFirstChar();

    if (choc::text::containsSurrogatePairs (p))
        (void) choc::text::convertSurrogatePairsToUTF8 (p);

    (void) sum;
    return 0;
}
