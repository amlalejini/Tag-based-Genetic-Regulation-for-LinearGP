/***
 * Directional Signal World
 *
 **/

#ifndef _DIR_SIG_WORLD_H
#define _DIR_SIG_WORLD_H

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
#include <algorithm>
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
#include "DirSignalOrg.h"
#include "DirSignalConfig.h"
#include "mutation_utils.h"
#include "Event.h"

namespace DirSigWorldDefs {
  #ifndef TAG_NUM_BITS
  #define TAG_NUM_BITS 64
  #endif
  constexpr size_t TAG_LEN = TAG_NUM_BITS;      // How many bits per tag?
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
      emp::UnifMod<emp::AsymmetricWrapMetric<TAG_LEN>>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "integer-symmetric",
      emp::UnifMod<emp::SymmetricWrapMetric<TAG_LEN>>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "hamming",
      emp::UnifMod<emp::HammingMetric<TAG_LEN>>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "hash",
      emp::UnifMod<emp::HashMetric<TAG_LEN>>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "streak",
      emp::UnifMod<emp::StreakMetric<TAG_LEN>>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "streak-exact",
      emp::UnifMod<emp::ExactDualStreakMetric<TAG_LEN>>,
    std::enable_if<false>
    >::type
    >::type
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

  using org_t = DirSigOrganism<emp::BitSet<TAG_LEN>,int>;
  using config_t = DirSigConfig;
}

struct DirSigCustomHardware {
  int response=-1;
  bool responded=false;

  void Reset() {
    response=-1;
    responded=false;
  }

  void SetResponse(int i) {
    responded = true;
    response = i;
  }

  int GetResponse() const { return response; }
  bool HasResponse() const { return responded; }
};

class DirSigWorld : public emp::World<DirSigWorldDefs::org_t> {
public:
  using tag_t = emp::BitSet<DirSigWorldDefs::TAG_LEN>;
  using inst_arg_t = int;
  using config_t = DirSigConfig;
  using custom_comp_t = DirSigCustomHardware;

  using org_t = DirSigOrganism<tag_t,inst_arg_t>;
  using phenotype_t = typename org_t::DirSigPhenotype;

  using matchbin_t = emp::MatchBin<DirSigWorldDefs::matchbin_val_t,
                                   DirSigWorldDefs::matchbin_metric_t,
                                   DirSigWorldDefs::matchbin_selector_t,
                                   DirSigWorldDefs::matchbin_regulator_t>;
  using mem_model_t = sgp::SimpleMemoryModel;
  using hardware_t = sgp::LinearFunctionsProgramSignalGP<mem_model_t,
                                                         tag_t,
                                                         inst_arg_t,
                                                         matchbin_t,
                                                         custom_comp_t>;
  using event_lib_t = typename hardware_t::event_lib_t;
  using base_event_t = typename hardware_t::event_t;
  using event_t = Event<DirSigWorldDefs::TAG_LEN>;
  using inst_lib_t = typename hardware_t::inst_lib_t;
  using inst_t = typename hardware_t::inst_t;
  using inst_prop_t = typename hardware_t::InstProperty;
  using program_t = typename hardware_t::program_t;
  using program_function_t = typename program_t::function_t;
  using mutator_t = MutatorLinearFunctionsProgram<hardware_t, tag_t, inst_arg_t>;

  using mut_landscape_t = emp::datastruct::mut_landscape_info<phenotype_t>;
  using systematics_t = emp::Systematics<org_t, typename org_t::genome_t, mut_landscape_t>;
  using taxon_t = typename systematics_t::taxon_t;

  /// Environment tracking struct.
  /// Used to track environment state information during an organism's evaluation.
  struct Environment {
    size_t num_states=0;
    size_t cur_state=0;
    tag_t right_env_tag;   ///< Indicates organism should do 'right' response.
    tag_t left_env_tag;   ///< Indicates organism should do 'left' response.

    void ResetEnv() {
      cur_state=0;
    }

    /// Shift environment state to right, return new environment state.
    size_t ShiftRight() {
      if (!num_states || cur_state == num_states-1) {
        cur_state = 0;
      } else {
        cur_state += 1;
      }
      return cur_state;
    }

    /// Shift environment state to the left, return new environment state.
    size_t ShiftLeft() {
      emp_assert(cur_state < num_states);
      if (!num_states) {
        cur_state=0;
      } else if (cur_state == 0) {
        cur_state = num_states - 1;
      } else {
        cur_state -= 1;
      }
      return cur_state;
    }

    const tag_t & GetShiftLeftTag() const {
      return left_env_tag;
    }

    const tag_t & GetShiftRightTag() const {
      return right_env_tag;
    }
  };

