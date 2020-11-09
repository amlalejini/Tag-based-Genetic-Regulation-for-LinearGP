/***
 * ============================
 * === Changing Signal Task ===
 * ============================
 *
 * Implements the changing signal task experiment.
 * - Organisms must respond correctly to a sequence of K distinct environmental signals. Each environmental
 *   signal requires a distinct response from the organism.
 **/

#ifndef _CHG_ENV_WORLD_H
#define _CHG_ENV_WORLD_H

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
#include "emp/bits/BitSet.hpp"
#include "emp/matchbin/MatchBin.hpp"
#include "emp/matchbin/matchbin_utils.hpp"
#include "emp/tools/string_utils.hpp"
#include "emp/control/Signal.hpp"
#include "emp/Evolve/World.hpp"
#include "emp/Evolve/World_select.hpp"
// SignalGP includes
#include "hardware/SignalGP/impls/SignalGPLinearFunctionsProgram.h"
#include "hardware/SignalGP/utils/LinearFunctionsProgram.h"
#include "hardware/SignalGP/utils/MemoryModel.h"
#include "hardware/SignalGP/utils/linear_program_instructions_impls.h"
#include "hardware/SignalGP/utils/linear_functions_program_instructions_impls.h"
// Local includes
#include "ChgEnvOrg.h"
#include "ChgEnvConfig.h"
#include "mutation_utils.h"
#include "Event.h"

/// Globally-scoped, static variables.
namespace ChgEnvWorldDefs {
  #ifndef TAG_NUM_BITS
  #define TAG_NUM_BITS 64
  #endif
  constexpr size_t TAG_LEN = TAG_NUM_BITS;  ///< How many bits per tag?
  constexpr size_t INST_TAG_CNT = 1;        ///< How many tags per instruction?
  constexpr size_t INST_ARG_CNT = 3;        ///< How many instruction arguments per instruction?
  constexpr size_t FUNC_NUM_TAGS = 1;       ///< How many tags are associated with each function in a program?

  #ifndef MATCH_THRESH
  #define MATCH_THRESH 0
  #endif
  #ifndef MATCH_REG
  #define MATCH_REG add
  #endif
  #ifndef MATCH_METRIC
  #define MATCH_METRIC streak
  #endif
  using matchbin_val_t = size_t;  ///< SignalGP function ID type (how are functions identified in the matchbin?)

  // What match threshold should we use?
  // Remember, the ranked selector threshold is in terms of DISTANCE, not similarity. Thus, unintuitive template values.
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
      emp::AsymmetricWrapMetric<TAG_LEN>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "integer-symmetric",
      emp::SymmetricWrapMetric<TAG_LEN>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "hamming",
      emp::HammingMetric<TAG_LEN>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "hash",
      emp::CryptoHashMetric<TAG_LEN>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "streak",
      emp::StreakMetric<TAG_LEN>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "streak-exact",
      emp::ExactDualStreakMetric<TAG_LEN>,
    std::enable_if<false>
    >::type
    >::type
    >::type
    >::type
    >::type
    >::type;
  #endif

  // How should we regulate functions?
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

  using org_t = ChgEnvOrganism<emp::BitSet<TAG_LEN>,int>;
}

/// Custom hardware component for SignalGP.
struct ChgEnvCustomHardware {
  int response = -1;       ///< Organism-set response to environment signal.
  bool responded = false;  ///< Flag: did organism express a response yet?

  void Reset() {
    response = -1;
    responded = false;
  }

  void SetResponse(int i) {
    response = i;
    responded = true;
  }

  int GetResponse() const { return response; }
  bool HasResponse() const { return responded; }
};

/// Changing signal world class definition. Manages the changing signal task evolution experiment.
/// Derives from Empirical's world class.
class ChgEnvWorld : public emp::World<ChgEnvWorldDefs::org_t> {
public:
  using tag_t = emp::BitSet<ChgEnvWorldDefs::TAG_LEN>;
  using inst_arg_t = int;
  using config_t = ChgEnvConfig;
  using custom_comp_t = ChgEnvCustomHardware;

  using org_t = ChgEnvOrganism<tag_t,inst_arg_t>;
  using phenotype_t = typename org_t::ChgEnvPhenotype;

  using matchbin_t = emp::MatchBin<ChgEnvWorldDefs::matchbin_val_t,
                                   ChgEnvWorldDefs::matchbin_metric_t,
                                   ChgEnvWorldDefs::matchbin_selector_t,
                                   ChgEnvWorldDefs::matchbin_regulator_t>;
  using mem_model_t = sgp::SimpleMemoryModel;
  using hardware_t = sgp::LinearFunctionsProgramSignalGP<mem_model_t,
                                                         tag_t,
                                                         inst_arg_t,
                                                         matchbin_t,
                                                         custom_comp_t>;
  using event_lib_t = typename hardware_t::event_lib_t;
  using base_event_t = typename hardware_t::event_t;
  using event_t = Event<ChgEnvWorldDefs::TAG_LEN>;
  using inst_lib_t = typename hardware_t::inst_lib_t;
  using inst_t = typename hardware_t::inst_t;
  using inst_prop_t = typename hardware_t::InstProperty;
  using program_t = typename hardware_t::program_t;
  using program_function_t = typename program_t::function_t;
  using mutator_t = MutatorLinearFunctionsProgram<hardware_t, tag_t, inst_arg_t>;

  using mut_landscape_t = emp::datastruct::mut_landscape_info<phenotype_t>;
  using systematics_t = emp::Systematics<org_t, typename org_t::genome_t, mut_landscape_t>;
  using taxon_t = typename systematics_t::taxon_t;

  /// State of the environment during an evaluation.
  struct Environment {
    size_t num_states=0;    ///< How many total environment states (signals) are there?
    size_t cur_state=0;     ///< What is the current state of the environment?
    emp::vector<tag_t> env_state_tags;  ///< Tags associated with each environment signal.
    emp::vector<size_t> env_schedule;   ///< Current environment signal ordering.

    void ResetEnv() {
      cur_state = 0;
    }

    tag_t & GetCurEnvTag() {
      emp_assert(cur_state < env_state_tags.size());
      return env_state_tags[cur_state];
    }
  };

  /// Struct used as intermediary for printing/outputting SignalGP hardware state at a given time step.
  struct HardwareStatePrintInfo {
    std::string global_mem_str="";               ///< String representation of global memory.
    size_t num_modules=0;                        ///< Number of modules in the program loaded on hardware.
    emp::vector<double> module_regulator_states; ///< State of regulation for each module.
    emp::vector<size_t> module_regulator_timers; ///< Regulation timer for each module.
    size_t num_active_threads=0;                 ///< How many active threads are running?
    std::string thread_state_str="";             ///< String representation of state of all threads.
  };

protected:
  // Default group
  size_t GENERATIONS;
  size_t POP_SIZE;
  bool STOP_ON_SOLUTION;
  // Evaluation group
  size_t EVAL_TRIAL_CNT;
  // Environment group
  size_t NUM_ENV_STATES;
  size_t NUM_ENV_UPDATES;
  size_t CPU_CYCLES_PER_ENV_UPDATE;
  // Program group
  bool USE_FUNC_REGULATION;
  bool USE_GLOBAL_MEMORY;
  emp::Range<size_t> FUNC_CNT_RANGE;
  emp::Range<size_t> FUNC_LEN_RANGE;
  emp::Range<int> ARG_VAL_RANGE;
  // Hardware group
  size_t MAX_ACTIVE_THREAD_CNT;
  size_t MAX_THREAD_CAPACITY;
  // Selection group
  size_t TOURNAMENT_SIZE;
  // Mutation group
  double MUT_RATE__INST_ARG_SUB;
  double MUT_RATE__INST_TAG_BF;
  double MUT_RATE__INST_SUB;
  double MUT_RATE__INST_INS;
  double MUT_RATE__INST_DEL;
  double MUT_RATE__SEQ_SLIP;
  double MUT_RATE__FUNC_DUP;
  double MUT_RATE__FUNC_DEL;
  double MUT_RATE__FUNC_TAG_BF;
  // Data collection group
  std::string OUTPUT_DIR;
  size_t SUMMARY_RESOLUTION;
  size_t SNAPSHOT_RESOLUTION;
  size_t ANALYZE_ORG_EVAL_TRIALS;

  Environment eval_environment;  ///< Tracks the environment during evaluation.

