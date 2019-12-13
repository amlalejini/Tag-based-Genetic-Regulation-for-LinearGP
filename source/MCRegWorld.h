/***
 * Multi-cell Regulation World
 *
 **/

#ifndef _MULTICELL_REG_WORLD_H
#define _MULTICELL_REG_WORLD_H

// macros courtesy of Matthew Andres Moreno
#ifndef SGP_REG_EXP_MACROS
#define STRINGVIEWIFY(s) std::string_view(IFY(s))
#define STRINGIFY(s) IFY(s)
#define IFY(s) #s
#define SGP_REG_EXP_MACROS
#endif

// C++ std
#include <functional>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <string_view>
#include <limits>
// Empirical
#include "tools/BitSet.h"
#include "tools/MatchBin.h"
#include "tools/matchbin_utils.h"
#include "tools/string_utils.h"
#include "control/Signal.h"
#include "Evolve/World.h"
#include "Evolve/World_select.h"
// SignalGP includes
#include "hardware/SignalGP/impls/SignalGPLinearFunctionsProgram.h"
#include "hardware/SignalGP/utils/LinearFunctionsProgram.h"
#include "hardware/SignalGP/utils/MemoryModel.h"
#include "hardware/SignalGP/utils/linear_program_instructions_impls.h"
#include "hardware/SignalGP/utils/linear_functions_program_instructions_impls.h"
// Local includes
#include "mutation_utils.h"
#include "MCRegConfig.h"
#include "MCRegOrg.h"
#include "MCRegDeme.h"
#include "Event.h"

namespace MCRegWorldDefs {
  constexpr size_t TAG_LEN = 64;      // How many bits per tag?
  constexpr size_t INST_TAG_CNT = 1;  // How many tags per instruction?
  constexpr size_t INST_ARG_CNT = 3;  // How many instruction arguments per instruction?
  constexpr size_t FUNC_NUM_TAGS = 1; // How many tags are associated with each function in a program?
  constexpr int INST_MIN_ARG_VAL = 0; // Minimum argument value?
  constexpr int INST_MAX_ARG_VAL = 7; // Maximum argument value?
  // matchbin <VALUE, METRIC, SELECTOR, REGULATOR>
  #ifndef MATCH_THRESH
  #define MATCH_THRESH 0
  #endif
  #ifndef MATCH_REG
  #define MATCH_REG add
  #endif
  #ifndef MATCH_METRIC
  #define MATCH_METRIC streak
  #endif
  using matchbin_val_t = size_t;                        // Module ID
  // What match threshold should we use?
  using matchbin_selector_t =
  #ifdef MATCH_THRESH
    std::conditional<STRINGVIEWIFY(MATCH_THRESH) == "0",
      emp::RankedSelector<>,
    std::conditional<STRINGVIEWIFY(MATCH_THRESH) == "25",
      emp::RankedSelector<std::ratio<TAG_LEN+(3*(TAG_LEN/4)), TAG_LEN>>,
    std::conditional<STRINGVIEWIFY(MATCH_THRESH) == "50",
      emp::RankedSelector<std::ratio<TAG_LEN+(TAG_LEN/2), TAG_LEN>>,
    std::conditional<STRINGVIEWIFY(MATCH_THRESH) == "75",
      emp::RankedSelector<std::ratio<TAG_LEN+(TAG_LEN/4), TAG_LEN>>,
      std::enable_if<false>
    >::type
    >::type
    >::type
    >::type;
  #endif

  // How should we measure tag similarity?
  using matchbin_metric_t =
  #ifdef MATCH_METRIC
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "integer",
      emp::SymmetricWrapMetric<TAG_LEN>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "hamming",
      emp::HammingMetric<TAG_LEN>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "hash",
      emp::HashMetric<TAG_LEN>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "streak",
      emp::StreakMetric<TAG_LEN>,
      std::enable_if<false>
    >::type
    >::type
    >::type
    >::type;
  #endif

  using matchbin_regulator_t =
  #ifdef MATCH_REG
    std::conditional<STRINGVIEWIFY(MATCH_REG) == "add",
      emp::AdditiveCountdownRegulator<>,
    std::conditional<STRINGVIEWIFY(MATCH_REG) == "mult",
      emp::MultiplicativeCountdownRegulator<>,
      std::enable_if<false>
    >::type
    >::type;
  #endif

