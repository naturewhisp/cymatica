#include <catch2/catch_test_macros.hpp>

#include "meter_change.h"

using namespace cymatica::cacp;

TEST_CASE("meter_change_v1: the 4/4 -> 3/4 -> 4/4 qualification passes every binding check and matches its golden",
          "[meter_change]") {
    const MeterChangeResult r = runMeterChangeV1("artifacts/cacp", false);
    INFO("failures: " << r.failures.size());
    REQUIRE(r.verdict == "PASS");
    REQUIRE(r.goldenPcmStatus == "PASS");
    REQUIRE(r.bar16Frame == 1344000);
    REQUIRE(r.tempoAck1Frame == 768000);
    REQUIRE(r.tempoAck2Frame == 1344000);
    REQUIRE(r.fallbacks == 0);
    REQUIRE(r.repeatIdentical);
}