  bool setup = false;               ///< Has this world been setup already?
  emp::Ptr<inst_lib_t> inst_lib;    ///< Manages SignalGP instruction set.
  emp::Ptr<event_lib_t> event_lib;  ///< Manages SignalGP events.
  emp::Ptr<mutator_t> mutator;      ///< Mutates SignalGP programs.

  size_t event_id__env_sig; ///< Event library ID for environment signals.

  emp::Ptr<hardware_t> eval_hardware;        ///< Used to evaluate programs.
  emp::vector<phenotype_t> trial_phenotypes; ///< Used to track phenotypes across organism evaluation trials.

  emp::Signal<void(size_t)> after_eval_sig; ///< Triggered after organism (ID given by size_t argument) evaluation
  emp::Signal<void(void)> end_setup_sig;    ///< Triggered at end of world setup.

  emp::Ptr<emp::DataFile> max_fit_file;    ///< Manages max fitness organism data tracking file (updated/output at SUMMARY_RESOLUTION)
  emp::Ptr<systematics_t> systematics_ptr; ///< Short cut to correctly-typed systematics manager. Base class will be responsible for memory management.

  size_t max_fit_org_id=0;
  double MAX_SCORE=0.0;       ///< Maximum possible score.
  bool found_solution=false;  ///< Found an organism that achieves a perfect score *DURING EVALUATION*

  bool KO_REGULATION=false;     ///< Is regulation knocked out right now?
  bool KO_GLOBAL_MEMORY=false;  ///< Is global memory access knocked out right now?

  /// Localize configuration parameters from input config object.
  void InitConfigs(const config_t & config);
  /// Initialize the instruction library.
  void InitInstLib();
  /// Initialize the event library.
  void InitEventLib();
  /// Initialize the SignalGP virtual hardware used to evaluate programs.
  void InitHardware();
  /// Initialize the environment (e.g., generate environment tag, etc).
  void InitEnvironment();
  /// Initialize and configure the mutator utility.
  void InitMutator();
  /// Initialize and configure data collection.
  void InitDataCollection();

  /// Initialize the population.
  void InitPop();
  /// Initialize the population with randomly generated programs.
  void InitPop_Random();
  /// Initialize the population with a hardcoded program. This is primarily used for debugging.
  void InitPop_Hardcoded();

  /// Evaluate the entire population.
  void DoEvaluation();
  /// Select parents for the next generation.
  void DoSelection();
  /// Move from one generation to the next.
  void DoUpdate();

  /// Evaluate org_t org on changing signal task.
  void EvaluateOrg(org_t & org, bool shuffle_env=true);

  /// Monster function that runs analyses on given organisms.
  /// - e.g., knockout experiments, traces, etc
  void AnalyzeOrg(const org_t & org, size_t org_id=0);
  /// Extract and output the execution trace of the given organism.
  void TraceOrganism(const org_t & org, size_t org_id=0);

  // -- Utilities --
  /// Output a snapshot of the current population.
  void DoPopulationSnapshot();
  /// Output a snapshot of the world's configuration.
  void DoWorldConfigSnapshot(const config_t & config);
  /// Output utility - stream a given program on a single line to ostream.
  void PrintProgramSingleLine(const program_t & prog, std::ostream & out);
  /// Output utility - stream a given function on a single line to ostream.
  void PrintProgramFunction(const program_function_t & func, std::ostream & out);
  /// Output utility - stream a given instruction on a single line to ostream.
  void PrintProgramInstruction(const inst_t & inst, std::ostream & out);
  /// Output utility - extract hardware state information from given SignalGP virtual hardware.
  HardwareStatePrintInfo GetHardwareStatePrintInfo(hardware_t & hw);
  /// Does the given phenotype qualify as a 'solution'?
  bool IsSolution(const phenotype_t & phen) const {
    return phen.GetScore() >= MAX_SCORE;
  }
public:
  ChgEnvWorld() {}
  ChgEnvWorld(emp::Random & r) : emp::World<org_t>(r) {}

  ~ChgEnvWorld() {
    if (setup) {
      inst_lib.Delete();
      event_lib.Delete();
      eval_hardware.Delete();
      mutator.Delete();
      max_fit_file.Delete();
    }
  }

  /// Setup world!
  void Setup(const config_t & config);

  /// Advance world by a single time step (generation).
  void RunStep();

  /// Run world for configured number of generations.
  void Run();
};

// ----- Protected implementation ------
void ChgEnvWorld::InitConfigs(const config_t & config) {
  // Default group
  GENERATIONS = config.GENERATIONS();
  POP_SIZE = config.POP_SIZE();
  STOP_ON_SOLUTION = config.STOP_ON_SOLUTION();
  // Evaluation group
  EVAL_TRIAL_CNT = config.EVAL_TRIAL_CNT();
  // Environment group
  NUM_ENV_STATES = config.NUM_ENV_STATES();
  NUM_ENV_UPDATES = config.NUM_ENV_UPDATES();
  CPU_CYCLES_PER_ENV_UPDATE = config.CPU_CYCLES_PER_ENV_UPDATE();
  // Program group
  USE_FUNC_REGULATION = config.USE_FUNC_REGULATION();
  USE_GLOBAL_MEMORY = config.USE_GLOBAL_MEMORY();
  FUNC_CNT_RANGE = {config.MIN_FUNC_CNT(), config.MAX_FUNC_CNT()};
  FUNC_LEN_RANGE = {config.MIN_FUNC_INST_CNT(), config.MAX_FUNC_INST_CNT()};
  ARG_VAL_RANGE = {config.INST_MIN_ARG_VAL(), config.INST_MAX_ARG_VAL()};
  // Hardware group
  MAX_ACTIVE_THREAD_CNT = config.MAX_ACTIVE_THREAD_CNT();
  MAX_THREAD_CAPACITY = config.MAX_THREAD_CAPACITY();
  // Selection group
  TOURNAMENT_SIZE = config.TOURNAMENT_SIZE();
  // Mutation group
  MUT_RATE__INST_ARG_SUB = config.MUT_RATE__INST_ARG_SUB();
  MUT_RATE__INST_TAG_BF = config.MUT_RATE__INST_TAG_BF();
  MUT_RATE__INST_SUB = config.MUT_RATE__INST_SUB();
  MUT_RATE__INST_INS = config.MUT_RATE__INST_INS();
  MUT_RATE__INST_DEL = config.MUT_RATE__INST_DEL();
  MUT_RATE__SEQ_SLIP = config.MUT_RATE__SEQ_SLIP();
  MUT_RATE__FUNC_DUP = config.MUT_RATE__FUNC_DUP();
  MUT_RATE__FUNC_DEL = config.MUT_RATE__FUNC_DEL();
  MUT_RATE__FUNC_TAG_BF = config.MUT_RATE__FUNC_TAG_BF();
  // Data collection group
  OUTPUT_DIR = config.OUTPUT_DIR();
  SUMMARY_RESOLUTION = config.SUMMARY_RESOLUTION();
  SNAPSHOT_RESOLUTION = config.SNAPSHOT_RESOLUTION();
  ANALYZE_ORG_EVAL_TRIALS = config.ANALYZE_ORG_EVAL_TRIALS();
}