  using org_t = MCRegOrganism<emp::BitSet<TAG_LEN>,int>;
}

class MCRegWorld : public emp::World<MCRegWorldDefs::org_t> {
public:
  using tag_t = emp::BitSet<MCRegWorldDefs::TAG_LEN>;
  using inst_arg_t = int;
  using org_t = MCRegOrganism<tag_t,inst_arg_t>;

  using matchbin_t = emp::MatchBin<MCRegWorldDefs::matchbin_val_t,
                                   MCRegWorldDefs::matchbin_metric_t,
                                   MCRegWorldDefs::matchbin_selector_t,
                                   MCRegWorldDefs::matchbin_regulator_t>;
  using mem_model_t = sgp::SimpleMemoryModel;
  using deme_t = MCRegDeme<mem_model_t, tag_t, inst_arg_t, matchbin_t>;
  using hardware_t = typename deme_t::hardware_t;
  using event_lib_t = typename hardware_t::event_lib_t;
  using base_event_t = typename hardware_t::event_t;
  using event_t = Event<MCRegWorldDefs::TAG_LEN>;
  using inst_lib_t = typename hardware_t::inst_lib_t;
  using inst_t = typename hardware_t::inst_t;
  using inst_prop_t = typename hardware_t::InstProperty;
  using program_t = typename hardware_t::program_t;
  using program_function_t = typename program_t::function_t;
  using mutator_t = MutatorLinearFunctionsProgram<hardware_t, tag_t, inst_arg_t>;
  using phenotype_t = typename org_t::MCRegPhenotype;

  using mut_landscape_t = emp::datastruct::mut_landscape_info<phenotype_t>;
  using systematics_t = emp::Systematics<org_t, typename org_t::genome_t, mut_landscape_t>;
  using taxon_t = typename systematics_t::taxon_t;

  /// State of the environment during an evaluation
  enum class ENV_STATE { DEVELOPMENT, RESPONSE };
  struct Environment {
    ENV_STATE cur_state=ENV_STATE::DEVELOPMENT;
    tag_t propagule_start_tag=tag_t();
    tag_t response_signal_tag=tag_t();

    void ResetEnv() {
      cur_state = ENV_STATE::DEVELOPMENT;
    }
    void SetPhase(ENV_STATE state) {
      cur_state = state;
    }
    ENV_STATE GetPhase() { return cur_state; }
  };

protected:
  // Localized config parameters:
  // Default group
  size_t GENERATIONS;
  size_t POP_SIZE;
  bool STOP_ON_SOLUTION;

  Environment eval_environment;

  bool setup = false;
  emp::Ptr<inst_lib_t> inst_lib;
  emp::Ptr<event_lib_t> event_lib;
  emp::Ptr<mutator_t> mutator;

  // TODO - Eval HW set
  emp::Ptr<deme_t> eval_deme;                ///< used to evaluate demes
  emp::Signal<void(size_t)> after_eval_sig;  ///< Triggered after organism (pop id given as argument) evaluation
  emp::Signal<void(void)> end_setup_sig;

  emp::Ptr<emp::DataFile> max_fit_file;
  emp::Ptr<systematics_t> systematics_ptr;  ///< Short cut to correctly-typed systematics manager. Base class will be responsible for memory management.

  bool found_solution = false;

  void InitConfigs(const MCRegConfig & config);
  void InitInstLib();
  void InitEventLib();
  void InitHardware();
  void InitEnvironment();
  void InitMutator();
  void InitDataCollection();

  void InitPop();
  void InitPop_Random();
  void InitPop_Hardcoded();

  void DoEvaluation();
  void DoSelection();
  void DoUpdate();

  void EvaluateOrg(org_t & org);

  // -- utilities --
  void DoPopulationSnapshot();
  // void DoWorldConfigSnapshot();

public:
  MCRegWorld(emp::Random & r) : emp::World<org_t>(r) {}

  ~MCRegWorld() {
    if (setup) {
      // TODO
    }
  }

  /// Setup world!
  void Setup(const MCRegConfig & config);

  /// Advance the world by a single time step (generation)
  void RunStep();

  /// Run world for configured number of generations
  void Run();
};

// ----- PUBLIC IMPLEMENTATIONS -----
void MCRegWorld::Setup(const MCRegConfig & config) {
  // todo
}

#endif