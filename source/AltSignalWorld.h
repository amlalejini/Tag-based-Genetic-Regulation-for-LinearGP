/***
 * Alternating Signal World
 *
 **/

#ifndef _ALT_SIGNAL_WORLD_H
#define _ALT_SIGNAL_WORLD_H

// emp tools
#include "tools/BitSet.h"
#include "tools/MatchBin.h"
#include "tools/matchbin_utils.h"

// emp Evolve includes
#include "Evolve/World.h"

// SignalGP includes
#include "hardware/SignalGP/impls/SignalGPLinearFunctionsProgram.h"
#include "hardware/SignalGP/utils/MemoryModel.h"

#include "AltSignalOrg.h"


// TODO - use compile args!
namespace AltSignalWorldDefs {
  constexpr size_t TAG_LEN = 64;
  // matchbin <VALUE, METRIC, SELECTOR>
  using matchbin_val_t = size_t;                        // Module ID
  using matchbin_metric_t = emp::StreakMetric<TAG_LEN>; // How should we measure tag similarity?
  using matchbin_selector_t = emp::RankedSelector<>;    // 0% min threshold
}

class AltSignalWorld : public emp::World<AltSignalOrganism> {
public:
  using tag_t = emp::BitSet<AltSignalWorldDefs::TAG_LEN>;
  using inst_arg_t = int;
  using org_t = AltSignalOrganism;

  using matchbin_t = emp::MatchBin<AltSignalWorldDefs::matchbin_val_t,
                                   AltSignalWorldDefs::matchbin_metric_t,
                                   AltSignalWorldDefs::matchbin_selector_t>;
  using mem_model_t = emp::signalgp::SimpleMemoryModel;
  using hardware_t = emp::signalgp::LinearFunctionsProgramSignalGP<mem_model_t,
                                                                  tag_t,
                                                                  inst_arg_t,
                                                                  matchbin_t>;

  struct Environment {
    size_t num_states=0;
    size_t cur_state=0;
    tag_t env_signal=tag_t();
  };

protected:

  Environment eval_environment;


public:
  AltSignalWorld() {}
  AltSignalWorld(emp::Random & r) : emp::World<AltSignalOrganism>(r) {}

  // ~AltSignalWorld() {}

};

#endif