  /// Struct used to track SignalGP hardware state.
  struct HardwareStatePrintInfo {
    std::string global_mem_str="";
    size_t num_modules=0;
    emp::vector<double> module_regulator_states;
    emp::vector<size_t> module_regulator_timers;
    // module values here
    size_t num_active_threads=0;
    std::string thread_state_str="";
  };

protected:
  // General settings group.
  size_t GENERATIONS;
  size_t POP_SIZE;
  bool STOP_ON_SOLUTION;
  // Evaluation group.
  size_t EVAL_TRIAL_CNT;
  size_t NUM_ENV_STATES;
  size_t NUM_ENV_UPDATES;
  size_t CPU_CYCLES_PER_ENV_UPDATE;
  // Program group
  bool USE_FUNC_REGULATION;
  bool USE_GLOBAL_MEMORY;
  emp::Range<size_t> FUNC_CNT_RANGE;
  emp::Range<size_t> FUNC_LEN_RANGE;
  // Hardware group
  size_t MAX_ACTIVE_THREAD_CNT;
  size_t MAX_THREAD_CAPACITY;
  // Selection group
  std::string SELECTION_MODE;
  size_t TEST_SAMPLE_SIZE;
  size_t TOURNAMENT_SIZE;
  // Mutation group
  double MUT_RATE__INST_ARG_SUB;
  double MUT_RATE__INST_SUB;
  double MUT_RATE__INST_INS;
  double MUT_RATE__INST_DEL;
  double MUT_RATE__SEQ_SLIP;
  double MUT_RATE__FUNC_DUP;
  double MUT_RATE__FUNC_DEL;
  double MUT_RATE__INST_TAG_BF;
  double MUT_RATE__FUNC_TAG_BF;
  // Data collection group
  std::string OUTPUT_DIR;
  size_t SUMMARY_RESOLUTION;
  size_t SNAPSHOT_RESOLUTION;

  Environment eval_environment;

  emp::vector<emp::BitVector> possible_dir_sequences;
  emp::vector<size_t> dir_seq_ids;

  bool setup = false;
  emp::Ptr<inst_lib_t> inst_lib;            ///< Manages SignalGP instruction set.
  emp::Ptr<event_lib_t> event_lib;          ///< Manages SignalGP events.
  emp::Ptr<mutator_t> mutator;

  size_t event_id__left_env_sig;
  size_t event_id__right_env_sig;
  emp::Ptr<hardware_t> eval_hardware;        ///< Used to evaluate programs.
  emp::vector<phenotype_t> trial_phenotypes; ///< Used to track phenotypes across organism evaluation trials.

  emp::Signal<void(size_t)> after_eval_sig; ///< Triggered after organism (ID given by size_t argument) evaluation
  emp::Signal<void(void)> end_setup_sig;    ///< Triggered at end of world setup.
  emp::Signal<void(void)> do_selection_sig; ///< Triggered when it's time to do selection!

  emp::Ptr<emp::DataFile> max_fit_file;
  emp::Ptr<systematics_t> systematics_ptr; ///< Short cut to correctly-typed systematics manager. Base class will be responsible for memory management.

  size_t max_fit_org_id=0;

  emp::vector< std::function<double(org_t &)> > lexicase_fit_funs;

  bool KO_REGULATION=false;
  bool KO_GLOBAL_MEMORY=false;

  void InitConfigs(const config_t & config);
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

  void AnalyzeOrg(const org_t & org, size_t org_id=0);
  void TraceOrganism(const org_t & org, size_t org_id=0);
  HardwareStatePrintInfo GetHardwareStatePrintInfo(hardware_t & hw);

  // -- Utilities --
  void DoPopulationSnapshot();
  void DoWorldConfigSnapshot(const config_t & config);
  void PrintProgramSingleLine(const program_t & prog, std::ostream & out);
  void PrintProgramFunction(const program_function_t & func, std::ostream & out);
  void PrintProgramInstruction(const inst_t & inst, std::ostream & out);

public:

  DirSigWorld() {}
  DirSigWorld(emp::Random & r) : emp::World<org_t>(r) {}