void ChgEnvWorld::InitInstLib() {
  if (!setup) inst_lib = emp::NewPtr<inst_lib_t>();
  inst_lib->Clear(); // Reset the instruction library
  /// Add default instructions.
  inst_lib->AddInst("Nop", [](hardware_t & hw, const inst_t & inst) { ; }, "No operation!");
  inst_lib->AddInst("Inc", sgp::inst_impl::Inst_Inc<hardware_t, inst_t>, "Increment!");
  inst_lib->AddInst("Dec", sgp::inst_impl::Inst_Dec<hardware_t, inst_t>, "Decrement!");
  inst_lib->AddInst("Not", sgp::inst_impl::Inst_Not<hardware_t, inst_t>, "Logical not of ARG[0]");
  inst_lib->AddInst("Add", sgp::inst_impl::Inst_Add<hardware_t, inst_t>, "");
  inst_lib->AddInst("Sub", sgp::inst_impl::Inst_Sub<hardware_t, inst_t>, "");
  inst_lib->AddInst("Mult", sgp::inst_impl::Inst_Mult<hardware_t, inst_t>, "");
  inst_lib->AddInst("Div", sgp::inst_impl::Inst_Div<hardware_t, inst_t>, "");
  inst_lib->AddInst("Mod", sgp::inst_impl::Inst_Mod<hardware_t, inst_t>, "");
  inst_lib->AddInst("TestEqu", sgp::inst_impl::Inst_TestEqu<hardware_t, inst_t>, "");
  inst_lib->AddInst("TestNEqu", sgp::inst_impl::Inst_TestNEqu<hardware_t, inst_t>, "");
  inst_lib->AddInst("TestLess", sgp::inst_impl::Inst_TestLess<hardware_t, inst_t>, "");
  inst_lib->AddInst("TestLessEqu", sgp::inst_impl::Inst_TestLessEqu<hardware_t, inst_t>, "");
  inst_lib->AddInst("TestGreater", sgp::inst_impl::Inst_TestGreater<hardware_t, inst_t>, "");
  inst_lib->AddInst("TestGreaterEqu", sgp::inst_impl::Inst_TestGreaterEqu<hardware_t, inst_t>, "");
  inst_lib->AddInst("SetMem", sgp::inst_impl::Inst_SetMem<hardware_t, inst_t>, "");
  inst_lib->AddInst("Close", sgp::inst_impl::Inst_Close<hardware_t, inst_t>, "", {inst_prop_t::BLOCK_CLOSE});
  inst_lib->AddInst("Break", sgp::inst_impl::Inst_Break<hardware_t, inst_t>, "");
  inst_lib->AddInst("Call", sgp::inst_impl::Inst_Call<hardware_t, inst_t>, "");
  inst_lib->AddInst("Return", sgp::inst_impl::Inst_Return<hardware_t, inst_t>, "");
  inst_lib->AddInst("CopyMem", sgp::inst_impl::Inst_CopyMem<hardware_t, inst_t>, "");
  inst_lib->AddInst("SwapMem", sgp::inst_impl::Inst_SwapMem<hardware_t, inst_t>, "");
  inst_lib->AddInst("InputToWorking", sgp::inst_impl::Inst_InputToWorking<hardware_t, inst_t>, "");
  inst_lib->AddInst("WorkingToOutput", sgp::inst_impl::Inst_WorkingToOutput<hardware_t, inst_t>, "");
  inst_lib->AddInst("Fork", sgp::inst_impl::Inst_Fork<hardware_t, inst_t>, "");
  inst_lib->AddInst("Terminate", sgp::inst_impl::Inst_Terminate<hardware_t, inst_t>, "");
  inst_lib->AddInst("If", sgp::lfp_inst_impl::Inst_If<hardware_t, inst_t>, "", {inst_prop_t::BLOCK_DEF});
  inst_lib->AddInst("While", sgp::lfp_inst_impl::Inst_While<hardware_t, inst_t>, "", {inst_prop_t::BLOCK_DEF});
  inst_lib->AddInst("Routine", sgp::lfp_inst_impl::Inst_Routine<hardware_t, inst_t>, "");
  inst_lib->AddInst("Terminal", sgp::inst_impl::Inst_Terminal<hardware_t, inst_t,
                                                            std::ratio<1>, std::ratio<-1>>, "");

  // If global memory access is allowed, add access instructions; otherwise, add an equivalent number
  // of no-operation instructions.
  if (USE_GLOBAL_MEMORY) {
    inst_lib->AddInst("WorkingToGlobal", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_GLOBAL_MEMORY) sgp::inst_impl::Inst_WorkingToGlobal<hardware_t, inst_t>(hw, inst);
    }, "");
    inst_lib->AddInst("GlobalToWorking", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_GLOBAL_MEMORY) sgp::inst_impl::Inst_GlobalToWorking<hardware_t, inst_t>(hw, inst);
    }, "");
  } else {
    inst_lib->AddInst("Nop-WorkingToGlobal", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-GlobalToWorking", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
  }

  // If we can use regulation, add regulation instructions; otherwise, add an equivalent number of
  // no-operation instructions.
  if (USE_FUNC_REGULATION) {
    inst_lib->AddInst("SetRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_SetRegulator<hardware_t, inst_t>(hw, inst);
    }, "");
    inst_lib->AddInst("SetOwnRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_SetOwnRegulator<hardware_t, inst_t>(hw, inst);
    }, "");
    inst_lib->AddInst("AdjRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_AdjRegulator<hardware_t, inst_t>(hw, inst);
    }, "");
    inst_lib->AddInst("AdjOwnRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_AdjOwnRegulator<hardware_t, inst_t>(hw, inst);
    }, "");

    inst_lib->AddInst("SetRegulator-", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_SetRegulator<hardware_t, inst_t, -1>(hw, inst);
    }, "");
    inst_lib->AddInst("SetOwnRegulator-", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_SetOwnRegulator<hardware_t, inst_t, -1>(hw, inst);
    }, "");
    inst_lib->AddInst("AdjRegulator-", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_AdjRegulator<hardware_t, inst_t, -1>(hw, inst);
    }, "");
    inst_lib->AddInst("AdjOwnRegulator-", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_AdjOwnRegulator<hardware_t, inst_t, -1>(hw, inst);
    }, "");

    inst_lib->AddInst("SenseRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_SenseRegulator<hardware_t, inst_t>(hw, inst);
    }, "");
    inst_lib->AddInst("SenseOwnRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_SenseOwnRegulator<hardware_t, inst_t>(hw, inst);
    }, "");
    inst_lib->AddInst("IncRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_IncRegulator<hardware_t, inst_t>(hw, inst);
    }, "");
    inst_lib->AddInst("IncOwnRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_IncOwnRegulator<hardware_t, inst_t>(hw, inst);
     }, "");
    inst_lib->AddInst("DecRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_DecRegulator<hardware_t, inst_t>(hw, inst);
    }, "");
    inst_lib->AddInst("DecOwnRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_DecOwnRegulator<hardware_t, inst_t>(hw, inst);
    }, "");
    inst_lib->AddInst("ClearRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_ClearRegulator<hardware_t, inst_t>(hw, inst);
    }, "");
    inst_lib->AddInst("ClearOwnRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_ClearOwnRegulator<hardware_t, inst_t>(hw, inst);
    }, "");
  } else {
    inst_lib->AddInst("Nop-SetRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-SetOwnRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-AdjRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-AdjOwnRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-SetRegulator-", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-SetOwnRegulator-", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-AdjRegulator-", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-AdjOwnRegulator-", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-ClearRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-ClearOwnRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-SenseRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-SenseOwnRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-IncRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-IncOwnRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-DecRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-DecOwnRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
  }

  // Add response instructions
  for (size_t i = 0; i < NUM_ENV_STATES; ++i) {
    inst_lib->AddInst("Response-" + emp::to_string(i), [this, i](hardware_t & hw, const inst_t & inst) {
      // Mark response in hardware.
      hw.GetCustomComponent().SetResponse(i);
      // Remove all pending threads.
      hw.RemoveAllPendingThreads();
      // Mark all active threads as dead.
      for (size_t thread_id : hw.GetActiveThreadIDs()) {
        hw.GetThread(thread_id).SetDead();
      }
    }, "Set organism response to the environment");
  }
}

void ChgEnvWorld::InitEventLib() {
  if (!setup) event_lib = emp::NewPtr<event_lib_t>();
  event_lib->Clear();
  // Setup event: EnvSignal
  // Args: name, handler_fun, dispatchers, desc
  event_id__env_sig = event_lib->AddEvent("EnvironmentSignal",
                                          [this](hardware_t & hw, const base_event_t & e) {
                                            const event_t & event = static_cast<const event_t&>(e);
                                            emp_assert(eval_environment.GetCurEnvTag() == event.GetTag());
                                            hw.SpawnThreadWithTag(event.GetTag());
                                          });
}

void ChgEnvWorld::InitHardware() {
  // If being configured for the first time, create a new hardware object.
  if (!setup) {
    eval_hardware = emp::NewPtr<hardware_t>(*random_ptr, *inst_lib, *event_lib);
  }
  // Configure SignalGP CPU
  eval_hardware->Reset();
  eval_hardware->SetActiveThreadLimit(MAX_ACTIVE_THREAD_CNT);
  eval_hardware->SetThreadCapacity(MAX_THREAD_CAPACITY);
  emp_assert(eval_hardware->ValidateThreadState());
}

