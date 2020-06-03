/***
 * ====================================
 * == Boolean Logic Calculator World ==
 * ====================================
 *
 * Implements the boolean logic calculator program synthesis experiment
 * todo - add description here
 **/

#ifndef BOOL_CALC_WORLD_H
#define BOOL_CALC_WORLD_H

// macros courtesy of Matthew Andres Moreno
#ifndef SGP_REG_EXP_MACROS
#define STRINGVIEWIFY(s) std::string_view(IFY(s))
#define STRINGIFY(s) IFY(s)
#define IFY(s) #s
#define SGP_REG_EXP_MACROS
#endif

#include <functional>
#include <iostream>
#include <fstream>
#include <unordered_map>
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

#include "BoolCalcConfig.h"
#include "BoolCalcOrg.h"
#include "BoolCalcTestCase.h"
#include "Event.h"
#include "reg_ko_instr_impls.h"
#include "mutation_utils.h"

/// Globally-scoped, static variables.
namespace BoolCalcWorldDefs {
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
      emp::UnifMod<emp::AsymmetricWrapMetric<TAG_LEN>>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "integer-symmetric",
      emp::UnifMod<emp::SymmetricWrapMetric<TAG_LEN>>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "hamming",
      emp::UnifMod<emp::HammingMetric<TAG_LEN>>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "streak",
      emp::UnifMod<emp::StreakMetric<TAG_LEN>>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "streak-exact",
      emp::UnifMod<emp::ExactDualStreakMetric<TAG_LEN>>,
    std::enable_if<false>
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

  using org_t = BoolCalcOrganism<emp::BitSet<TAG_LEN>,int>;
  using config_t = BoolCalcConfig;
}

// - Inst_Add
template<typename HARDWARE_T, typename INSTRUCTION_T, typename OPERAND_T>
void Inst_Nand(HARDWARE_T & hw, const INSTRUCTION_T & inst) {
  auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
  auto & mem_state = call_state.GetMemory();
  const OPERAND_T a = (OPERAND_T)mem_state.AccessWorking(inst.GetArg(0));
  const OPERAND_T b = (OPERAND_T)mem_state.AccessWorking(inst.GetArg(1));
  const OPERAND_T result = ~(a&b);
  mem_state.SetWorking(inst.GetArg(2), result);
}

/// Custom hardware component for SignalGP
struct BoolCalcCustomHardware {

  using operand_t = BoolCalcTestInfo::operand_t;
  using response_t = BoolCalcTestInfo::RESPONSE_TYPE;

  response_t response_type=response_t::NONE;
  operand_t response_value=0;
  bool responded=false;
  int response_function_id=-1;

  void Reset() {
    response_type = response_t::NONE;
    response_value = 0;
    responded = false;
    response_function_id=-1;
  }

  void ExpressWait(int func_id=-1) {
    response_type = response_t::WAIT;
    responded = true;
    response_function_id=func_id;
  }

  void ExpressError(int func_id=-1) {
    response_type = response_t::ERROR;
    responded = true;
    response_function_id=func_id;
  }

  void ExpressResult(operand_t val, int func_id=-1) {
    response_type = response_t::NUMERIC;
    response_value = val;
    responded = false;
    response_function_id=func_id;
  }

  bool HasResponse() const { return responded; }
  response_t GetResponseType() const { return response_type; }
  operand_t GetResponseValue() const { return response_value; }
  int GetResponseFunctionID() const { return response_function_id; }
};

class BoolCalcWorld : public emp::World<BoolCalcWorldDefs::org_t> {
public:
  using tag_t = emp::BitSet<BoolCalcWorldDefs::TAG_LEN>;
  using inst_arg_t = int;
  using config_t = BoolCalcConfig;
  using operand_t = BoolCalcTestInfo::operand_t;
  using custom_comp_t = BoolCalcCustomHardware;

  using org_t = BoolCalcOrganism<tag_t, inst_arg_t>;
  using phenotype_t = typename org_t::phenotype_t;

