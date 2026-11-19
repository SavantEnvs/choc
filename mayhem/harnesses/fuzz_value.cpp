// libFuzzer harness: choc::value::Value::deserialise (choc/containers/choc_Value.h) — the binary
// Type + Value format — then JSON conversion and a serialise/deserialise round-trip.
#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include "choc/containers/choc_Value.h"
#include "choc/text/choc_JSON.h"

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    try
    {
        choc::value::InputData in { data, data + size };
        auto v = choc::value::Value::deserialise (in);
        (void) choc::json::toString (v, true);
        (void) v.getType().getSignature (true);

        auto serialised = v.serialise();
        choc::value::InputData again { serialised.data.data(), serialised.data.data() + serialised.data.size() };
        (void) choc::value::Value::deserialise (again);
    }
    catch (const std::exception&) {}

    return 0;
}