void ChgEnvWorld::InitEnvironment() {
  eval_environment.num_states = NUM_ENV_STATES;
  eval_environment.env_state_tags = emp::RandomBitSets<ChgEnvWorldDefs::TAG_LEN>(*random_ptr, NUM_ENV_STATES, true);
  emp_assert(eval_environment.env_state_tags.size() == NUM_ENV_STATES);
  std::cout << "--- ENVIRONMENT SETUP ---" << std::endl;
  for (size_t i = 0; i < NUM_ENV_STATES; ++i) {
    eval_environment.env_schedule.emplace_back(i);
    std::cout << "  Environment tag " << i << ": ";
    eval_environment.env_state_tags[i].Print();
    std::cout << std::endl;
  }
  eval_environment.ResetEnv();
  trial_phenotypes.resize(EVAL_TRIAL_CNT);
  for (phenotype_t & phen : trial_phenotypes) {
    phen.Reset();
  }
}

void ChgEnvWorld::InitMutator() {
  if (!setup) { mutator = emp::NewPtr<mutator_t>(*inst_lib); }
  mutator->ResetLastMutationTracker();
  // Set program constraints
  mutator->SetProgFunctionCntRange(FUNC_CNT_RANGE);
  mutator->SetProgFunctionInstCntRange(FUNC_LEN_RANGE);
  mutator->SetProgInstArgValueRange(ARG_VAL_RANGE);
  mutator->SetTotalInstLimit(2*FUNC_LEN_RANGE.GetUpper()*FUNC_CNT_RANGE.GetUpper());
  mutator->SetFuncNumTags(ChgEnvWorldDefs::FUNC_NUM_TAGS);
  mutator->SetInstNumTags(ChgEnvWorldDefs::INST_TAG_CNT);
  mutator->SetInstNumArgs(ChgEnvWorldDefs::INST_ARG_CNT);
  // Set mutation rates
  mutator->SetRateInstArgSub(MUT_RATE__INST_ARG_SUB);
  mutator->SetRateInstTagBF(MUT_RATE__INST_TAG_BF);
  mutator->SetRateInstSub(MUT_RATE__INST_SUB);
  mutator->SetRateInstIns(MUT_RATE__INST_INS);
  mutator->SetRateInstDel(MUT_RATE__INST_DEL);
  mutator->SetRateSeqSlip(MUT_RATE__SEQ_SLIP);
  mutator->SetRateFuncDup(MUT_RATE__FUNC_DUP);
  mutator->SetRateFuncDel(MUT_RATE__FUNC_DEL);
  mutator->SetRateFuncTagBF(MUT_RATE__FUNC_TAG_BF);
  // Set world mutation function.
  this->SetMutFun([this](org_t & org, emp::Random & rnd) {
    org.ResetMutations();                // Reset organism's recorded mutations.
    mutator->ResetLastMutationTracker(); // Reset mutator mutation tracking.
    const size_t mut_cnt = mutator->ApplyAll(rnd, org.GetGenome().program);
    // Record mutations in organism.
    auto & mut_dist = mutator->GetLastMutations();
    auto & org_mut_tracker = org.GetMutations();
    org_mut_tracker["inst_arg_sub"] = mut_dist[mutator_t::MUTATION_TYPES::INST_ARG_SUB];
    org_mut_tracker["inst_tag_bit_flip"] = mut_dist[mutator_t::MUTATION_TYPES::INST_TAG_BIT_FLIP];
    org_mut_tracker["inst_sub"] = mut_dist[mutator_t::MUTATION_TYPES::INST_SUB];
    org_mut_tracker["inst_ins"] = mut_dist[mutator_t::MUTATION_TYPES::INST_INS];
    org_mut_tracker["inst_del"] = mut_dist[mutator_t::MUTATION_TYPES::INST_DEL];
    org_mut_tracker["seq_slip_dup"] = mut_dist[mutator_t::MUTATION_TYPES::SEQ_SLIP_DUP];
    org_mut_tracker["seq_slip_del"] = mut_dist[mutator_t::MUTATION_TYPES::SEQ_SLIP_DEL];
    org_mut_tracker["func_dup"] = mut_dist[mutator_t::MUTATION_TYPES::FUNC_DUP];
    org_mut_tracker["func_del"] = mut_dist[mutator_t::MUTATION_TYPES::FUNC_DEL];
    org_mut_tracker["func_tag_bit_flip"] = mut_dist[mutator_t::MUTATION_TYPES::FUNC_TAG_BIT_FLIP];
    return mut_cnt;
  });
}

void ChgEnvWorld::InitPop() {
  this->Clear();
  InitPop_Random();
}

void ChgEnvWorld::InitPop_Random() {
  for (size_t i = 0; i < POP_SIZE; ++i) {
    this->Inject({sgp::GenRandLinearFunctionsProgram<hardware_t, ChgEnvWorldDefs::TAG_LEN>
                                  (*random_ptr, *inst_lib,
                                   FUNC_CNT_RANGE,
                                   ChgEnvWorldDefs::FUNC_NUM_TAGS,
                                   FUNC_LEN_RANGE,
                                   ChgEnvWorldDefs::INST_TAG_CNT,
                                   ChgEnvWorldDefs::INST_ARG_CNT,
                                   ARG_VAL_RANGE)
                  }, 1);
  }
}