  ~DirSigWorld() {
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

// ---- Protected setup-function implementations ----
void DirSigWorld::InitConfigs(const config_t & config) {
  // General settings group.
  GENERATIONS = config.GENERATIONS();
  POP_SIZE = config.POP_SIZE();
  STOP_ON_SOLUTION = config.STOP_ON_SOLUTION();
  // Evaluation group.
  EVAL_TRIAL_CNT = config.EVAL_TRIAL_CNT();
  NUM_ENV_STATES = config.NUM_ENV_STATES();
  NUM_ENV_UPDATES = config.NUM_ENV_UPDATES();
  CPU_CYCLES_PER_ENV_UPDATE = config.CPU_CYCLES_PER_ENV_UPDATE();
  // Program group
  USE_FUNC_REGULATION = config.USE_FUNC_REGULATION();
  USE_GLOBAL_MEMORY = config.USE_GLOBAL_MEMORY();
  FUNC_CNT_RANGE = {config.MIN_FUNC_CNT(), config.MAX_FUNC_CNT()};
  FUNC_LEN_RANGE = {config.MIN_FUNC_INST_CNT(), config.MAX_FUNC_INST_CNT()};
  // Hardware group
  MAX_ACTIVE_THREAD_CNT = config.MAX_ACTIVE_THREAD_CNT();
  MAX_THREAD_CAPACITY = config.MAX_THREAD_CAPACITY();
  // Selection group
  SELECTION_MODE = config.SELECTION_MODE();
  TEST_SAMPLE_SIZE = config.TEST_SAMPLE_SIZE();
  TOURNAMENT_SIZE = config.TOURNAMENT_SIZE();
  // Mutation group
  MUT_RATE__INST_ARG_SUB = config.MUT_RATE__INST_ARG_SUB();
  MUT_RATE__INST_SUB = config.MUT_RATE__INST_SUB();
  MUT_RATE__INST_INS = config.MUT_RATE__INST_INS();
  MUT_RATE__INST_DEL = config.MUT_RATE__INST_DEL();
  MUT_RATE__SEQ_SLIP = config.MUT_RATE__SEQ_SLIP();
  MUT_RATE__FUNC_DUP = config.MUT_RATE__FUNC_DUP();
  MUT_RATE__FUNC_DEL = config.MUT_RATE__FUNC_DEL();
  MUT_RATE__INST_TAG_BF = config.MUT_RATE__INST_TAG_BF();
  MUT_RATE__FUNC_TAG_BF = config.MUT_RATE__FUNC_TAG_BF();
  // Data collection group
  OUTPUT_DIR = config.OUTPUT_DIR();
  SUMMARY_RESOLUTION = config.SUMMARY_RESOLUTION();
  SNAPSHOT_RESOLUTION = config.SNAPSHOT_RESOLUTION();
}

void DirSigWorld::InitInstLib() {
  if (!setup) inst_lib = emp::NewPtr<inst_lib_t>();
  inst_lib->Clear(); // Reset the instruction library
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
  inst_lib->AddInst("Countdown", sgp::lfp_inst_impl::Inst_Countdown<hardware_t, inst_t>, "", {inst_prop_t::BLOCK_DEF});
  inst_lib->AddInst("Routine", sgp::lfp_inst_impl::Inst_Routine<hardware_t, inst_t>, "");
  inst_lib->AddInst("Terminal", sgp::inst_impl::Inst_Terminal<hardware_t, inst_t>, "");

  // If we can use global memory, give programs access. Otherwise, nops.
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

  // if (allow regulation)
  // If we can use regulation, add instructions. Otherwise, nops.
  if (USE_FUNC_REGULATION) {
    inst_lib->AddInst("ClearRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_ClearRegulator<hardware_t, inst_t>(hw, inst);
    }, "");
    inst_lib->AddInst("ClearOwnRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_ClearOwnRegulator<hardware_t, inst_t>(hw, inst);
    }, "");
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
  } else {
    inst_lib->AddInst("Nop-ClearRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-ClearOwnRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-SetRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-SetOwnRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-AdjRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-AdjOwnRegulator", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
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

void DirSigWorld::InitEventLib() {
  if (!setup) event_lib = emp::NewPtr<event_lib_t>();
  event_lib->Clear();
  // Setup event: EnvSignal
  // Args: name, handler_fun, dispatchers, desc
  event_id__left_env_sig = event_lib->AddEvent("LeftSignal",
                                          [this](hardware_t & hw, const base_event_t & e) {
                                            const event_t & event = static_cast<const event_t&>(e);
                                            emp_assert(eval_environment.GetShiftLeftTag() == event.GetTag());
                                            hw.SpawnThreadWithTag(event.GetTag());
                                          });
  event_id__right_env_sig = event_lib->AddEvent("RightSignal",
                                        [this](hardware_t & hw, const base_event_t & e) {
                                          const event_t & event = static_cast<const event_t&>(e);
                                          emp_assert(eval_environment.GetShiftRightTag() == event.GetTag());
                                          hw.SpawnThreadWithTag(event.GetTag());
                                        });
}

void DirSigWorld::InitHardware() {
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

void DirSigWorld::InitEnvironment() {
  eval_environment.num_states = NUM_ENV_STATES;
  constexpr size_t tag_len = DirSigWorldDefs::TAG_LEN;
  const emp::vector<tag_t> lr_tags = emp::RandomBitSets<tag_len>(*random_ptr, 2, true);
  eval_environment.left_env_tag = lr_tags[0];
  eval_environment.right_env_tag = lr_tags[1];
  std::cout << "-- Environment Setup --" << std::endl;
  // Generate all possible environment sequences.
  const size_t num_sequences = emp::Pow2(NUM_ENV_UPDATES);
  possible_dir_sequences.resize(num_sequences, NUM_ENV_UPDATES);
  dir_seq_ids.resize(num_sequences);
  std::iota(dir_seq_ids.begin(), dir_seq_ids.end(), 0);
  // -- #angryface --
  // std::for_each(possible_dir_sequences.begin(), possible_dir_sequences.end(),
  //               [this, n=0](auto & vec) mutable {
  //                   std::cout << "n="<<n<<std::endl;
  //                   vec.SetUInt(0, n);
  //                   vec.Print();
  //                   std::cout << std::endl;
  //                   ++n;
  //               });
  // BitVector::SetUInt seems broken, so I'll just brute force this...
  // std::cout << "Possible lr environment direction sequences:" << std::endl;
  size_t strip_size = 1;
  bool val = false;
  size_t counter = 0;
  for (size_t pos = 0; pos < NUM_ENV_UPDATES; ++pos) {
    counter = 0;
    for (size_t i = 0; i < possible_dir_sequences.size(); ++i) {
      auto & vec = possible_dir_sequences[i];
      vec.Set(pos, val);
      ++counter;
      if (counter == strip_size) { val = !val; counter = 0; }
    }
    strip_size *= 2;
  }
  // print out direction sequences for sanity
  // for (uint32_t i = 0; i < possible_dir_sequences.size(); ++i) {
  //   std::cout << " " << i << ": ";
  //   auto & vec = possible_dir_sequences[i];
  //   vec.Print();
  //   std::cout << "; id=" << dir_seq_ids[i] << std::endl;
  // }
  eval_environment.ResetEnv();
  trial_phenotypes.resize(EVAL_TRIAL_CNT);
  std::for_each(trial_phenotypes.begin(), trial_phenotypes.end(), [this](auto & phen) {
    phen.Reset(TEST_SAMPLE_SIZE);
  });
}

void DirSigWorld::InitMutator() {
  if (!setup) { mutator = emp::NewPtr<mutator_t>(*inst_lib); }
  mutator->ResetLastMutationTracker();
  // Set program constraints
  mutator->SetProgFunctionCntRange(FUNC_CNT_RANGE);
  mutator->SetProgFunctionInstCntRange(FUNC_LEN_RANGE);
  mutator->SetProgInstArgValueRange({DirSigWorldDefs::INST_MIN_ARG_VAL, DirSigWorldDefs::INST_MAX_ARG_VAL});
  mutator->SetTotalInstLimit(2*FUNC_LEN_RANGE.GetUpper()*FUNC_CNT_RANGE.GetUpper());
  mutator->SetFuncNumTags(DirSigWorldDefs::FUNC_NUM_TAGS);
  mutator->SetInstNumTags(DirSigWorldDefs::INST_TAG_CNT);
  mutator->SetInstNumArgs(DirSigWorldDefs::INST_ARG_CNT);
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

void DirSigWorld::InitDataCollection() {
  emp_assert(!setup);
  if (setup) {
    max_fit_file.Delete();
  } else {
    mkdir(OUTPUT_DIR.c_str(), ACCESSPERMS);
    if(OUTPUT_DIR.back() != '/')
        OUTPUT_DIR += '/';
  }
  // -- generally useful functions --
  std::function<size_t(void)> get_update = [this]() { return this->GetUpdate(); };
  // -- fitness file --
  SetupFitnessFile(OUTPUT_DIR + "/fitness.csv").SetTimingRepeat(SUMMARY_RESOLUTION);
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
  //   - aggregate score, test_seqs, test_scores
  // - mutations (counts by type)
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) {
    return emp::to_string(taxon.GetData().GetFitness());
  }, "fitness", "Taxon fitness");

  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) {
    return emp::to_string(taxon.GetData().GetPhenotype().GetAggregateScore());
  }, "aggregate_score", "Aggregate score.");

  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) {
    std::ostringstream stream;
    const phenotype_t & phen = taxon.GetData().GetPhenotype();
    stream << "\"";
    for (size_t i = 0; i < phen.test_scores.size(); ++i) {
      if (i) stream << ",";
      stream << phen.test_scores[i];
    }
    stream << "\"";
    return stream.str();
  }, "scores_by_test", "Organism's scores on each test.");

  systematics_ptr->AddSnapshotFun([this](const taxon_t & taxon) {
    std::ostringstream stream;
    stream << "\"";
    const phenotype_t & phen = taxon.GetData().GetPhenotype();
    for (size_t i = 0; i < phen.test_ids.size(); ++i) {
      if (i) stream << ",";
      stream << phen.test_ids[i];
    }
    stream << "\"";
    return stream.str();
  }, "test_ids", "Test IDs. The bitstring representation of each ID gives the L/R direction sequence.");

  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
    if (taxon.GetData().HasMutationType("inst_arg_sub")) {
      return emp::to_string(taxon.GetData().GetMutationCount("inst_arg_sub"));
    } else {
      return "0";
    }
  }, "inst_arg_sub_mut_cnt", "How many mutations from parent taxon to this taxon?");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("inst_tag_bit_flip")) {
        return emp::to_string(taxon.GetData().GetMutationCount("inst_tag_bit_flip"));
      } else {
        return "0";
      }
    }, "inst_tag_bit_flip_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("inst_sub")) {
        return emp::to_string(taxon.GetData().GetMutationCount("inst_sub"));
      } else {
        return "0";
      }
    }, "inst_sub_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("inst_ins")) {
        return emp::to_string(taxon.GetData().GetMutationCount("inst_ins"));
      } else {
        return "0";
      }
    }, "inst_ins_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("inst_del")) {
        return emp::to_string(taxon.GetData().GetMutationCount("inst_del"));
      } else {
        return "0";
      }
    }, "inst_del_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("seq_slip_dup")) {
        return emp::to_string(taxon.GetData().GetMutationCount("seq_slip_dup"));
      } else {
        return "0";
      }
    }, "seq_slip_dup_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("seq_slip_del")) {
        return emp::to_string(taxon.GetData().GetMutationCount("seq_slip_del"));
      } else {
        return "0";
      }
    }, "seq_slip_del_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("func_dup")) {
        return emp::to_string(taxon.GetData().GetMutationCount("func_dup"));
      } else {
        return "0";
      }
    }, "func_dup_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("func_del")) {
        return emp::to_string(taxon.GetData().GetMutationCount("func_del"));
      } else {
        return "0";
      }
    }, "func_del_mut_cnt", "Mutation count");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("func_tag_bit_flip")) {
        return emp::to_string(taxon.GetData().GetMutationCount("func_tag_bit_flip"));
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
  // -- Dominant File --
  max_fit_file = emp::NewPtr<emp::DataFile>(OUTPUT_DIR + "/max_fit_org.csv");
  max_fit_file->AddFun(get_update, "update");
  max_fit_file->template AddFun<size_t>([this]() { return max_fit_org_id; }, "pop_id");

  max_fit_file->template AddFun<double>([this]() {
    return this->GetOrg(max_fit_org_id).GetPhenotype().GetAggregateScore();
  }, "aggregate_score");
  max_fit_file->template AddFun<std::string>([this]() {
    std::ostringstream stream;
    const phenotype_t & phen = this->GetOrg(max_fit_org_id).GetPhenotype();
    stream << "\"";
    for (size_t i = 0; i < phen.test_scores.size(); ++i) {
      if (i) stream << ",";
      stream << phen.test_scores[i];
    }
    stream << "\"";
    return stream.str();
  }, "scores_by_test", "Organism's scores on each test.");
  max_fit_file->template AddFun<std::string>([this]() {
    std::ostringstream stream;
    stream << "\"";
    const phenotype_t & phen = this->GetOrg(max_fit_org_id).GetPhenotype();
    for (size_t i = 0; i < phen.test_ids.size(); ++i) {
      if (i) stream << ",";
      stream << phen.test_ids[i];
    }
    stream << "\"";
    return stream.str();
  }, "test_ids", "Test IDs. The bitstring representation of each ID gives the L/R direction sequence.");
  max_fit_file->template AddFun<std::string>([this]() {
    std::ostringstream stream;
    stream << "\"";
    const phenotype_t & phen = this->GetOrg(max_fit_org_id).GetPhenotype();
    for (size_t i = 0; i < phen.test_ids.size(); ++i) {
      if (i) stream << ",";
      stream << possible_dir_sequences[phen.test_ids[i]];
    }
    stream << "\"";
    return stream.str();
  }, "test_seqs", "Test IDs. The bitstring representation of each ID gives the L/R direction sequence.");
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

void DirSigWorld::InitPop() {
  this->Clear();
  InitPop_Random();
}

void DirSigWorld::InitPop_Random() {
  for (size_t i = 0; i < POP_SIZE; ++i) {
    this->Inject({sgp::GenRandLinearFunctionsProgram<hardware_t, DirSigWorldDefs::TAG_LEN>
                                  (*random_ptr, *inst_lib,
                                   FUNC_CNT_RANGE,
                                   DirSigWorldDefs::FUNC_NUM_TAGS,
                                   FUNC_LEN_RANGE,
                                   DirSigWorldDefs::INST_TAG_CNT,
                                   DirSigWorldDefs::INST_ARG_CNT,
                                   {DirSigWorldDefs::INST_MIN_ARG_VAL,DirSigWorldDefs::INST_MAX_ARG_VAL})
                  }, 1);
  }
}

void DirSigWorld::InitPop_Hardcoded() {
  std::cout << "nope" << std::endl;
}

// ----- protected utility functions -----

void DirSigWorld::DoEvaluation() {
  // Sample tests! (shuffle test ids)
  emp::Shuffle(*random_ptr, dir_seq_ids);
  max_fit_org_id = 0;
  for (size_t org_id = 0; org_id < this->GetSize(); ++org_id) {
    emp_assert(this->IsOccupied(org_id));
    EvaluateOrg(this->GetOrg(org_id));
    if (CalcFitnessID(org_id) > CalcFitnessID(max_fit_org_id)) max_fit_org_id = org_id;
    // Record phenotype information for this taxon.
    after_eval_sig.Trigger(org_id);
  }
}

void DirSigWorld::DoSelection() {
  do_selection_sig.Trigger(); // outsourced to configurable signal trigger
}

void DirSigWorld::DoUpdate() {
  // Log current update, best fitnesses.
  const double max_score = CalcFitnessID(max_fit_org_id);
  const double max_possible = TEST_SAMPLE_SIZE * NUM_ENV_UPDATES;
  // found_solution = IsSolution(GetOrg())
  std::cout << "update: " << GetUpdate() << "; ";
  std::cout << "best score (" << max_fit_org_id << "): " << max_score << "; ";
  std::cout << "max score? " << (max_score >= max_possible) << std::endl;
  const size_t cur_update = GetUpdate();
  if (SUMMARY_RESOLUTION) {
    if (!(cur_update % SUMMARY_RESOLUTION) || cur_update == GENERATIONS ) {
      max_fit_file->Update();
    }
  }
  if (SNAPSHOT_RESOLUTION) {
    if (!(cur_update % SNAPSHOT_RESOLUTION) || cur_update == GENERATIONS) {
      DoPopulationSnapshot();
      if (cur_update) {
        systematics_ptr->Snapshot(OUTPUT_DIR + "/phylo_" + emp::to_string(cur_update) + ".csv");
        // AnalyzeOrg(GetOrg(max_fit_org_id), max_fit_org_id);
      }
    }
  }
  Update();
  ClearCache();
}

void DirSigWorld::EvaluateOrg(org_t & org) {
  // Evaluate given organism on each test (TEST_SAMPLE_SIZE) NUM_TRIALS number of times.
  // Fitness on a test = min(trial on test)
  org.GetPhenotype().Reset(TEST_SAMPLE_SIZE);
  // Ready the hardware
  eval_hardware->SetProgram(org.GetGenome().program);
  // Evaluate the organism on each sampled test (tests should be shuffled prior to evaluation).
  emp_assert(dir_seq_ids.size() == possible_dir_sequences.size());
  std::for_each(trial_phenotypes.begin(), trial_phenotypes.end(), [this](auto & phen) { phen.Reset(TEST_SAMPLE_SIZE); });
  for (size_t sample_id = 0; sample_id < TEST_SAMPLE_SIZE; ++sample_id) {
    const size_t test_id = sample_id % dir_seq_ids.size();         // Make sure we're sampling a valid test id.
    auto & env_seq = possible_dir_sequences[dir_seq_ids[test_id]]; // Grab the environment sequence we'll be using.
    size_t min_trial_id = 0;
    // std::cout << "===================================" << std::endl;
    // std::cout << "SAMPLE ID = " << sample_id << "; TEST ID = " << test_id << std::endl;
    // std::cout << "Env seq: "; env_seq.Print(); std::cout << std::endl;
    for (size_t trial_id = 0; trial_id < EVAL_TRIAL_CNT; ++trial_id) {
      // std::cout << "TRIAL ID = " << trial_id << std::endl;
      emp_assert(trial_id < trial_phenotypes.size());
      // Reset the trial phenotype
      phenotype_t & trial_phen = trial_phenotypes[trial_id];
      // Reset the environment.
      eval_environment.ResetEnv();
      // Reset the hardware matchbin between trials.
      eval_hardware->ResetMatchBin();
      // Evaluate the organism in the current environment sequence.
      emp_assert(env_seq.GetSize() == NUM_ENV_UPDATES);
      emp_assert(trial_phen.test_scores[sample_id] == 0);
      for (size_t env_update = 0; env_update < NUM_ENV_UPDATES; ++env_update) {
        // What direction will the environment be shifting? [0: left, 1: right]
        const bool dir = env_seq.Get(env_update);
        const size_t cur_env_state = (dir) ? eval_environment.ShiftRight() : eval_environment.ShiftLeft();
        // std::cout << "  Env update: " << env_update << "; Direction: " << dir << "; Current state: " << cur_env_state << std::endl;
        // Reset the hardware!
        eval_hardware->ResetHardwareState();
        eval_hardware->GetCustomComponent().Reset();
        emp_assert(eval_hardware->ValidateThreadState());
        emp_assert(eval_hardware->GetActiveThreadIDs().size() == 0);
        emp_assert(eval_hardware->GetNumQueuedEvents() == 0);
        if (dir) {
          // Shift right.
          eval_hardware->QueueEvent(event_t(event_id__right_env_sig, eval_environment.GetShiftRightTag()));
        } else {
          // Shift left.
          eval_hardware->QueueEvent(event_t(event_id__left_env_sig, eval_environment.GetShiftLeftTag()));
        }
        // Step the hardware forward to process the event.
        for (size_t step = 0; step < CPU_CYCLES_PER_ENV_UPDATE; ++step) {
          eval_hardware->SingleProcess();
          if (!( eval_hardware->GetNumActiveThreads() || eval_hardware->GetNumPendingThreads() )) break;
        }
        // How did the organism respond?
        if (eval_hardware->GetCustomComponent().HasResponse() && eval_hardware->GetCustomComponent().GetResponse() == (int)cur_env_state) {
          // Correct! Increment score.
          trial_phen.test_scores[sample_id] += 1;
          // std::cout << "  Correct response!" << std::endl;
        } else {
          // Incorrect! Bail out on trial evaluation.
          break;
        }
      }
      // std::cout << "  Test trial score = " << trial_phen.test_scores[sample_id] << std::endl;
      // is this trial the new worst trial?
      if (trial_phen.test_scores[sample_id] < trial_phenotypes[min_trial_id].test_scores[sample_id]) {
        min_trial_id = trial_id;
      }
    }
    phenotype_t & org_phen = org.GetPhenotype();
    const double min_trial_score = trial_phenotypes[min_trial_id].test_scores[sample_id];
    org_phen.test_scores[sample_id] = min_trial_score;
    org_phen.aggregate_score += min_trial_score;
    org_phen.test_ids[sample_id] = dir_seq_ids[test_id];
  }
  // std::cout << "Aggregate score = " << org.GetPhenotype().aggregate_score << std::endl;
  // std::for_each(org.GetPhenotype().test_scores.begin(), org.GetPhenotype().test_scores.end(), [i=0](double score) mutable {
    // std::cout << " => Sample " << i << " score = " << score << std::endl;
    // ++i;
  // });
  // std::cout << "EVAL DONE" << std::endl;
  // exit(-1);
}


void DirSigWorld::AnalyzeOrg(const org_t & org, size_t org_id/*=0*/) {
  // TODO
}

void DirSigWorld::TraceOrganism(const org_t & org, size_t org_id/*=0*/) {
  // TODO
}

DirSigWorld::HardwareStatePrintInfo DirSigWorld::GetHardwareStatePrintInfo(hardware_t & hw) {
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


// -- Utilities --
void DirSigWorld::DoPopulationSnapshot() {
  // Make a new data file for snapshot.
  emp::DataFile snapshot_file(OUTPUT_DIR + "/pop_" + emp::to_string((int)GetUpdate()) + ".csv");
  size_t cur_org_id = 0;
  // Add functions.
  snapshot_file.AddFun<size_t>([this]() { return this->GetUpdate(); }, "update");
  snapshot_file.AddFun<size_t>([this, &cur_org_id]() { return cur_org_id; }, "pop_id");
  // max_fit_file->AddFun(, "genotype_id");
  snapshot_file.template AddFun<double>([this, &cur_org_id]() {
    return this->GetOrg(cur_org_id).GetPhenotype().GetAggregateScore();
  }, "aggregate_score");
  snapshot_file.template AddFun<std::string>([this, &cur_org_id]() {
    std::ostringstream stream;
    const phenotype_t & phen = this->GetOrg(cur_org_id).GetPhenotype();
    stream << "\"";
    for (size_t i = 0; i < phen.test_scores.size(); ++i) {
      if (i) stream << ",";
      stream << phen.test_scores[i];
    }
    stream << "\"";
    return stream.str();
  }, "scores_by_test", "Organism's scores on each test.");
  snapshot_file.template AddFun<std::string>([this, &cur_org_id]() {
    std::ostringstream stream;
    stream << "\"";
    const phenotype_t & phen = this->GetOrg(cur_org_id).GetPhenotype();
    for (size_t i = 0; i < phen.test_ids.size(); ++i) {
      if (i) stream << ",";
      stream << phen.test_ids[i];
    }
    stream << "\"";
    return stream.str();
  }, "test_ids", "Test IDs. The bitstring representation of each ID gives the L/R direction sequence.");
  max_fit_file->template AddFun<std::string>([this, &cur_org_id]() {
    std::ostringstream stream;
    stream << "\"";
    const phenotype_t & phen = this->GetOrg(cur_org_id).GetPhenotype();
    for (size_t i = 0; i < phen.test_ids.size(); ++i) {
      if (i) stream << ",";
      stream << possible_dir_sequences[phen.test_ids[i]];
    }
    stream << "\"";
    return stream.str();
  }, "test_seqs", "Test Seqs. The bitstring representation gives the L/R direction sequence.");
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

void DirSigWorld::DoWorldConfigSnapshot(const config_t & config) {
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
  get_cur_value = []() { return emp::to_string(DirSigWorldDefs::TAG_LEN); };
  snapshot_file.Update();
  // INST_TAG_CNT
  get_cur_param = []() { return "INST_TAG_CNT"; };
  get_cur_value = []() { return emp::to_string(DirSigWorldDefs::INST_TAG_CNT); };
  snapshot_file.Update();
  // INST_ARG_CNT
  get_cur_param = []() { return "INST_ARG_CNT"; };
  get_cur_value = []() { return emp::to_string(DirSigWorldDefs::INST_ARG_CNT); };
  snapshot_file.Update();
  // FUNC_NUM_TAGS
  get_cur_param = []() { return "FUNC_NUM_TAGS"; };
  get_cur_value = []() { return emp::to_string(DirSigWorldDefs::FUNC_NUM_TAGS); };
  snapshot_file.Update();
  // INST_MIN_ARG_VAL
  get_cur_param = []() { return "INST_MIN_ARG_VAL"; };
  get_cur_value = []() { return emp::to_string(DirSigWorldDefs::INST_MIN_ARG_VAL); };
  snapshot_file.Update();
  // INST_MAX_ARG_VAL
  get_cur_param = []() { return "INST_MAX_ARG_VAL"; };
  get_cur_value = []() { return emp::to_string(DirSigWorldDefs::INST_MAX_ARG_VAL); };
  snapshot_file.Update();
  // environment signal
  get_cur_param = []() { return "env_state_tags"; };
  get_cur_value = [this]() {
    std::ostringstream stream;
    stream << "\"[";
    eval_environment.GetShiftLeftTag().Print(stream);
    stream << ",";
    eval_environment.GetShiftRightTag().Print(stream);
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

void DirSigWorld::PrintProgramSingleLine(const program_t & prog, std::ostream & out) {
  out << "[";
  for (size_t func_id = 0; func_id < prog.GetSize(); ++func_id) {
    if (func_id) out << ",";
    PrintProgramFunction(prog[func_id], out);
  }
  out << "]";
}

void DirSigWorld::PrintProgramFunction(const program_function_t & func, std::ostream & out) {
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

void DirSigWorld::PrintProgramInstruction(const inst_t & inst, std::ostream & out) {
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

// ------- PUBLIC FUNCTIONS --------

void DirSigWorld::Setup(const config_t & config) {
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
  // How should we do selection?
  if (SELECTION_MODE == "tournament") {
    do_selection_sig.AddAction([this]() {
      emp::TournamentSelect(*this, TOURNAMENT_SIZE, POP_SIZE);
    });
  } else if (SELECTION_MODE == "lexicase") {
    lexicase_fit_funs.clear();
    for (size_t i = 0; i < TEST_SAMPLE_SIZE; ++i) {
      lexicase_fit_funs.emplace_back([i](org_t & org) {
        emp_assert(i < org.GetPhenotype().test_scores.size());
        return org.GetPhenotype().test_scores[i];
      });
    }
    do_selection_sig.AddAction([this]() {
      emp::LexicaseSelect(*this, lexicase_fit_funs, POP_SIZE);
    });
  } else {
    std::cout << "UNKNOWN selection scheme ("<< SELECTION_MODE <<")!" << std::endl;
    exit(-1);
  }
  // Misc world configuration
  this->SetPopStruct_Mixed(true); // Population is well-mixed with synchronous generations.
  this->SetFitFun([this](org_t & org) {
    return org.GetPhenotype().GetAggregateScore();
  });
  // Configure data collection/snapshots
  InitDataCollection();
  DoWorldConfigSnapshot(config);
  // End of setup!
  end_setup_sig.Trigger();
  setup=true;
}

/// Advance world by a single time step (generation).
void DirSigWorld::RunStep() {
  // (1) evaluate population, (2) select parents, (3) update the world
  DoEvaluation();
  DoSelection();
  DoUpdate();
}

/// Run world for configured number of generations.
void DirSigWorld::Run() {
  for (size_t u = 0; u <= GENERATIONS; ++u) {
    RunStep();
  }
}

#endif