  using matchbin_t = emp::MatchBin<BoolCalcWorldDefs::matchbin_val_t,
                                   BoolCalcWorldDefs::matchbin_metric_t,
                                   BoolCalcWorldDefs::matchbin_selector_t,
                                   BoolCalcWorldDefs::matchbin_regulator_t>;

  using mem_model_t = sgp::SimpleMemoryModel;
  using hardware_t = sgp::LinearFunctionsProgramSignalGP<mem_model_t,
                                                         tag_t,
                                                         inst_arg_t,
                                                         matchbin_t,
                                                         custom_comp_t>;
  using event_lib_t = typename hardware_t::event_lib_t;
  using base_event_t = typename hardware_t::event_t;
  using event_t = MessageEvent<BoolCalcWorldDefs::TAG_LEN>;
  using inst_lib_t = typename hardware_t::inst_lib_t;
  using inst_t = typename hardware_t::inst_t;
  using inst_prop_t = typename hardware_t::InstProperty;
  using program_t = typename hardware_t::program_t;
  using program_function_t = typename program_t::function_t;
  using mutator_t = MutatorLinearFunctionsProgram<hardware_t, tag_t, inst_arg_t>;
  using hw_response_type = BoolCalcTestInfo::RESPONSE_TYPE;

  using test_case_t = BoolCalcTestInfo::BoolCalcTestCase;

protected:
  // --- Localized Configuration Settings ---
  // Default group
  int SEED;
  size_t GENERATIONS;
  size_t POP_SIZE;
  bool STOP_ON_SOLUTION;
  // Evaluation group
  std::string TESTING_SET_FILE;
  std::string TRAINING_SET_FILE;
  // Selection group
  bool DOWN_SAMPLE;
  double DOWN_SAMPLE_RATE;
  // Program group
  bool USE_FUNC_REGULATION;
  bool USE_GLOBAL_MEMORY;
  emp::Range<size_t> FUNC_CNT_RANGE;
  emp::Range<size_t> FUNC_LEN_RANGE;
  emp::Range<int> ARG_VAL_RANGE;
  // Virtual hardware group
  size_t MAX_ACTIVE_THREAD_CNT;
  size_t MAX_THREAD_CAPACITY;
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

  bool setup=false;
  std::string output_path;

  emp::Ptr<inst_lib_t> inst_lib;            ///< Manages SignalGP instruction set.
  emp::Ptr<event_lib_t> event_lib;          ///< Manages SignalGP events.
  emp::Ptr<mutator_t> mutator;
  emp::Ptr<hardware_t> eval_hardware;        ///< Used to evaluate programs.

  emp::Signal<void(size_t)> after_eval_sig; ///< Triggered after organism (ID given by size_t argument) evaluation
  emp::Signal<void(void)> end_setup_sig;    ///< Triggered at end of world setup.
  emp::Signal<void(void)> do_selection_sig; ///< Triggered when it's time to do selection!

  emp::vector< std::function<double(org_t &)> > lexicase_fit_funs;  ///< Manages fitness functions if we're doing lexicase selection.
  emp::vector<size_t> training_case_ids;

  bool KO_REGULATION=false;       ///< Is regulation knocked out right now?
  bool KO_UP_REGULATION=false;    ///< Is up-regulation knocked out right now?
  bool KO_DOWN_REGULATION=false;  ///< Is down-regulation knocked out right now?
  bool KO_GLOBAL_MEMORY=false;    ///< Is global memory access knocked out right now?

  size_t event_id_input_sig=0;

  void InitConfigs(const config_t & config);
  void InitInstLib();
  void InitEventLib();
  void InitHardware();
  void InitMutator();
  void InitDataCollection();
  void InitSelection();

  void InitPop();
  void InitPop_Random();

  emp::vector<test_case_t> LoadTestCases(const std::string & path);

public:

  ~BoolCalcWorld() {
    // todo
  }

  void RunStep();
  void Run();
  void Setup(const config_t & config);
};