void ChgEnvWorld::InitDataCollection() {
  // todo - make okay to call setup twice!
  emp_assert(!setup);
  if (setup) {
    max_fit_file.Delete();
  } else {
    mkdir(OUTPUT_DIR.c_str(), ACCESSPERMS);
    if(OUTPUT_DIR.back() != '/')
        OUTPUT_DIR += '/';
  }
  // Some generally useful functions
  std::function<size_t(void)> get_update = [this]() { return this->GetUpdate(); };
  // Data files:
  // - Fitness file
  // - Max fitness file (solution should be last line(?))
  // - Phylogeny file
  //   - mutation distribution from parent
  // - Population snapshot
  // SETUP:
  // --- Fitness File ---
  SetupFitnessFile(OUTPUT_DIR + "/fitness.csv").SetTimingRepeat(SUMMARY_RESOLUTION);
  // --- Systematics tracking ---
  systematics_ptr = emp::NewPtr<systematics_t>([](const org_t & o) { return o.GetGenome(); });
  // We want to record phenotype information AFTER organism is evaluated.
  // - for this, we need to find the appropriate taxon post-evaluation
  after_eval_sig.AddAction([this](size_t pop_id) {
    emp::Ptr<taxon_t> taxon = systematics_ptr->GetTaxonAt(pop_id);
    taxon->GetData().RecordFitness(this->CalcFitnessID(pop_id));
    taxon->GetData().RecordPhenotype(this->GetOrg(pop_id).GetPhenotype());
  });
  // We want to record mutations when organism is added to the population
  // - because mutations are applied automatically by this->DoBirth => this->AddOrgAt => sys->OnNew
  std::function<void(emp::Ptr<taxon_t>, org_t&)> record_taxon_mut_data =
    [this](emp::Ptr<taxon_t> taxon, org_t & org) {
      taxon->GetData().RecordMutation(org.GetMutations());
    };
  systematics_ptr->OnNew(record_taxon_mut_data);
  // Add snapshot functions
  // - fitness information (taxon->GetFitness)
  // - phenotype information
  //   - res collected, correct resp, no resp
  // - mutations (counts by type)
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) {
    return emp::to_string(taxon.GetData().GetFitness());
  }, "fitness", "Taxon fitness");
  systematics_ptr->AddSnapshotFun([this](const taxon_t & taxon) -> std::string {
    const bool is_sol = this->IsSolution(taxon.GetData().GetPhenotype());
    return (is_sol) ? "1" : "0";
  }, "is_solution", "Is this a solution?");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) {
    return emp::to_string(taxon.GetData().GetPhenotype().GetScore());
  }, "org_score", "");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) {
    return emp::to_string(taxon.GetData().GetPhenotype().GetEnvMatches());
  }, "org_matches", "");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) {
    return emp::to_string(taxon.GetData().GetPhenotype().GetEnvMisses());
  }, "org_misses", "");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) {
    return emp::to_string(taxon.GetData().GetPhenotype().GetNoResponses());
  }, "org_no_responses", "");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
    // if (taxon.GetData().HasMutationType("inst_arg_sub")) {
    if (emp::Has(taxon.GetData().mut_counts, "inst_arg_sub")) {
      return emp::to_string(taxon.GetData().mut_counts.at("inst_arg_sub"));
    } else {
      return "0";
    }
  }, "inst_arg_sub_mut_cnt", "How many mutations from parent taxon to this taxon?");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (emp::Has(taxon.GetData().mut_counts, "inst_tag_bit_flip")) {
        return emp::to_string(taxon.GetData().mut_counts.at("inst_tag_bit_flip"));
      } else {
        return "0";
      }
    }, "inst_tag_bit_flip_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (emp::Has(taxon.GetData().mut_counts, "inst_sub")) {
        return emp::to_string(taxon.GetData().mut_counts.at("inst_sub"));
      } else {
        return "0";
      }
    }, "inst_sub_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (emp::Has(taxon.GetData().mut_counts, "inst_ins")) {
        return emp::to_string(taxon.GetData().mut_counts.at("inst_ins"));
      } else {
        return "0";
      }
    }, "inst_ins_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (emp::Has(taxon.GetData().mut_counts, "inst_del")) {
        return emp::to_string(taxon.GetData().mut_counts.at("inst_del"));
      } else {
        return "0";
      }
    }, "inst_del_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (emp::Has(taxon.GetData().mut_counts, "seq_slip_dup")) {
        return emp::to_string(taxon.GetData().mut_counts.at("seq_slip_dup"));
      } else {
        return "0";
      }
    }, "seq_slip_dup_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (emp::Has(taxon.GetData().mut_counts, "seq_slip_del")) {
        return emp::to_string(taxon.GetData().mut_counts.at("seq_slip_del"));
      } else {
        return "0";
      }
    }, "seq_slip_del_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (emp::Has(taxon.GetData().mut_counts, "func_dup")) {
        return emp::to_string(taxon.GetData().mut_counts.at("func_dup"));
      } else {
        return "0";
      }
    }, "func_dup_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (emp::Has(taxon.GetData().mut_counts, "func_del")) {
        return emp::to_string(taxon.GetData().mut_counts.at("func_del"));
      } else {
        return "0";
      }
    }, "func_del_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      // if (taxon.GetData().HasMutationType("func_tag_bit_flip")) {
      if (emp::Has(taxon.GetData().mut_counts, "func_tag_bit_flip")) {
        return emp::to_string(taxon.GetData().mut_counts.at("func_tag_bit_flip"));
      } else {
        return "0";
      }
    }, "func_tag_bit_flip_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([this](const taxon_t & taxon) {
    std::ostringstream stream;
    stream << "\"";
    this->PrintProgramSingleLine(taxon.GetInfo().GetProgram(), stream);
    stream << "\"";
    return stream.str();
  }, "program", "Program representing this taxon");
  AddSystematics(systematics_ptr);
  SetupSystematicsFile(0, OUTPUT_DIR + "/systematics.csv").SetTimingRepeat(SUMMARY_RESOLUTION);
  // --- Dominant File ---
  max_fit_file = emp::NewPtr<emp::DataFile>(OUTPUT_DIR + "/max_fit_org.csv");
  max_fit_file->AddFun(get_update, "update");
  max_fit_file->template AddFun<size_t>([this]() { return max_fit_org_id; }, "pop_id");
  max_fit_file->template AddFun<bool>([this]() {
    org_t & org = this->GetOrg(max_fit_org_id);
    return this->IsSolution(org.GetPhenotype());
  }, "solution");
  max_fit_file->template AddFun<double>([this]() {
    return this->GetOrg(max_fit_org_id).GetPhenotype().GetScore();
  }, "score");
  max_fit_file->template AddFun<size_t>([this]() {
    return this->GetOrg(max_fit_org_id).GetPhenotype().GetEnvMatches();
  }, "num_matches");
  max_fit_file->template AddFun<size_t>([this]() {
    return this->GetOrg(max_fit_org_id).GetPhenotype().GetEnvMisses();
  }, "num_misses");
  max_fit_file->template AddFun<size_t>([this]() {
    return this->GetOrg(max_fit_org_id).GetPhenotype().GetNoResponses();
  }, "num_no_responses");
  max_fit_file->template AddFun<size_t>([this]() {
    return this->GetOrg(max_fit_org_id).GetGenome().GetProgram().GetSize();
  }, "num_modules");
  max_fit_file->template AddFun<size_t>([this]() {
    return this->GetOrg(max_fit_org_id).GetGenome().GetProgram().GetInstCount();
  }, "num_instructions");
  max_fit_file->template AddFun<std::string>([this]() {
    std::ostringstream stream;
    stream << "\"";
    this->PrintProgramSingleLine(this->GetOrg(max_fit_org_id).GetGenome().GetProgram(), stream);
    stream << "\"";
    return stream.str();
  }, "program");
  max_fit_file->PrintHeaderKeys();
}

void ChgEnvWorld::DoWorldConfigSnapshot(const config_t & config) {
  // Print matchbin metric
  std::cout << "Requested MatchBin Metric: " << STRINGVIEWIFY(MATCH_METRIC) << std::endl;
  std::cout << "Requested MatchBin Match Thresh: " << STRINGVIEWIFY(MATCH_THRESH) << std::endl;
  std::cout << "Requested MatchBin Regulator: " << STRINGVIEWIFY(MATCH_REG) << std::endl;
  // Make a new data file for snapshot.
  emp::DataFile snapshot_file(OUTPUT_DIR + "/run_config.csv");
  std::function<std::string()> get_cur_param;
  std::function<std::string()> get_cur_value;
  snapshot_file.template AddFun<std::string>([&get_cur_param]() -> std::string { return get_cur_param(); }, "parameter");
  snapshot_file.template AddFun<std::string>([&get_cur_value]() -> std::string { return get_cur_value(); }, "value");
  snapshot_file.PrintHeaderKeys();  // param, value
  // matchbin metric
  get_cur_param = []() { return "matchbin_metric"; };
  get_cur_value = []() { return emp::to_string(STRINGVIEWIFY(MATCH_METRIC)); };
  snapshot_file.Update();
  // matchbin threshold
  get_cur_param = []() { return "matchbin_thresh"; };
  get_cur_value = []() { return emp::to_string(STRINGVIEWIFY(MATCH_THRESH)); };
  snapshot_file.Update();
  // matchbin regulator
  get_cur_param = []() { return "matchbin_regulator"; };
  get_cur_value = []() { return emp::to_string(STRINGVIEWIFY(MATCH_REG)); };
  snapshot_file.Update();
  // TAG_LEN
  get_cur_param = []() { return "TAG_LEN"; };
  get_cur_value = []() { return emp::to_string(ChgEnvWorldDefs::TAG_LEN); };
  snapshot_file.Update();
  // INST_TAG_CNT
  get_cur_param = []() { return "INST_TAG_CNT"; };
  get_cur_value = []() { return emp::to_string(ChgEnvWorldDefs::INST_TAG_CNT); };
  snapshot_file.Update();
  // INST_ARG_CNT
  get_cur_param = []() { return "INST_ARG_CNT"; };
  get_cur_value = []() { return emp::to_string(ChgEnvWorldDefs::INST_ARG_CNT); };
  snapshot_file.Update();
  // FUNC_NUM_TAGS
  get_cur_param = []() { return "FUNC_NUM_TAGS"; };
  get_cur_value = []() { return emp::to_string(ChgEnvWorldDefs::FUNC_NUM_TAGS); };
  snapshot_file.Update();
  // environment signal
  get_cur_param = []() { return "env_state_tags"; };
  get_cur_value = [this]() {
    std::ostringstream stream;
    stream << "\"[";
    for (size_t i = 0; i < NUM_ENV_STATES; ++i) {
      if (i) stream << ",";
      eval_environment.env_state_tags[i].Print(stream);
    }
    stream << "]\"";
    return stream.str();
  };
  snapshot_file.Update();
  for (const auto & entry : config) {
    get_cur_param = [&entry]() { return entry.first; };
    get_cur_value = [&entry]() { return emp::to_string(entry.second->GetValue()); };
    snapshot_file.Update();
  }
}

