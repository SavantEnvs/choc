// libFuzzer harness: choc::json::parse / parseValue (choc/text/choc_JSON.h), plus a
// toString -> parse round-trip of whatever parsed. The input is held in a std::string: parse() hands
// the view's pointer to a UTF8Pointer, which reads up to a NUL terminator, so a bare string_view over
// unterminated bytes is over-read on every number at the end of input (reported separately).
#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <stdexcept>
#include "choc/text/choc_JSON.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    const std::string text (reinterpret_cast<const char*> (data), size);

    try
    {
        auto v = choc::json::parse (text);
        auto pretty = choc::json::toString (v, true);
        auto compact = choc::json::toString (v, false);
        (void) choc::json::parse (pretty);
        (void) choc::json::parse (compact);
    }
    catch (const std::exception&) {}

    try
    {
        auto v = choc::json::parseValue (text);
        (void) choc::json::toString (v, false);
    }
    catch (const std::exception&) {}

    return 0;
}