// ---- Public function implementations ----
void BoolCalcWorld::Setup(const config_t & config) {
  std:: cout << "--- Setting up BoolCalcWorld ---" << std::endl;
  setup = false;

  Reset(); // Reset the world
  this->SetAutoMutate(false); // Don't want mutations on population initialization.

  // Localize configuration parameters
  InitConfigs(config);
  // Reset the world's random number seed.
  random_ptr->ResetSeed(SEED);

  // Create instruction/event libraries
  InitInstLib();
  InitEventLib();
  // Initialize evaluation hardware
  InitHardware();
  // Initialize the program mutator
  InitMutator();
  // Initialize selection
  InitSelection();
  // How should the population be initialized?
  end_setup_sig.AddAction([this]() {
    std::cout << "Initializing the population..." << std::endl;
    InitPop();
    std::cout << " Done." << std::endl;
    this->SetAutoMutate(); // Set to automutate after initialization.
  });






  setup=true;
}

// ---- Internal function implementations ----
void BoolCalcWorld::InitConfigs(const config_t & config) {
  // General
  SEED = config.SEED();
  GENERATIONS = config.GENERATIONS();
  POP_SIZE = config.POP_SIZE();
  STOP_ON_SOLUTION = config.STOP_ON_SOLUTION();
  // Evaluation
  TESTING_SET_FILE = config.TESTING_SET_FILE();
  TRAINING_SET_FILE = config.TRAINING_SET_FILE();
  // Selection
  DOWN_SAMPLE = config.DOWN_SAMPLE();
  DOWN_SAMPLE_RATE = config.DOWN_SAMPLE_RATE();
  // Program
  USE_FUNC_REGULATION = config.USE_FUNC_REGULATION();
  USE_GLOBAL_MEMORY = config.USE_GLOBAL_MEMORY();
  FUNC_CNT_RANGE = {config.MIN_FUNC_CNT(), config.MAX_FUNC_CNT()};
  FUNC_LEN_RANGE = {config.MIN_FUNC_INST_CNT(), config.MAX_FUNC_INST_CNT()};
  ARG_VAL_RANGE = {config.INST_MIN_ARG_VAL(), config.INST_MAX_ARG_VAL()};
  // Hardware
  MAX_ACTIVE_THREAD_CNT = config.MAX_ACTIVE_THREAD_CNT();
  MAX_THREAD_CAPACITY = config.MAX_THREAD_CAPACITY();
  // Mutation
  MUT_RATE__INST_ARG_SUB = config.MUT_RATE__INST_ARG_SUB();
  MUT_RATE__INST_SUB = config.MUT_RATE__INST_SUB();
  MUT_RATE__INST_INS = config.MUT_RATE__INST_INS();
  MUT_RATE__INST_DEL = config.MUT_RATE__INST_DEL();
  MUT_RATE__SEQ_SLIP = config.MUT_RATE__SEQ_SLIP();
  MUT_RATE__FUNC_DUP = config.MUT_RATE__FUNC_DUP();
  MUT_RATE__FUNC_DEL = config.MUT_RATE__FUNC_DEL();
  MUT_RATE__INST_TAG_BF = config.MUT_RATE__INST_TAG_BF();
  MUT_RATE__FUNC_TAG_BF = config.MUT_RATE__FUNC_TAG_BF();
  // Data collection
  OUTPUT_DIR = config.OUTPUT_DIR();
  SUMMARY_RESOLUTION = config.SUMMARY_RESOLUTION();
  SNAPSHOT_RESOLUTION = config.SNAPSHOT_RESOLUTION();
}