void ChgEnvWorld::DoPopulationSnapshot() {
  // Make a new data file for snapshot.
  emp::DataFile snapshot_file(OUTPUT_DIR + "/pop_" + emp::to_string((int)GetUpdate()) + ".csv");
  size_t cur_org_id = 0;
  // Add functions.
  snapshot_file.AddFun<size_t>([this]() { return this->GetUpdate(); }, "update");
  snapshot_file.AddFun<size_t>([this, &cur_org_id]() { return cur_org_id; }, "pop_id");
  // max_fit_file->AddFun(, "genotype_id");
  snapshot_file.AddFun<bool>([this, &cur_org_id]() {
    org_t & org = this->GetOrg(cur_org_id);
    return this->IsSolution(org.GetPhenotype());
  }, "solution");
  snapshot_file.template AddFun<double>([this, &cur_org_id]() {
    return this->GetOrg(cur_org_id).GetPhenotype().GetScore();
  }, "score");
  snapshot_file.template AddFun<size_t>([this, &cur_org_id]() {
    return this->GetOrg(cur_org_id).GetPhenotype().GetEnvMatches();
  }, "env_matches");
  snapshot_file.template AddFun<size_t>([this, &cur_org_id]() {
    return this->GetOrg(cur_org_id).GetPhenotype().GetEnvMisses();
  }, "env_misses");
  snapshot_file.template AddFun<size_t>([this, &cur_org_id]() {
    return this->GetOrg(cur_org_id).GetPhenotype().GetNoResponses();
  }, "no_responses");
  snapshot_file.template AddFun<size_t>([this, &cur_org_id]() {
    return this->GetOrg(cur_org_id).GetGenome().GetProgram().GetSize();
  }, "num_modules");
  snapshot_file.template AddFun<size_t>([this, &cur_org_id]() {
    return this->GetOrg(cur_org_id).GetGenome().GetProgram().GetInstCount();
  }, "num_instructions");
  snapshot_file.template AddFun<std::string>([this, &cur_org_id]() {
    std::ostringstream stream;
    stream << "\"";
    this->PrintProgramSingleLine(this->GetOrg(cur_org_id).GetGenome().GetProgram(), stream);
    stream << "\"";
    return stream.str();
  }, "program");
  snapshot_file.PrintHeaderKeys();
  for (cur_org_id = 0; cur_org_id < GetSize(); ++cur_org_id) {
    emp_assert(IsOccupied(cur_org_id));
    snapshot_file.Update();
  }
}

void ChgEnvWorld::EvaluateOrg(org_t & org, bool shuffle_env/*=true*/) {
  // Evaluate org NUM_TRIALS times, keep worst phenotype.
  // Reset organism phenotype.
  org.GetPhenotype().Reset();
  // Ready the hardware!
  eval_hardware->SetProgram(org.GetGenome().program);
  size_t min_trial_id = 0;
  for (size_t trial_id = 0; trial_id < EVAL_TRIAL_CNT; ++trial_id) {
    emp_assert(trial_id < trial_phenotypes.size());
    // Reset trial phenotype
    phenotype_t & trial_phen = trial_phenotypes[trial_id];
    trial_phen.Reset();
    // reset the environment
    eval_environment.ResetEnv();
    // shuffle environment schedule for this trial
    if (shuffle_env) { emp::Shuffle(*random_ptr, eval_environment.env_schedule); }
    // reset hardware matchbin between trials
    eval_hardware->ResetMatchBin();
    eval_hardware->ResetHardwareState();
    // Evaluate the organism in the environment.
    for (size_t env_update = 0; env_update < NUM_ENV_UPDATES; ++env_update) {
      // Reset the hardware!
      eval_hardware->ResetBaseHardwareState();
      eval_hardware->GetCustomComponent().Reset();
      emp_assert(eval_hardware->ValidateThreadState());
      emp_assert(eval_hardware->GetActiveThreadIDs().size() == 0);
      emp_assert(eval_hardware->GetNumQueuedEvents() == 0);
      // Select a random environment.
      emp_assert(env_update % NUM_ENV_STATES < eval_environment.env_schedule.size());
      emp_assert(eval_environment.env_schedule[env_update % NUM_ENV_STATES] < eval_environment.env_state_tags.size());
      eval_environment.cur_state = eval_environment.env_schedule[env_update % NUM_ENV_STATES]; //random_ptr->GetUInt(0, NUM_ENV_STATES);
      eval_hardware->QueueEvent(event_t(event_id__env_sig, eval_environment.GetCurEnvTag()));
      // Step the hardware! If at any point, there are not active || pending threads, we're done!
      for (size_t step = 0; step < CPU_CYCLES_PER_ENV_UPDATE; ++step) {
        eval_hardware->SingleProcess();
        if (!( eval_hardware->GetNumActiveThreads() || eval_hardware->GetNumPendingThreads() )) break;
      }
      // Did the hardware match, miss, or not respond to environment signal?
      if (eval_hardware->GetCustomComponent().HasResponse()) {
        trial_phen.env_matches += (size_t)(eval_hardware->GetCustomComponent().GetResponse() == (int)eval_environment.cur_state);
        trial_phen.env_misses += (size_t)(eval_hardware->GetCustomComponent().GetResponse() != (int)eval_environment.cur_state);
      } else {
        trial_phen.no_responses += 1;
      }
    }
    trial_phen.score = (double)trial_phen.env_matches; // Score = number of times organism matched environment.
    if (trial_phen.GetScore() < trial_phenotypes[min_trial_id].GetScore()) {
      min_trial_id = trial_id;
    }
  }
  // Organism phenotype = min trial phenotype
  org.GetPhenotype().env_matches = trial_phenotypes[min_trial_id].env_matches;
  org.GetPhenotype().env_misses = trial_phenotypes[min_trial_id].env_misses;
  org.GetPhenotype().no_responses = trial_phenotypes[min_trial_id].no_responses;
  org.GetPhenotype().score = trial_phenotypes[min_trial_id].score;
}

/// Analyze organism
void ChgEnvWorld::AnalyzeOrg(const org_t & org, size_t org_id/*=0*/) {

  size_t trial_id = 0;
  org_t test_org(org);
  org_t ko_mem_org(org);
  org_t ko_reg_org(org);
  org_t ko_all_org(org);
  const size_t orig_eval_trial_cnt = EVAL_TRIAL_CNT;
  EVAL_TRIAL_CNT = 1;

  emp::DataFile analysis_file(OUTPUT_DIR + "/analysis_org_" + emp::to_string(org_id) + "_update_" + emp::to_string((int)GetUpdate()) + ".csv");
  analysis_file.template AddFun<size_t>([this]() { return this->GetUpdate(); }, "update");
  analysis_file.template AddFun<size_t>([&org_id]() { return org_id; }, "pop_id");
  analysis_file.template AddFun<size_t>([&trial_id]() { return trial_id; }, "trial_id");
  analysis_file.template AddFun<bool>([&test_org, this]() {
    org_t & org = test_org;
    return this->IsSolution(org.GetPhenotype());
  }, "solution");
  analysis_file.template AddFun<double>([&test_org]() {
    org_t & org = test_org;
    return org.GetPhenotype().GetScore();
  }, "score");
  analysis_file.template AddFun<double>([&ko_reg_org]() {
    return ko_reg_org.GetPhenotype().GetScore();
  }, "score_ko_regulation");
  analysis_file.template AddFun<double>([&ko_mem_org]() {
    return ko_mem_org.GetPhenotype().GetScore();
  }, "score_ko_global_memory");
  analysis_file.template AddFun<double>([&ko_all_org]() {
    return ko_all_org.GetPhenotype().GetScore();
  }, "score_ko_all");
  analysis_file.template AddFun<std::string>([this]() {
    std::ostringstream stream;
    stream << "\"[";
    for (size_t i = 0; i < eval_environment.env_schedule.size(); ++i) {
      if (i) stream << ",";
      stream << eval_environment.env_schedule[i];
    }
    stream << "]\"";
    return stream.str();
  }, "environment_seq");
  analysis_file.template AddFun<size_t>([this, &org_id]() {
    return this->GetOrg(org_id).GetGenome().GetProgram().GetSize();
  }, "num_modules");
  analysis_file.template AddFun<size_t>([this, &org_id]() {
    return this->GetOrg(org_id).GetGenome().GetProgram().GetInstCount();
  }, "num_instructions");
  // analysis_file.template AddFun<std::string>([this, &org_id]() {
  //   std::ostringstream stream;
  //   stream << "\"";
  //   this->PrintProgramSingleLine(this->GetOrg(org_id).GetGenome().GetProgram(), stream);
  //   stream << "\"";
  //   return stream.str();
  // }, "program");
  analysis_file.PrintHeaderKeys();

  // Analyze org across many trials.
  for (trial_id=0; trial_id < ANALYZE_ORG_EVAL_TRIALS; ++trial_id) {
    // Use consistent environment schedule for knockouts.
    emp::Shuffle(*random_ptr, eval_environment.env_schedule);
    // Evaluate org normally.
    EvaluateOrg(test_org, false);
    KO_GLOBAL_MEMORY = true;
    KO_REGULATION = false;
    EvaluateOrg(ko_mem_org, false);
    //     - ko regulation
    KO_GLOBAL_MEMORY = false;
    KO_REGULATION = true;
    EvaluateOrg(ko_reg_org, false);
    //     - ko memory & ko regulation
    KO_GLOBAL_MEMORY = true;
    KO_REGULATION = true;
    EvaluateOrg(ko_all_org, false);
    // Reset KO variables to both be false
    KO_GLOBAL_MEMORY = false;
    KO_REGULATION = false;
    analysis_file.Update();
  }
  KO_GLOBAL_MEMORY = false;
  KO_REGULATION = false;
  EVAL_TRIAL_CNT = orig_eval_trial_cnt;
  ////////////////////////////////////////////////
  // (2) Run a full trace of this organism.
  TraceOrganism(org, org_id);
}

