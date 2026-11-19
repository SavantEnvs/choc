// libFuzzer harness: choc::base64 decode/encode (choc/memory/choc_Base64.h), WildcardPattern
// (choc/text/choc_Wildcard.h) and string utilities (choc/text/choc_StringUtilities.h).
// The input before the first '\n' is the wildcard pattern, the rest is the text.
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include "choc/memory/choc_Base64.h"
#include "choc/text/choc_Wildcard.h"
#include "choc/text/choc_StringUtilities.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    std::string_view all (reinterpret_cast<const char*> (data), size);

    std::vector<uint8_t> decoded;
    if (choc::base64::decodeToContainer (decoded, all))
    {
        auto encoded = choc::base64::encodeToString (decoded);
        std::vector<uint8_t> again;

        if (! choc::base64::decodeToContainer (again, encoded) || again != decoded)
        {
            fprintf (stderr, "base64 round-trip mismatch\n");
            abort();
        }
    }

    auto newline = all.find ('\n');
    auto pattern = all.substr (0, newline);
    std::string text (newline == std::string_view::npos ? std::string_view() : all.substr (newline + 1));

    choc::text::WildcardPattern caseSensitive (pattern, true), caseInsensitive (pattern, false);
    (void) caseSensitive.matches (text);
    (void) caseInsensitive.matches (text);

    (void) choc::text::splitString (text, ',', true);
    (void) choc::text::splitIntoLines (text, false);
    (void) choc::text::splitAtWhitespace (text, false);
    (void) choc::text::trim (text);
    // replace() has no meaning for an empty search string (it would insert forever), so only a
    // non-empty pattern is used as one.
    if (! pattern.empty())
        (void) choc::text::replace (text, std::string (pattern), "x");
    (void) choc::text::percentEncodeURI (text);
    (void) choc::text::addDoubleQuotes (text);

    return 0;
}