void BoolCalcWorld::InitInstLib() {
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
  inst_lib->AddInst("Routine", sgp::lfp_inst_impl::Inst_Routine<hardware_t, inst_t>, "");
  inst_lib->AddInst("Terminal", sgp::inst_impl::Inst_Terminal<hardware_t, inst_t,
                                                              std::ratio<1>, std::ratio<-1>>, "");
  inst_lib->AddInst("Nand", Inst_Nand<hardware_t, inst_t, operand_t>, "Perform NAND");

  // If we can use global memory, give programs access. Otherwise, nops.
  if (USE_GLOBAL_MEMORY) {
    inst_lib->AddInst(
      "WorkingToGlobal",
      [this](hardware_t & hw, const inst_t & inst) {
        if (!KO_GLOBAL_MEMORY) sgp::inst_impl::Inst_WorkingToGlobal<hardware_t, inst_t>(hw, inst);
      },
      "Push working memory to global memory"
    );
    inst_lib->AddInst(
      "GlobalToWorking",
      [this](hardware_t & hw, const inst_t & inst) {
        if (!KO_GLOBAL_MEMORY) sgp::inst_impl::Inst_GlobalToWorking<hardware_t, inst_t>(hw, inst);
      },
      "Pull global memory into working memory"
    );
  } else {
    inst_lib->AddInst("Nop-WorkingToGlobal", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "Nop");
    inst_lib->AddInst("Nop-GlobalToWorking", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "Nop");
  }

  // If we can use regulation, add instructions. Otherwise, nops.
  if (USE_FUNC_REGULATION) {
    inst_lib->AddInst(
      "SetRegulator",
      [this](hardware_t & hw, const inst_t & inst) {
        if (KO_REGULATION) {
          return;
        } else if (KO_DOWN_REGULATION) {
          inst_impls::Inst_SetRegulator_KO_DOWN_REG<hardware_t, inst_t>(hw, inst);
        } else if (KO_UP_REGULATION) {
          inst_impls::Inst_SetRegulator_KO_UP_REG<hardware_t, inst_t>(hw, inst);
        } else {
          sgp::inst_impl::Inst_SetRegulator<hardware_t, inst_t>(hw, inst);
        }
      },
    ""
    );
    inst_lib->AddInst("SetRegulator-", [this](hardware_t & hw, const inst_t & inst) {
      if (KO_REGULATION) {
        return;
      } else if (KO_DOWN_REGULATION) {
        inst_impls::Inst_SetRegulator_KO_DOWN_REG<hardware_t, inst_t, -1>(hw, inst);
      } else if (KO_UP_REGULATION) {
        inst_impls::Inst_SetRegulator_KO_UP_REG<hardware_t, inst_t, -1>(hw, inst);
      } else {
        sgp::inst_impl::Inst_SetRegulator<hardware_t, inst_t, -1>(hw, inst);
      }
    }, "");

    inst_lib->AddInst("SetOwnRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (KO_REGULATION) {
        return;
      } else if (KO_DOWN_REGULATION) {
        inst_impls::Inst_SetOwnRegulator_KO_DOWN_REG<hardware_t, inst_t>(hw, inst);
      } else if (KO_UP_REGULATION) {
        inst_impls::Inst_SetOwnRegulator_KO_UP_REG<hardware_t, inst_t>(hw, inst);
      } else {
        sgp::inst_impl::Inst_SetOwnRegulator<hardware_t, inst_t>(hw, inst);
      }
    }, "");
    inst_lib->AddInst("SetOwnRegulator-", [this](hardware_t & hw, const inst_t & inst) {
      if (KO_REGULATION) {
        return;
      } else if (KO_DOWN_REGULATION) {
        inst_impls::Inst_SetOwnRegulator_KO_DOWN_REG<hardware_t, inst_t, -1>(hw, inst);
      } else if (KO_UP_REGULATION) {
        inst_impls::Inst_SetOwnRegulator_KO_UP_REG<hardware_t, inst_t, -1>(hw, inst);
      } else {
        sgp::inst_impl::Inst_SetOwnRegulator<hardware_t, inst_t, -1>(hw, inst);
      }
    }, "");

    inst_lib->AddInst("AdjRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (KO_REGULATION) {
        return;
      } else if (KO_DOWN_REGULATION) {
        inst_impls::Inst_AdjRegulator_KO_DOWN_REG<hardware_t, inst_t>(hw, inst);
      } else if (KO_UP_REGULATION) {
        inst_impls::Inst_AdjRegulator_KO_UP_REG<hardware_t, inst_t>(hw, inst);
      } else {
        sgp::inst_impl::Inst_AdjRegulator<hardware_t, inst_t>(hw, inst);
      }
    }, "");
    inst_lib->AddInst("AdjRegulator-", [this](hardware_t & hw, const inst_t & inst) {
      if (KO_REGULATION) {
        return;
      } else if (KO_DOWN_REGULATION) {
        inst_impls::Inst_AdjRegulator_KO_DOWN_REG<hardware_t, inst_t, -1>(hw, inst);
      } else if (KO_UP_REGULATION) {
        inst_impls::Inst_AdjRegulator_KO_UP_REG<hardware_t, inst_t, -1>(hw, inst);
      } else {
        sgp::inst_impl::Inst_AdjRegulator<hardware_t, inst_t, -1>(hw, inst);
      }
    }, "");
    inst_lib->AddInst("AdjOwnRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (KO_REGULATION) {
        return;
      } else if (KO_DOWN_REGULATION) {
        inst_impls::Inst_AdjOwnRegulator_KO_DOWN_REG<hardware_t, inst_t>(hw, inst);
      } else if (KO_UP_REGULATION) {
        inst_impls::Inst_AdjOwnRegulator_KO_UP_REG<hardware_t, inst_t>(hw, inst);
      } else {
        sgp::inst_impl::Inst_AdjOwnRegulator<hardware_t, inst_t>(hw, inst);
      }
    }, "");
    inst_lib->AddInst("AdjOwnRegulator-", [this](hardware_t & hw, const inst_t & inst) {
      if (KO_REGULATION) {
        return;
      } else if (KO_DOWN_REGULATION) {
        inst_impls::Inst_AdjOwnRegulator_KO_DOWN_REG<hardware_t, inst_t, -1>(hw, inst);
      } else if (KO_UP_REGULATION) {
        inst_impls::Inst_AdjOwnRegulator_KO_UP_REG<hardware_t, inst_t, -1>(hw, inst);
      } else {
        sgp::inst_impl::Inst_AdjOwnRegulator<hardware_t, inst_t, -1>(hw, inst);
      }
    }, "");

    inst_lib->AddInst("ClearRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (KO_REGULATION) {
        return;
      } else if (KO_DOWN_REGULATION) {
        inst_impls::Inst_ClearRegulator_KO_DOWN_REG<hardware_t, inst_t>(hw, inst);
      } else if (KO_UP_REGULATION) {
        inst_impls::Inst_ClearRegulator_KO_UP_REG<hardware_t, inst_t>(hw, inst);
      } else {
        sgp::inst_impl::Inst_ClearRegulator<hardware_t, inst_t>(hw, inst);
      }
    }, "");
    inst_lib->AddInst("ClearOwnRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (KO_REGULATION) {
        return;
      } else if (KO_DOWN_REGULATION) {
        inst_impls::Inst_ClearOwnRegulator_KO_DOWN_REG<hardware_t, inst_t>(hw, inst);
      } else if (KO_UP_REGULATION) {
        inst_impls::Inst_ClearOwnRegulator_KO_UP_REG<hardware_t, inst_t>(hw, inst);
      } else {
        sgp::inst_impl::Inst_ClearOwnRegulator<hardware_t, inst_t>(hw, inst);
      }
    }, "");
    inst_lib->AddInst("SenseRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_SenseRegulator<hardware_t, inst_t>(hw, inst);
    }, "");
    inst_lib->AddInst("SenseOwnRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (!KO_REGULATION) sgp::inst_impl::Inst_SenseOwnRegulator<hardware_t, inst_t>(hw, inst);
    }, "");

    inst_lib->AddInst("IncRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (KO_REGULATION || KO_DOWN_REGULATION) {
        return;
      } else {
        sgp::inst_impl::Inst_IncRegulator<hardware_t, inst_t>(hw, inst);
      }
    }, "");
    inst_lib->AddInst("IncOwnRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (KO_REGULATION || KO_DOWN_REGULATION) {
        return;
      } else {
        sgp::inst_impl::Inst_IncOwnRegulator<hardware_t, inst_t>(hw, inst);
      }
     }, "");
    inst_lib->AddInst("DecRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (KO_REGULATION || KO_UP_REGULATION) {
        return;
      } else {
        sgp::inst_impl::Inst_DecRegulator<hardware_t, inst_t>(hw, inst);
      }
    }, "");
    inst_lib->AddInst("DecOwnRegulator", [this](hardware_t & hw, const inst_t & inst) {
      if (KO_REGULATION || KO_UP_REGULATION) {
        return;
      } else {
        sgp::inst_impl::Inst_DecOwnRegulator<hardware_t, inst_t>(hw, inst);
      }
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
  inst_lib->AddInst(
    "ExpressWAIT",
    [](hardware_t & hw, const inst_t & inst) {
      const auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
      const auto & flow = call_state.GetTopFlow();
      const size_t mp = flow.GetMP();
      // Mark response on hardware.
      hw.GetCustomComponent().ExpressWait((int)mp);
      // Remove all pending threads
      hw.RemoveAllPendingThreads();
      // Mark all active threads as dead.
      for (size_t thread_id : hw.GetActiveThreadIDs()) {
        hw.GetThread(thread_id).SetDead();
      }
    },
    "Express WAIT response."
  );

  inst_lib->AddInst(
    "ExpressERROR",
    [](hardware_t & hw, const inst_t & inst) {
      const auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
      const auto & flow = call_state.GetTopFlow();
      const size_t mp = flow.GetMP();
      // Mark response on hardware.
      hw.GetCustomComponent().ExpressError((int)mp);
      // Remove all pending threads
      hw.RemoveAllPendingThreads();
      // Mark all active threads as dead.
      for (size_t thread_id : hw.GetActiveThreadIDs()) {
        hw.GetThread(thread_id).SetDead();
      }
    },
    "Express ERROR response."
  );

  inst_lib->AddInst(
    "ExpressResult",
    [](hardware_t & hw, const inst_t & inst) {
      auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
      auto & mem_state = call_state.GetMemory();
      const auto & flow = call_state.GetTopFlow();
      const size_t mp = flow.GetMP();
      // Get result
      const operand_t res = (operand_t)mem_state.AccessWorking(inst.GetArg(0));
      // Mark response on hardware.
      hw.GetCustomComponent().ExpressResult(res, (int)mp);
      // Remove all pending threads
      hw.RemoveAllPendingThreads();
      // Mark all active threads as dead.
      for (size_t thread_id : hw.GetActiveThreadIDs()) {
        hw.GetThread(thread_id).SetDead();
      }
    },
    "Express RESULT response."
  );
}

void BoolCalcWorld::InitEventLib() {
  if (!setup) event_lib = emp::NewPtr<event_lib_t>();
  event_lib->Clear();
  event_id_input_sig = event_lib->AddEvent(
    "InputSignal",
    [this](hardware_t & hw, const base_event_t & e) {
      const event_t & event = static_cast<const event_t&>(e);
      auto thread_id = hw.SpawnThreadWithTag(event.GetTag());
      if (thread_id && event.GetData().size()) {
        // If message resulted in thread being spawned, load message into local working space.
        auto & thread = hw.GetThread(thread_id.value());
        // Wait, wait. Does this thread have calls on the call stack?
        if (thread.GetExecState().call_stack.size()) {
          auto & call_state = thread.GetExecState().GetTopCallState();
          auto & mem_state = call_state.GetMemory();
          for (auto mem : event.GetData()) { mem_state.SetWorking(mem.first, mem.second); }
        }
      }
    }
  );
}

void BoolCalcWorld::InitHardware() {
  // If this is the first time through, create a new virtual hardware object.
  if (!setup) {
    eval_hardware = emp::NewPtr<hardware_t>(*random_ptr, *inst_lib, *event_lib);
  }
  // Configure SignalGP CPU
  eval_hardware->Reset();
  eval_hardware->SetActiveThreadLimit(MAX_ACTIVE_THREAD_CNT);
  eval_hardware->SetThreadCapacity(MAX_THREAD_CAPACITY);
  emp_assert(eval_hardware->ValidateThreadState());
}

void BoolCalcWorld::InitMutator() {
  if (!setup) { mutator = emp::NewPtr<mutator_t>(*inst_lib); }
  mutator->ResetLastMutationTracker();
  // Set program constraints
  mutator->SetProgFunctionCntRange(FUNC_CNT_RANGE);
  mutator->SetProgFunctionInstCntRange(FUNC_LEN_RANGE);
  mutator->SetProgInstArgValueRange(ARG_VAL_RANGE);
  mutator->SetTotalInstLimit(2*FUNC_LEN_RANGE.GetUpper()*FUNC_CNT_RANGE.GetUpper());
  mutator->SetFuncNumTags(BoolCalcWorldDefs::FUNC_NUM_TAGS);
  mutator->SetInstNumTags(BoolCalcWorldDefs::INST_TAG_CNT);
  mutator->SetInstNumArgs(BoolCalcWorldDefs::INST_ARG_CNT);
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
    // org.ResetMutations();                // Reset organism's recorded mutations.
    mutator->ResetLastMutationTracker(); // Reset mutator mutation tracking.
    const size_t mut_cnt = mutator->ApplyAll(rnd, org.GetGenome().program);
    return mut_cnt;
  });
}

