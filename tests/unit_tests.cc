#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include "catch.hpp"

#include <limits>

#include "tools/BitSet.h"
#include "tools/math.h"
#include "tools/Random.h"
#include "tools/MatchBin.h"
#include "tools/matchbin_utils.h"

#include "AltSignalWorld.h"
#include "AltSignalConfig.h"

TEST_CASE( "Hello World", "[general]" ) {
  std::cout << "Hello tests!" << std::endl;
}

/*
TEST_CASE( "Figuring Out Ranked Selector Thresholds", "[general]" ) {
  constexpr size_t TAG_WIDTH = 16;
  using tag_t = emp::BitSet<TAG_WIDTH>;
  emp::HammingMetric<TAG_WIDTH> hamming_metric;
  emp::Random random(2);

  emp::vector<tag_t> tags;
  for (size_t i = 0; i < emp::Pow(2, TAG_WIDTH); ++i) {
    tags.emplace_back();
    tags.back().SetUInt(0, i);
  }

  // Create a couple of match bins!
  emp::MatchBin<size_t,
                emp::HammingMetric<TAG_WIDTH>,
                emp::RankedSelector<>> matchbin_0(random); // 0% min threshold

  emp::MatchBin<size_t,
                emp::HammingMetric<TAG_WIDTH>,
                emp::RankedSelector<std::ratio<TAG_WIDTH+(TAG_WIDTH/2), TAG_WIDTH>>
               > matchbin_1(random); // 50% min threshold

  for (size_t i = 0; i < tags.size(); ++i) {
    std::cout << "=== i = " << i << " ===" << std::endl;
    // Clear the matchbins.
    matchbin_0.Clear();
    matchbin_1.Clear();
    emp::vector<size_t> matches;
    // Add focal tag to matchbin.
    matchbin_0.Set(i, tags[i], i);
    matchbin_1.Set(i, tags[i], i);
    std::cout << "Focal tag = "; tags[i].Print(); std::cout << std::endl;
    // Does 0000 match with focal tag?
    std::cout << "Matchbin 0" << std::endl;
    matches = matchbin_0.Match(tags[0], 1);
    if (matches.size()) {
      std::cout << "  "; tags[0].Print();
      std::cout << " matched with "; tags[i].Print();
      std::cout << std::endl;
    } else {
      std::cout << "  "; tags[0].Print();
      std::cout << " DID NOT MATCH WITH "; tags[i].Print();
      std::cout << std::endl;
    }
    std::cout << "Matchbin 1" << std::endl;
    matches = matchbin_1.Match(tags[0], 1);
    if (matches.size()) {
      std::cout << "  "; tags[0].Print();
      std::cout << " matched with "; tags[i].Print();
      std::cout << std::endl;
    } else {
      std::cout << "  "; tags[0].Print();
      std::cout << " DID NOT MATCH WITH "; tags[i].Print();
      std::cout << std::endl;
    }
  }

}
*/

TEST_CASE( "AltSignalWorld ") {
  AltSignalConfig config;
  config.SEED(2);
  config.POP_SIZE(100);
  config.GENERATIONS(100);

  emp::Random random(config.SEED());
  AltSignalWorld world(random);

  world.Setup(config);

}