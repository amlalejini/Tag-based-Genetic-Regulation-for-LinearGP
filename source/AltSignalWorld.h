/***
 * Alternating Signal World
 *
 **/

#ifndef _ALT_SIGNAL_WORLD_H
#define _ALT_SIGNAL_WORLD_H

// Empirical
#include "tools/BitSet.h"
#include "tools/MatchBin.h"
#include "tools/matchbin_utils.h"
#include "control/Signal.h"
#include "Evolve/World.h"
// SignalGP includes
#include "hardware/SignalGP/impls/SignalGPLinearFunctionsProgram.h"
#include "hardware/SignalGP/utils/MemoryModel.h"
// Local includes
#include "AltSignalOrg.h"
#include "AltSignalConfig.h"


// TODO - use compile args!
namespace AltSignalWorldDefs {
  constexpr size_t TAG_LEN = 64;
  constexpr size_t INST_TAG_CNT = 1;
  constexpr size_t INST_ARG_CNT = 3;
  // matchbin <VALUE, METRIC, SELECTOR>
  using matchbin_val_t = size_t;                        // Module ID
  using matchbin_metric_t = emp::StreakMetric<TAG_LEN>; // How should we measure tag similarity?
  using matchbin_selector_t = emp::RankedSelector<>;    // 0% min threshold

  using org_t = AltSignalOrganism<emp::BitSet<TAG_LEN>,int>;
}

/// Custom hardware component for SignalGP.
struct CustomHardware {
  int response=-1;
};

class AltSignalWorld : public emp::World<AltSignalWorldDefs::org_t> {
public:
  using tag_t = emp::BitSet<AltSignalWorldDefs::TAG_LEN>;
  using inst_arg_t = int;
  using org_t = AltSignalOrganism<tag_t,inst_arg_t>;

  using matchbin_t = emp::MatchBin<AltSignalWorldDefs::matchbin_val_t,
                                   AltSignalWorldDefs::matchbin_metric_t,
                                   AltSignalWorldDefs::matchbin_selector_t>;
  using mem_model_t = emp::signalgp::SimpleMemoryModel;
  using hardware_t = emp::signalgp::LinearFunctionsProgramSignalGP<mem_model_t,
                                                                  tag_t,
                                                                  inst_arg_t,
                                                                  matchbin_t,
                                                                  CustomHardware>;
  using event_lib_t = typename hardware_t::event_lib_t;
  using inst_lib_t = typename hardware_t::inst_lib_t;

  /// State of the environment during an evaluation.
  struct Environment {
    size_t num_states=0;
    size_t cur_state=0;
    tag_t env_signal=tag_t();
  };

protected:

  size_t GENERATIONS;
  size_t POP_SIZE;

  Environment eval_environment;

  bool setup = false;
  emp::Ptr<inst_lib_t> inst_lib;
  emp::Ptr<event_lib_t> event_lib;

  emp::Signal<void(void)> end_setup_sig;

  void InitConfigs(const AltSignalConfig & config);

  void InitPop();
  void InitPop_Random();

  void DoEvaluation();
  void DoSelection();

public:
  AltSignalWorld() {}
  AltSignalWorld(emp::Random & r) : emp::World<org_t>(r) {}

  ~AltSignalWorld() {
    if (setup) {
      inst_lib.Delete();
      event_lib.Delete();
    }
  }
  void Reset();

  /// Setup world!
  void Setup(const AltSignalConfig & config);

  void RunStep();
  void Run();
};

// ---- PROTECTED IMPLEMENTATIONS ----

void AltSignalWorld::InitConfigs(const AltSignalConfig & config) {
  GENERATIONS = config.GENERATIONS();
  POP_SIZE = config.POP_SIZE();
}

void AltSignalWorld::InitPop() {
  InitPop_Random();
}

void AltSignalWorld::InitPop_Random() {
  for (size_t i = 0; i < POP_SIZE; ++i) {
    // TODO!
  }
}

// ---- PUBLIC IMPLEMENTATIONS ----

void AltSignalWorld::Setup(const AltSignalConfig & config) {
  // Localize configuration parameters.
  InitConfigs(config);

  // Create instruction/event libraries.
  inst_lib = emp::NewPtr<inst_lib_t>();
  event_lib = emp::NewPtr<event_lib_t>();

  // How should population be initialized?
  end_setup_sig.AddAction([this]() {
    std::cout << "Initializing population...";
    InitPop();
    std::cout << " Done" << std::endl;
  });

  end_setup_sig.Trigger();
  setup = true;
}

#endif