/// Perform step-by-step execution trace on an organism, output
void ChgEnvWorld::TraceOrganism(const org_t & org, size_t org_id/*=0*/) {
  org_t trace_org(org); // Make a copy of the the given organism so we don't mess with any of the original's
                        // data.
  // Data file to store trace information.
  emp::DataFile trace_file(OUTPUT_DIR + "/trace_org_" + emp::to_string(org_id) + "_update_" + emp::to_string((int)GetUpdate()) + ".csv");
  // Add functions to trace file.
  HardwareStatePrintInfo hw_state_info;
  size_t env_update=0;
  size_t cpu_step=0;
  // ----- Timing information -----
  trace_file.template AddFun<size_t>([&env_update]() {
    return env_update;
  }, "env_update");
  trace_file.template AddFun<size_t>([&cpu_step]() {
    return cpu_step;
  }, "cpu_step");
  // ----- Environment Information -----
  //    * num_states
  trace_file.template AddFun<size_t>([this]() {
    return eval_environment.num_states;
  }, "num_env_states");
  //    * cur_state
  trace_file.template AddFun<size_t>([this]() {
    return eval_environment.cur_state;
  }, "cur_env_state");
  //    * env_signal_tag
  trace_file.template AddFun<std::string>([this]() {
    std::ostringstream stream;
    eval_environment.env_state_tags[eval_environment.cur_state].Print(stream);
    return stream.str();
  }, "cur_env_signal_tag");
  // ----- Hardware Information -----
  //    * current response
  trace_file.template AddFun<int>([this]() {
    return eval_hardware->GetCustomComponent().GetResponse();
  }, "cur_response");
  //    * correct responses
  trace_file.template AddFun<bool>([&trace_org, this]() {
    return eval_hardware->GetCustomComponent().response == (int)eval_environment.cur_state;
  }, "has_correct_response");
  //    * num_modules
  trace_file.template AddFun<size_t>([&hw_state_info]() {
    return hw_state_info.num_modules;
  }, "num_modules");
  //    * module_regulator_states
  trace_file.template AddFun<std::string>([&hw_state_info]() {
    std::ostringstream stream;
    stream << "\"[";
    for (size_t i = 0; i < hw_state_info.module_regulator_states.size(); ++i) {
      if (i) stream << ",";
      stream << hw_state_info.module_regulator_states[i];
    }
    stream << "]\"";
    return stream.str();
  }, "module_regulator_states");
  //    * which module would be triggered by the environment signal?
  trace_file.template AddFun<int>([this]() {
    const auto matches = eval_hardware->GetMatchBin().Match(eval_environment.env_state_tags[eval_environment.cur_state]);
    if (matches.size()) return (int)matches[0];
    else return -1;
  }, "env_signal_closest_match");
  //    * match scores against environment signal tag for each module
  trace_file.template AddFun<std::string>([this]() {
    const auto match_scores = eval_hardware->GetMatchBin().ComputeMatchScores(eval_environment.env_state_tags[eval_environment.cur_state]);
    emp::vector<double> scores(eval_hardware->GetNumModules());
    std::ostringstream stream;
    for (const auto & pair : match_scores) {
      const size_t module_id = pair.first;
      const double match_score = pair.second;
      scores[module_id] = match_score;
    }
    stream << "\"[";
    for (size_t i = 0; i < scores.size(); ++i) {
      if (i) stream << ",";
      stream << scores[i];
    }
    stream << "]\"";
    return stream.str();
  }, "env_signal_match_scores");
  //    * module_regulator_timers
  trace_file.template AddFun<std::string>([&hw_state_info]() {
    std::ostringstream stream;
    stream << "\"[";
    for (size_t i = 0; i < hw_state_info.module_regulator_timers.size(); ++i) {
      if (i) stream << ",";
      stream << hw_state_info.module_regulator_timers[i];
    }
    stream << "]\"";
    return stream.str();
  }, "module_regulator_timers");
  //    * global_mem_str
  trace_file.template AddFun<std::string>([&hw_state_info]() {
    return "\"" + hw_state_info.global_mem_str + "\"";
  }, "global_mem");
  //    * num_active_threads
  trace_file.template AddFun<size_t>([&hw_state_info]() {
    return hw_state_info.num_active_threads;
  }, "num_active_threads");
  //    * thread_state_str
  trace_file.template AddFun<std::string>([&hw_state_info]() {
    return "\"" + hw_state_info.thread_state_str + "\"";
  }, "thread_state_info");
  trace_file.PrintHeaderKeys();

  // ---- Do an traced-evaluation ----
  trace_org.GetPhenotype().Reset();
  // Ready the hardware!
  eval_hardware->SetProgram(trace_org.GetGenome().program);
  // Reset trial phenotype
  phenotype_t trace_phen = trace_org.GetPhenotype();
  trace_phen.Reset();
  // reset the environment
  eval_environment.ResetEnv();
  // shuffle environment schedule for this trial
  emp::Shuffle(*random_ptr, eval_environment.env_schedule);
  // reset hardware matchbin between trials
  eval_hardware->ResetMatchBin();
  // Evaluate the organism in the environment.
  for (env_update = 0; env_update < NUM_ENV_UPDATES; ++env_update) {
    // Reset the hardware!
    eval_hardware->ResetHardwareState();
    eval_hardware->GetCustomComponent().Reset();
    emp_assert(eval_hardware->ValidateThreadState());
    emp_assert(eval_hardware->GetActiveThreadIDs().size() == 0);
    emp_assert(eval_hardware->GetNumQueuedEvents() == 0);
    // Select a random environment.
    emp_assert(env_update % NUM_ENV_STATES < eval_environment.env_schedule.size());
    emp_assert(eval_environment.env_schedule[env_update % NUM_ENV_STATES] < eval_environment.env_state_tags.size());
    eval_environment.cur_state = eval_environment.env_schedule[env_update % NUM_ENV_STATES]; //random_ptr->GetUInt(0, NUM_ENV_STATES);
    eval_hardware->QueueEvent(event_t(event_id__env_sig, eval_environment.GetCurEnvTag()));
    // Step the hardware! If at any point, there are not active || pending threads, we're done!
    cpu_step = 0;
    hw_state_info = GetHardwareStatePrintInfo(*eval_hardware);
    trace_file.Update(); // Always output state BEFORE time step advances
    while (cpu_step < CPU_CYCLES_PER_ENV_UPDATE) {
      eval_hardware->SingleProcess();
      ++cpu_step;
      hw_state_info = GetHardwareStatePrintInfo(*eval_hardware);
      trace_file.Update();
      if (!( eval_hardware->GetNumActiveThreads() || eval_hardware->GetNumPendingThreads() )) break;
    }
    // Did the hardware match, miss, or not respond to environment signal?
    if (eval_hardware->GetCustomComponent().HasResponse()) {
      trace_phen.env_matches += (size_t)(eval_hardware->GetCustomComponent().GetResponse() == (int)eval_environment.cur_state);
      trace_phen.env_misses += (size_t)(eval_hardware->GetCustomComponent().GetResponse() != (int)eval_environment.cur_state);
    } else {
      trace_phen.no_responses += 1;
    }
  }
  trace_phen.score = (double)trace_phen.env_matches; // Score = number of times organism matched environment.
}

void ChgEnvWorld::DoEvaluation() {
  max_fit_org_id = 0;
  for (size_t org_id = 0; org_id < this->GetSize(); ++org_id) {
    emp_assert(this->IsOccupied(org_id));
    EvaluateOrg(this->GetOrg(org_id));
    if (CalcFitnessID(org_id) > CalcFitnessID(max_fit_org_id)) max_fit_org_id = org_id;
    // Record phenotype information for this taxon
    after_eval_sig.Trigger(org_id);
  }
}

void ChgEnvWorld::DoSelection() {
  // Keeping it simple with tournament selection!
  emp::TournamentSelect(*this, TOURNAMENT_SIZE, POP_SIZE);
}

