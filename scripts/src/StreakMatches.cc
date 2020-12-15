
#include "emp/math/Random.hpp"
#include "emp/math/Range.hpp"
#include "emp/matchbin/MatchBin.hpp"
#include "emp/matchbin/matchbin_metrics.hpp"
#include "emp/bits/BitSet.hpp"

#include <algorithm>

constexpr size_t tag_width = 256;
constexpr size_t num_pairs = 100000;
constexpr double interval = 0.90;   // Value between 0 and 1

using tag_t = emp::BitSet<tag_width>;

int main() {
  emp::Random random(2);
  emp::StreakMetric<tag_width> streak;

  emp::vector<double> values(num_pairs);
  std::generate(
    values.begin(),
    values.end(),
    [&streak, &random]() {
      // Generate pair of random tags.
      tag_t a(random, 0.5);
      tag_t b(random, 0.5);
      return streak(a, b);
    }
  );

  std::sort(
    values.begin(),
    values.end()
  );

  const size_t chop = (size_t)( (double)values.size() * ( (1 - interval) / 2.0) );
  const size_t bottom_i = chop;
  const size_t top_i = values.size() - chop;
  // std::cout << bottom_i << "," << top_i << std::endl;
  std::cout << values[bottom_i] << "," << values[top_i] << std::endl;
  return 0;
}