// libFuzzer harness: the embedded QuickJS engine via choc::javascript::createQuickJSContext()
// (choc/javascript/choc_javascript_QuickJS.h): evaluate the input as a script and convert the
// result to a choc::value::Value. A fresh context per input, so no state carries over.
#include <cstdint>
#include <cstddef>
#include <string>
#include "choc/javascript/choc_javascript.h"
#include "choc/text/choc_JSON.h"
#include "choc/javascript/choc_javascript_QuickJS.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    try
    {
        auto context = choc::javascript::createQuickJSContext();
        auto result = context.evaluateExpression (std::string (reinterpret_cast<const char*> (data), size));
        (void) choc::json::toString (result, false);
    }
    catch (...) {}

    return 0;
}