void BoolCalcWorld::InitSelection() {

  // (1) Load testing set
  LoadTestCases(TESTING_SET_FILE);
  // (1.5) Load training set
  // (2) fill out test_set_ids
  // (3) initialize lexicase fitness functions

  // // How should we do selection?
  // if (DOWN_SAMPLE) {
  //   //
  // } else {
  //   // todo - what are fit funs?
  //   do_selection_sig.AddAction([this]() {
  //     emp::LexicaseSelect(*this, lexicase_fit_funs, POP_SIZE);
  //   });
  // }

}

void BoolCalcWorld::InitPop() {
  this->Clear();
  InitPop_Random();
}

void BoolCalcWorld::InitPop_Random() {
  for (size_t i = 0; i < POP_SIZE; ++i) {
    this->Inject({sgp::GenRandLinearFunctionsProgram<hardware_t, BoolCalcWorldDefs::TAG_LEN>
                                  (*random_ptr, *inst_lib,
                                   FUNC_CNT_RANGE,
                                   BoolCalcWorldDefs::FUNC_NUM_TAGS,
                                   FUNC_LEN_RANGE,
                                   BoolCalcWorldDefs::INST_TAG_CNT,
                                   BoolCalcWorldDefs::INST_ARG_CNT,
                                   ARG_VAL_RANGE)
                  }, 1);
  }
}

