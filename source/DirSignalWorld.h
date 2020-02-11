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

  emp::Ptr<emp::DataFile> max_fit_file;
  emp::Ptr<systematics_t> systematics_ptr; ///< Short cut to correctly-typed systematics manager. Base class will be responsible for memory management.

  size_t max_fit_org_id=0;

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
  event_id__left_env_sig = event_lib->AddEvent("RightSignal",
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
  std::cout << "Possible lr environment direction sequences:" << std::endl;
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
  // TODO
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
  // TODO
  std::cout << "nope" << std::endl;
}

// ----- protected utility functions -----

void DirSigWorld::DoEvaluation() {
  // todo
}
void DirSigWorld::DoSelection() {
  // todo
}
void DirSigWorld::DoUpdate() {
  // todo
}

void DirSigWorld::EvaluateOrg(org_t & org) {
  // TODO
}


void DirSigWorld::AnalyzeOrg(const org_t & org, size_t org_id/*=0*/) {
  // TODO
}

void DirSigWorld::TraceOrganism(const org_t & org, size_t org_id/*=0*/) {
  // TODO
}

HardwareStatePrintInfo DirSigWorld::GetHardwareStatePrintInfo(hardware_t & hw) {
  // TODO
}


// -- Utilities --
void DirSigWorld::DoPopulationSnapshot() {
  // TODO
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
  // INST_MIN_ARG_VAL
  get_cur_param = []() { return "INST_MIN_ARG_VAL"; };
  get_cur_value = []() { return emp::to_string(ChgEnvWorldDefs::INST_MIN_ARG_VAL); };
  snapshot_file.Update();
  // INST_MAX_ARG_VAL
  get_cur_param = []() { return "INST_MAX_ARG_VAL"; };
  get_cur_value = []() { return emp::to_string(ChgEnvWorldDefs::INST_MAX_ARG_VAL); };
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
  // TODO
}

void DirSigWorld::PrintProgramFunction(const program_function_t & func, std::ostream & out) {
  // TODO
}

void DirSigWorld::PrintProgramInstruction(const inst_t & inst, std::ostream & out) {
  // TODO
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
  // Misc world configuration
  this->SetPopStruct_Mixed(true); // Population is well-mixed with synchronous generations.
  this->SetFitFun([this](org_t & org) {
    return org.GetPhenotype().GetAggregateScore();
  });
  // Configure data collection/snapshots
  // InitDataCollection();
  // DoWorldConfigSnapshot(config);
  // End of setup!
  end_setup_sig.Trigger();
  setup=true;
}

#endif
