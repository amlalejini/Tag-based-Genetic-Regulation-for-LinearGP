#ifndef TAG_LGP_MATCH_BIN_REGULATORS_H
#define TAG_LGP_MATCH_BIN_REGULATORS_H

#include <cmath>
#include <ratio>
#include "emp/matchbin/MatchBin.hpp"
#include "emp/matchbin/matchbin_regulators.hpp"
#include "emp/math/math.hpp"

/// Exponential regulator
/// This DOES NOT guarantee values between 0 and 1
/// Regulated Match = raw_match * (BASE^regulator_state)
template <typename Base=std::ratio<11,10>, size_t STATE_LIMIT=100>
struct ExponentialCountdownRegulator : emp::RegulatorBase<double, double, double> {

  static constexpr double max_state = STATE_LIMIT;
  static constexpr double min_state = -1 * STATE_LIMIT;
  static constexpr double base = (
    static_cast<double>(Base::num) / static_cast<double>(Base::den)
  );

  // positive = downregulated
  // negative = upregulated
  double state;

  // countdown timer to reseting state
  size_t timer;

  ExponentialCountdownRegulator() : state(0.0), timer(0) {}

  /// Apply regulation to a raw match score.
  /// Returns a value between 0 and 1.
  double operator()(const double raw_score) const override {
    const double res = raw_score * std::pow(base, state);
    return res;
  }

  /// A positive value downregulates the item,
  /// a value of zero is neutral,
  /// and a negative value upregulates the item.
  bool Set(const double & set) override {
    timer = 1;
    const double new_state = emp::Max( emp::Min(max_state, set), min_state );
    const bool change = new_state != state;
    state = new_state;
    // return whether regulator value changed
    // (i.e., we need to purge the cache)
    return change;
  }

  /// A negative value upregulates the item,
  /// a value of exactly zero is neutral
  /// and a postive value downregulates the item.
  bool Adj(const double & amt) override {
    timer = 1;

    const double new_state = emp::Max( emp::Min(max_state, state + amt), min_state );
    const bool change = new_state != state;
    state = new_state;

    // return whether regulator value changed
    // (i.e., we need to purge the cache)
    return change;
  }

  /// Timer decay.
  /// Return whether MatchBin should be updated
  bool Decay(const int steps) override {
    if (steps < 0) {
      // if reverse decay is requested
      timer += -steps;
    } else {
      // if forward decay is requested
      timer -= std::min(timer, static_cast<size_t>(steps));
    }

    return timer == 0 ? (
      std::exchange(state, 0.0) != 0.0
    ) : false;
  }

  /// Return a double representing the state of the regulator.
  const double & View() const override { return state; }

  std::string name() const override {
    return "Exponential Countdown Regulator";
  }

  bool operator!=(const ExponentialCountdownRegulator & other) const {
    return state != other.state || timer != other.timer;
  }

  #ifdef CEREAL_NVP
  template <class Archive>
  void serialize( Archive & ar )
  {
    ar(
      CEREAL_NVP(state),
      CEREAL_NVP(timer)
    );
  }
  #endif

};

#endif