emp::vector<BoolCalcTestInfo::BoolCalcTestCase> BoolCalcWorld::LoadTestCases(const std::string & path) {
  std::ifstream tests_fstream(path);
  if (!tests_fstream.is_open()) {
    std::cout << "Failed to open test case file (" << path << "). Exiting..." << std::endl;
    exit(-1);
  }
  std::string cur_line;
  emp::vector<std::string> line_components;
  emp::vector<test_case_t> testcases;
  // If file is empty, failure!
  if (tests_fstream.eof()) return testcases;
  // Collect header
  std::getline(tests_fstream, cur_line);
  emp::slice(cur_line, line_components, ',');
  std::unordered_map<std::string, size_t> header_lu;
  for (size_t i = 0; i < line_components.size(); ++i) {
    header_lu[line_components[i]] = i;
    // std::cout << "header_lu[" << line_components[i] << "]=" << i << std::endl;
  }
  // Verify minimum required header information
  if (!(emp::Has(header_lu, "input") &&
        emp::Has(header_lu, "output") &&
        emp::Has(header_lu, "type")))
  {
    std::cout << "Invalid test case file contents ("<<path<<")" << std::endl;
    exit(-1);
  }
  // Extract tests
  // while (!tests_fstream.eof()) {


  // }
  return testcases;
}

#endif