#include <numeric>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <pmt.h>

#include "dummy/Dummy.h"

/**
 * Verify default behavior of the Dummy sensor
 *
 * Test validates that:
 * 1. Default sensor returns measurements with zero values
 * 2. Time advances between measurements
 * 3. Power and energy calculations return zero
 */
TEST_CASE("Default") {
  const size_t kSleepMs = GENERATE(1, 1e2, 1e3);

  // Initialize
  auto sensor = pmt::Dummy::Create();

  // Take two measurements
  pmt::State state1 = sensor->Read();
  std::this_thread::sleep_for(std::chrono::milliseconds(kSleepMs));
  pmt::State state2 = sensor->Read();

  // Verify each state has expected default values
  for (const pmt::State& state : {state1, state2}) {
    REQUIRE(state.NrMeasurements() == 1);  // Single measurement point
    REQUIRE(state.watts(0) == 0);          // Zero power consumption
    REQUIRE(state.joules(0) == 0);         // Zero energy consumption
  }

  // Verify time-based calculations
  CHECK(pmt::PMT::seconds(state1, state2) > 0);  // Time advances
  CHECK_THAT(pmt::PMT::watts(state1, state2),    // Power remains zero
             Catch::Matchers::WithinULP(0.f, 0));
  CHECK_THAT(pmt::PMT::joules(state1, state2),  // Energy remains zero
             Catch::Matchers::WithinULP(0.f, 0));
}

/**
 * Verify the Dummy sensor reporting power
 * - The Dummy sensor is configured to report a fixed power value
 *
 * Test validates that:
 * 1. Sensor correctly reports configured power value
 * 2. Power calculation from energy matches configured value
 */
TEST_CASE("Set watt") {
  const float kWatt = 42;
  const size_t kSleepMs = GENERATE(1, 1e2, 1e3);

  // Initialize
  auto sensor = pmt::Dummy::Create();
  sensor->UseFixedWatts({kWatt});

  // Take two measurements
  pmt::State state1 = sensor->Read();
  std::this_thread::sleep_for(std::chrono::milliseconds(kSleepMs));
  pmt::State state2 = sensor->Read();

  // Verify returned power
  const float watt = pmt::PMT::watts(state1, state2);
  CHECK_THAT(watt, Catch::Matchers::WithinAbs(kWatt, 1.f));

  // Verify power calculation from energy
  const float joules = pmt::PMT::joules(state1, state2);
  const float seconds = pmt::PMT::seconds(state1, state2);
  REQUIRE(seconds > 0);
  CHECK_THAT(joules / seconds, Catch::Matchers::WithinAbs(kWatt, 1.f));
}

/**
 * Verify the Dummy sensor reporting energy
 * - The Dummy sensor is configured to report a fixed energy value
 *
 * Test validates that:
 * 1. Sensor correctly reports configured power value
 * 2. Power calculation from energy matches configured value
 */
TEST_CASE("Set joules") {
  const float kWatt = 42;
  const size_t kSleepMs = GENERATE(1, 1e2, 1e3);

  // Initialize
  auto sensor = pmt::Dummy::Create();
  sensor->UseFixedJoules({kWatt});

  // Take two measurements
  pmt::State state1 = sensor->Read();
  std::this_thread::sleep_for(std::chrono::milliseconds(kSleepMs));
  pmt::State state2 = sensor->Read();

  // Verify returned power
  const float watt = pmt::PMT::watts(state1, state2);
  CHECK_THAT(watt, Catch::Matchers::WithinAbs(kWatt, 1.f));

  // Verify power calculation from energy
  const float joules = pmt::PMT::joules(state1, state2);
  const float seconds = pmt::PMT::seconds(state1, state2);
  REQUIRE(seconds > 0);
  CHECK_THAT(joules / seconds, Catch::Matchers::WithinAbs(kWatt, 1.f));
}

/**
 * Verify the Dummy sensor reporting power
 * - The Dummy sensor is configured to report a fixed power value
 *
 * Test validates that:
 * 1. Sensor correctly reports sum of configured power values
 * 2. Power calculation from energy matches sum of configured values
 * 3. Individual sensor values are correctly reported
 */
TEST_CASE("Set watt for 2 sensors") {
  const std::vector<float> kWatt{{42, 420}};
  const float kWattSum = std::accumulate(kWatt.begin(), kWatt.end(), 0.f);
  const size_t kSleepMs = GENERATE(1, 1e2, 1e3);

  // Initialize
  auto sensor = pmt::Dummy::Create();
  sensor->UseFixedWatts(kWatt);

  // Take two measurements
  pmt::State state1 = sensor->Read();
  std::this_thread::sleep_for(std::chrono::milliseconds(kSleepMs));
  pmt::State state2 = sensor->Read();

  // Verify returned power
  const float watt = pmt::PMT::watts(state1, state2);
  CHECK_THAT(watt, Catch::Matchers::WithinAbs(kWattSum, 1.f));

  // Verify power calculation from energy
  const float joules = pmt::PMT::joules(state1, state2);
  const float seconds = pmt::PMT::seconds(state1, state2);
  REQUIRE(seconds > 0);
  CHECK_THAT(joules / seconds, Catch::Matchers::WithinAbs(kWattSum, 1.f));

  // Verify individual sensor values
  for (size_t i = 0; i < kWatt.size(); ++i) {
    CHECK_THAT(state1.watts(i + 1), Catch::Matchers::WithinAbs(kWatt[i], 1.f));
    CHECK_THAT(state2.watts(i + 1), Catch::Matchers::WithinAbs(kWatt[i], 1.f));
  }
}