void ChgEnvWorld::DoUpdate() {
  // Log current update, Best fitness
  const double max_fit = CalcFitnessID(max_fit_org_id);
  found_solution = IsSolution(GetOrg(max_fit_org_id).GetPhenotype());
  std::cout << "update: " << GetUpdate() << "; ";
  std::cout << "best score (" << max_fit_org_id << "): " << max_fit << "; ";
  std::cout << "solution found: " << found_solution << std::endl;
  const size_t cur_update = GetUpdate();
  if (SUMMARY_RESOLUTION) {
    if ( !(cur_update % SUMMARY_RESOLUTION) || cur_update == GENERATIONS || (STOP_ON_SOLUTION & found_solution) ) max_fit_file->Update();
  }
  if (SNAPSHOT_RESOLUTION) {
    if ( !(cur_update % SNAPSHOT_RESOLUTION) || cur_update == GENERATIONS || (STOP_ON_SOLUTION & found_solution) ) {
      DoPopulationSnapshot();
      if (cur_update || (STOP_ON_SOLUTION & found_solution)) {
        systematics_ptr->Snapshot(OUTPUT_DIR + "/phylo_" + emp::to_string(cur_update) + ".csv");
        AnalyzeOrg(GetOrg(max_fit_org_id), max_fit_org_id);
      }
    }
  }
  Update();
  ClearCache();
}

void ChgEnvWorld::PrintProgramSingleLine(const program_t & prog, std::ostream & out) {
  out << "[";
  for (size_t func_id = 0; func_id < prog.GetSize(); ++func_id) {
    if (func_id) out << ",";
    PrintProgramFunction(prog[func_id], out);
  }
  out << "]";
}

void ChgEnvWorld::PrintProgramFunction(const program_function_t & func, std::ostream & out) {
  out << "{";
  // print function tags
  out << "[";
  for (size_t tag_id = 0; tag_id < func.GetTags().size(); ++tag_id) {
    if (tag_id) out << ",";
    func.GetTag(tag_id).Print(out);
  }
  out << "]:";
  // print instruction sequence
  out << "[";
  for (size_t inst_id = 0; inst_id < func.GetSize(); ++inst_id) {
    if (inst_id) out << ",";
    PrintProgramInstruction(func[inst_id], out);
  }
  out << "]";
  out << "}";
}

void ChgEnvWorld::PrintProgramInstruction(const inst_t & inst, std::ostream & out) {
  out << inst_lib->GetName(inst.GetID()) << "[";
  // print tags
  for (size_t tag_id = 0; tag_id < inst.GetTags().size(); ++tag_id) {
    if (tag_id) out << ",";
    inst.GetTag(tag_id).Print(out);
  }
  out << "](";
  // print args
  for (size_t arg_id = 0; arg_id < inst.GetArgs().size(); ++arg_id) {
    if (arg_id) out << ",";
    out << inst.GetArg(arg_id);
  }
  out << ")";
}

ChgEnvWorld::HardwareStatePrintInfo ChgEnvWorld::GetHardwareStatePrintInfo(hardware_t & hw) {
  HardwareStatePrintInfo print_info;
  // Collect global memory
  std::ostringstream gmem_stream;
  hw.GetMemoryModel().PrintMemoryBuffer(hw.GetMemoryModel().GetGlobalBuffer(), gmem_stream);
  print_info.global_mem_str = gmem_stream.str();

  // number of modules
  print_info.num_modules = hw.GetNumModules();
  // module regulator states & timers
  print_info.module_regulator_states.resize(hw.GetNumModules());
  print_info.module_regulator_timers.resize(hw.GetNumModules());
  for (size_t module_id = 0; module_id < hw.GetNumModules(); ++module_id) {
    const auto reg_state = hw.GetMatchBin().GetRegulator(module_id).state;
    const auto reg_timer = hw.GetMatchBin().GetRegulator(module_id).timer;
    print_info.module_regulator_states[module_id] = reg_state;
    print_info.module_regulator_timers[module_id] = reg_timer;
  }
  // number of active threads
  print_info.num_active_threads = hw.GetNumActiveThreads();
  // for each thread: {thread_id: {priority: ..., callstack: [...]}}
  // - priority
  // - callstack: [callstate, callstate, etc]
  //   - callstate: [memory, flowstack]
  //     - flowstack: [type, mp, ip, begin, end]
  print_info.thread_state_str = "[";
  bool comma = false;
  for (size_t thread_id : hw.GetThreadExecOrder()) {
    auto & thread = hw.GetThread(thread_id);
    std::ostringstream stream;
    if (comma) { stream << ","; }
    stream << "{";
    stream << "id:" << thread_id << ",";
    // thread priority
    stream << "priority:" << thread.GetPriority() << ",";
    // thread state
    stream << "state:";
    if (thread.IsDead()) { stream << "dead,"; }
    else if (thread.IsPending()) { stream << "pending,"; }
    else if (thread.IsRunning()) { stream << "running,"; }
    else { stream << "unknown,"; }
    // call stack
    stream << "call_stack:[";
    auto & call_stack = thread.GetExecState().GetCallStack();
    for (size_t i = 0; i < call_stack.size(); ++i) {
      auto & mem_state = thread.GetExecState().GetCallStack()[i].memory;
      auto & flow_stack = thread.GetExecState().GetCallStack()[i].flow_stack;
      if (i) stream << ",";
      // - call state -
      stream << "{";
      stream << "working_memory:";
      hw.GetMemoryModel().PrintMemoryBuffer(mem_state.GetWorkingMemory(), stream);
      stream << ",input_memory:";
      hw.GetMemoryModel().PrintMemoryBuffer(mem_state.GetInputMemory(), stream);
      stream << ",output_memory:";
      hw.GetMemoryModel().PrintMemoryBuffer(mem_state.GetOutputMemory(), stream);
      // - call state - flow stack -
      stream << ",flow_stack:[";
      for (size_t f = 0; f < flow_stack.size(); ++f) {
        auto & flow = flow_stack[f];
        if (f) stream << ",";
        stream << "{";
        // type
        stream << "type:";
        if (flow.IsBasic()) { stream << "basic,"; }
        else if (flow.IsWhileLoop()) { stream << "whileloop,"; }
        else if (flow.IsRoutine()) { stream << "routine,"; }
        else if (flow.IsCall()) { stream << "call,"; }
        else { stream << "unknown,"; }
        // mp
        stream << "mp:" << flow.mp << ",";
        // ip
        stream << "ip:" << flow.ip << ",";
        // begin
        stream << "begin:" << flow.begin << ",";
        // end
        stream << "end:" << flow.end;
        stream << "}";
      }
      stream << "]";
      stream << "}";
    }
    stream << "]";
    stream << "}";
    print_info.thread_state_str += stream.str();
    if (!comma) { comma=true; }
  }
  print_info.thread_state_str += "]";
  return print_info;
}

// -_-_-_-_- PUBLIC IMPLEMENTATIONS -_-_-_-_-
void ChgEnvWorld::Setup(const config_t & config) {
  // Localize configuration parameters
  InitConfigs(config);
  // Create instruction/event libraries
  InitInstLib();
  InitEventLib();
  // Initialize evaluation hardware
  InitHardware();
  std::cout << "Configured Hardware MatchBin: " << eval_hardware->GetMatchBin().name() << std::endl;
  // Initialize the evaluation environment
  InitEnvironment();
  // Initialize the organism mutator
  InitMutator();
  // How should the population be initialized?
  end_setup_sig.AddAction([this]() {
    std::cout << "Initializing population...";
    InitPop();
    std::cout << " Done" << std::endl;
    this->SetAutoMutate(); // Set to automutate after initializing population!
  });
  // Misc world configuration
  this->SetPopStruct_Mixed(true); // Population is well-mixed with synchronous generations.
  this->SetFitFun([this](org_t & org) {
    return org.GetPhenotype().GetScore();
  });
  MAX_SCORE = NUM_ENV_UPDATES;
  // Configure data collection/snapshots
  InitDataCollection();
  DoWorldConfigSnapshot(config);
  // End of setup!
  end_setup_sig.Trigger();
  setup = true;
}

void ChgEnvWorld::Run() {
  for (size_t u = 0; u <= GENERATIONS; ++u) {
    RunStep();
    if (STOP_ON_SOLUTION & found_solution) break;
  }
}

void ChgEnvWorld::RunStep() {
  // (1) evaluate pop, (2) select parents, (3) update world
  DoEvaluation();
  DoSelection();
  DoUpdate();
}

#endif