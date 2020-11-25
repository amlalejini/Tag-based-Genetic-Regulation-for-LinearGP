/***
 * ====================================
 * == Boolean Logic Calculator World ==
 * ====================================
 *
 * Implements the boolean logic calculator program synthesis experiment
 * todo - add description here

 * Note that support for custom categorical output was added after fully implementing this to support
 * the boolean logic calculator problem.
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
#include "random_utils.h"

#include "BoolCalcConfig.h"
#include "BoolCalcOrg.h"
#include "BoolCalcTestCase.h"
#include "Event.h"
#include "reg_ko_instr_impls.h"
#include "mutation_utils.h"
#include "matchbin_regulators.h"

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
      emp::AsymmetricWrapMetric<TAG_LEN>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "integer-symmetric",
      emp::SymmetricWrapMetric<TAG_LEN>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "hamming",
      emp::HammingMetric<TAG_LEN>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "streak",
      emp::StreakMetric<TAG_LEN>,
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "streak-exact",
      emp::ExactDualStreakMetric<TAG_LEN>,
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
    std::conditional<STRINGVIEWIFY(MATCH_REG) == "exp",
      ExponentialCountdownRegulator<>,
    std::enable_if<false>
    >::type
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
    responded = true;
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
  using hw_response_type_t = BoolCalcTestInfo::RESPONSE_TYPE;

  using test_case_t = BoolCalcTestInfo::TestCase;

  /// Struct used as intermediary for printing/outputting SignalGP hardware state at a given time step.
  struct HardwareStatePrintInfo {
    std::string global_mem_str="";      ///< String representation of global memory.
    size_t num_modules=0;               ///< Number of modules in the program loaded on hardware.
    emp::vector<double> module_regulator_states; ///< State of regulation for each module.
    emp::vector<size_t> module_regulator_timers; ///< Regulation timer for each module.
    size_t num_active_threads=0;        ///< How many active threads are running?
    std::string thread_state_str="";    ///< String representation of state of all threads.
  };

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
  size_t CPU_CYCLES_PER_INPUT_SIGNAL;
  bool CATEGORICAL_OUTPUT;

  // Selection group
  bool DOWN_SAMPLE;
  double DOWN_SAMPLE_RATE;
  bool SAMPLE_BY_TEST_TYPE;
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
  double MUT_RATE__INST_TAG_SINGLE_BF;
  double MUT_RATE__FUNC_TAG_SINGLE_BF;
  double MUT_RATE__INST_TAG_SEQ_RAND;
  double MUT_RATE__FUNC_TAG_SEQ_RAND;
  // Data collection group
  std::string OUTPUT_DIR;
  size_t SUMMARY_RESOLUTION;
  size_t SNAPSHOT_RESOLUTION;
  bool OUTPUT_PROGRAMS;

  bool setup=false;
  std::string output_path;
  bool found_solution=false;
  size_t max_fit_org_id=0;

  emp::Ptr<inst_lib_t> inst_lib;            ///< Manages SignalGP instruction set.
  emp::Ptr<event_lib_t> event_lib;          ///< Manages SignalGP events.
  emp::Ptr<mutator_t> mutator;
  emp::Ptr<hardware_t> eval_hardware;        ///< Used to evaluate programs.

  // emp::Signal<void(size_t)> after_eval_sig; ///< Triggered after organism (ID given by size_t argument) evaluation
  emp::Signal<void(void)> end_setup_sig;    ///< Triggered at end of world setup.
  emp::Signal<void(void)> do_selection_sig; ///< Triggered when it's time to do selection!

  emp::Ptr<emp::DataFile> max_fit_file;

  emp::vector< std::function<double(const org_t &)> > lexicase_fit_funs;  ///< Manages fitness functions if we're doing lexicase selection.
  emp::vector<size_t> training_case_ids;
  emp::vector<size_t> all_test_case_ids;
  emp::vector<test_case_t> training_cases;
  emp::vector<test_case_t> testing_cases;
  emp::vector<test_case_t> all_test_cases; ///< Used for screening for solutions

  emp::vector<std::string> test_case_types;                      ///< The list of all _types_ of test cases (e.g., NAND, NOT, ERROR_NUM_NUM, etc)
  std::unordered_map<std::string, size_t> test_case_type_ids;    ///< Lookup test case type id by type string
  emp::vector< emp::vector<size_t> > training_case_ids_by_type;  ///< training cases categorized by type
  // pre-compute sampling by type?
  emp::vector<size_t> training_case_sample_size_by_test_case_type; // todo - initselection this
  emp::vector<size_t> sampled_training_case_ids;

  size_t num_eval_tests;

  emp::vector<tag_t> test_input_signal_tags;        ///< Stored by ID
  emp::vector<std::string> test_input_signals;      ///< ...
  std::unordered_map<std::string, size_t> test_input_signal_lu;

  std::unordered_set<size_t> output_categories; ///< Used when CATEGORICAL_OUTPUT is true

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

  void DoEvaluation();
  void DoSelection();
  void DoUpdate();

  void EvaluateOrg(org_t & org,
                   const emp::vector<test_case_t> & tests,
                   const emp::vector<size_t> & test_eval_order,
                   size_t num_tests=0,
                   bool bail_on_fail=false);

  bool ScreenSolution(const org_t & org);

  void AnalyzeOrg(const org_t & org, size_t pop_id);

  void TraceOrganism(const org_t & org, size_t org_id=0); // TODO

  /// Output a snapshot of the world's configuration.
  void DoWorldConfigSnapshot(const config_t & config);
  void DoPopulationSnapshot();

  void PrintProgramSingleLine(const program_t & prog, std::ostream & out=std::cout);
  void PrintProgramFunction(const program_function_t & func, std::ostream & out=std::cout);
  void PrintProgramInstruction(const inst_t & inst, std::ostream & out=std::cout);
  /// Output utility - extract hardware state information from given SignalGP virtual hardware.
  HardwareStatePrintInfo GetHardwareStatePrintInfo(hardware_t & hw);

  emp::vector<test_case_t> LoadTestCases(const std::string & path);

  void ShuffleTrainingSampleByType() {
    // this function only works if we're down sampling by test type
    emp_assert(DOWN_SAMPLE && SAMPLE_BY_TEST_TYPE);
    // all test case types must be represented in training case sample types
    emp_assert(test_case_types.size() == training_case_sample_size_by_test_case_type.size());
    size_t sample_id = 0;
    for (size_t type_id = 0; type_id < test_case_types.size(); ++type_id) {
      const size_t type_sample_size = training_case_sample_size_by_test_case_type[type_id];
      emp::Shuffle(*random_ptr, training_case_ids_by_type[type_id]);
      emp_assert(type_sample_size <= training_case_ids_by_type[type_id].size());
      for (size_t i = 0; i < type_sample_size; ++i) {
        emp_assert(sample_id < sampled_training_case_ids.size());
        sampled_training_case_ids[sample_id] = training_case_ids_by_type[type_id][i];
        ++sample_id;
      }
    }
    // std::cout << "Sampled training case ids: " << sampled_training_case_ids << std::endl;
  }

  /// Compute the distribution of test types passed by a phenotype
  /// NOTE - this only works for phenotypes evaluated on the training set
  std::map<std::string, size_t> GetPassingTestTypeDistribution(const phenotype_t & phen) {
    std::map<std::string, size_t> distribution;
    for (const auto & type : test_case_types) distribution[type] = 0;
    for (size_t i = 0; i < phen.test_scores.size(); ++i) {
      const double score = phen.test_scores[i];
      const size_t test_id = phen.test_ids[i];
      emp_assert(test_id < training_cases.size());
      if (score < 1.0) continue;
      const test_case_t & test_case = training_cases[test_id];
      distribution[test_case.type_str] += 1;
    }
    return distribution;
  }

  /// Compute the distribution of test types evaluated by a phenotype
  /// NOTE - this only works for phenotypes evaluated on the training set
  std::map<std::string, size_t> GetEvalTestTypeDistribution(const phenotype_t & phen) {
    std::map<std::string, size_t> distribution;
    for (const auto & type : test_case_types) distribution[type] = 0;
    for (size_t i = 0; i < phen.test_ids.size(); ++i) {
      const size_t test_id = phen.test_ids[i];
      emp_assert(test_id < training_cases.size());
      const test_case_t & test_case = training_cases[test_id];
      distribution[test_case.type_str] += 1;
    }
    return distribution;
  }

public:

  ~BoolCalcWorld() {
    if(inst_lib) inst_lib.Delete();
    if(event_lib) event_lib.Delete();
    if(mutator) mutator.Delete();
    if(eval_hardware) eval_hardware.Delete();
    if(max_fit_file) max_fit_file.Delete();
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
  found_solution = false;
  // Localize configuration parameters
  InitConfigs(config);
  // Reset the world's random number seed.
  random_ptr->ResetSeed(SEED);

  // Initialize selection
  InitSelection();
  // Create instruction/event libraries
  InitInstLib();
  InitEventLib();
  // Initialize evaluation hardware
  InitHardware();
  // Initialize the program mutator
  InitMutator();

  // How should the population be initialized?
  end_setup_sig.AddAction([this]() {
    std::cout << "Initializing the population..." << std::endl;
    InitPop();
    std::cout << " Done." << std::endl;
    this->SetAutoMutate(); // Set to automutate after initialization.
  });
  // Misc. world configuration
  this->SetPopStruct_Mixed(true);
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

void BoolCalcWorld::RunStep() {
  DoEvaluation();
  DoSelection();
  DoUpdate();
}

void BoolCalcWorld::Run() {
  for (size_t u = 0; u <= GENERATIONS; ++u) {
    RunStep();
    if (STOP_ON_SOLUTION & found_solution) break;
  }
}

// ---- Internal function implementations ----
void BoolCalcWorld::DoEvaluation() {
  // If we're down sampling, shuffle the training cases.
  const bool use_samples_by_type = DOWN_SAMPLE && SAMPLE_BY_TEST_TYPE;
  if (use_samples_by_type) {
    ShuffleTrainingSampleByType();
  } else if (DOWN_SAMPLE) {
    emp::Shuffle(*random_ptr, training_case_ids);
  }

  max_fit_org_id = 0;
  for (size_t org_id = 0; org_id < GetSize(); ++org_id) {
    emp_assert(IsOccupied(org_id));
    EvaluateOrg(
      GetOrg(org_id),
      training_cases,
      (use_samples_by_type) ? sampled_training_case_ids : training_case_ids,
      num_eval_tests
    );
    if (CalcFitnessID(org_id) > CalcFitnessID(max_fit_org_id)) max_fit_org_id = org_id;
  }
}

void BoolCalcWorld::DoSelection() {
  do_selection_sig.Trigger();
}

void BoolCalcWorld::DoUpdate() {
  const double max_score = CalcFitnessID(max_fit_org_id);
  const double max_passes = GetOrg(max_fit_org_id).GetPhenotype().num_passes;
  const size_t cur_update = GetUpdate();

  found_solution = ScreenSolution(GetOrg(max_fit_org_id));
  // Flag this organism as a solution (for output purposes)!
  GetOrg(max_fit_org_id).GetPhenotype().is_solution = found_solution;

  std::cout << "update: " << cur_update << "; ";
  std::cout << "best score (" << max_fit_org_id << "): " << max_score << "; ";
  std::cout << "passes: " << max_passes << "; ";
  std::cout << "solution? " << found_solution << std::endl;

  if (SUMMARY_RESOLUTION) {
    const bool summarize = (!(cur_update % SUMMARY_RESOLUTION)) || (cur_update == GENERATIONS) || (STOP_ON_SOLUTION & found_solution);
    if (summarize) {
      max_fit_file->Update();
    }
  }

  if (SNAPSHOT_RESOLUTION) {
    const bool snapshot = (!(cur_update % SNAPSHOT_RESOLUTION)) || (cur_update == GENERATIONS) || (STOP_ON_SOLUTION & found_solution);
    if (snapshot) {
      DoPopulationSnapshot();
      if (cur_update || (STOP_ON_SOLUTION & found_solution)) {
        AnalyzeOrg(GetOrg(max_fit_org_id), max_fit_org_id);
      }
    }
  }

  Update();
  ClearCache();
}

// todo - modify this to support running on training, testing, or both
void BoolCalcWorld::EvaluateOrg(
  org_t & org,
  const emp::vector<test_case_t> & tests,
  const emp::vector<size_t> & test_eval_order,
  size_t num_tests/*=0*/,
  bool bail_on_fail/*=false*/
) {
  // Evaluate given organism on each test (num_eval_tests)
  if (!num_tests) num_tests = test_eval_order.size();
  emp_assert(num_tests <= test_eval_order.size(), num_tests, test_eval_order.size());
  emp_assert(num_tests <= tests.size(), num_tests, tests.size());
  phenotype_t & phen = org.GetPhenotype();
  phen.Reset(num_tests);

  // Ready the hardware
  eval_hardware->SetProgram(org.GetGenome().program); // This resets the hardware completely.
  // Evaluate program on each training example
  for (size_t eval_index = 0; eval_index < num_tests; ++eval_index) {
    emp_assert(eval_index < phen.test_scores.size());
    emp_assert(phen.test_scores[eval_index] == 0);
    eval_hardware->ResetMatchBin();       // Reset matchbin (regulation) between tests
    eval_hardware->ResetHardwareState();  // Reset global memory between tests
    // grab the test case id
    const size_t test_id = test_eval_order[eval_index];
    phen.test_ids[eval_index] = test_id;
    // grab the appropriate training example
    const test_case_t & test_case = tests[test_id];
    // compute amount of partial credit for each correct response to an input signal
    const double partial_credit = 1.0 / (double)test_case.test_signals.size();
    size_t num_correct_sig_resps = 0;
    for (size_t sig_i = 0; sig_i < test_case.test_signals.size(); ++sig_i) {
      const BoolCalcTestInfo::TestSignal & test_sig = test_case.test_signals[sig_i];
      const tag_t & test_sig_tag = test_input_signal_tags[test_sig.GetSignalID()];
      // Reset the hardware
      eval_hardware->ResetBaseHardwareState(); // Only reset threads, not global memory
      eval_hardware->GetCustomComponent().Reset();
      emp_assert(eval_hardware->ValidateThreadState());
      emp_assert(eval_hardware->GetActiveThreadIDs().size() == 0);
      emp_assert(eval_hardware->GetNumQueuedEvents() == 0);
      // Queue calculator button input
      if (test_sig.IsOperand()) {
        eval_hardware->QueueEvent(
          event_t(event_id_input_sig, test_sig_tag, {{0, (double)test_sig.GetOperand()}})
        );
      } else if (test_sig.IsOperator()) {
        eval_hardware->QueueEvent(
          event_t(event_id_input_sig, test_sig_tag)
        );
      }
      // Step the hardware forward to process the signal
      for (size_t step = 0; step < CPU_CYCLES_PER_INPUT_SIGNAL; ++step) {
        eval_hardware->SingleProcess();
        // Stop early if no active or pending threads
        const size_t num_active_threads = eval_hardware->GetNumActiveThreads();
        const size_t num_pending_threads = eval_hardware->GetNumPendingThreads();
        if (!( num_active_threads || num_pending_threads )) break;
      }
      // How did the organism respond?
      const bool has_response = eval_hardware->GetCustomComponent().HasResponse();
      const hw_response_type_t resp_type = eval_hardware->GetCustomComponent().GetResponseType();
      const operand_t resp_val = eval_hardware->GetCustomComponent().GetResponseValue();
      const bool is_correct = test_sig.IsCorrect(resp_type, resp_val);
      if (has_response && is_correct) {
        phen.test_scores[eval_index] += partial_credit;
        num_correct_sig_resps += 1;
      } else if (
          has_response &&
          test_sig.GetCorrectResponseType() == hw_response_type_t::NUMERIC &&
          resp_type == hw_response_type_t::NUMERIC)
      {
        phen.test_scores[eval_index] += 0.1*partial_credit; // get some credit
      } else {
        // Bail early
        break;
      }
    }
    // Did the program correctly respond to all input signals?
    // Update test score if necessary
    const bool pass = (num_correct_sig_resps == test_case.test_signals.size());
    if (pass) { phen.test_scores[eval_index] = 1.0; }
    else if (bail_on_fail) { break; } // Organism failed test and we want to bail on first fail.

    // Update number of passes
    phen.num_passes += (size_t)pass;

    // Update aggregate score
    phen.aggregate_score += phen.test_scores[eval_index];
  }
}

bool BoolCalcWorld::ScreenSolution(const org_t & org) {
  // How many tests did this organism pass?
  const size_t max_passes = org.GetPhenotype().num_passes;
  // Did this organism pass all the tests it was run on?
  const bool sol_candidate = max_passes >= org.GetPhenotype().test_scores.size();
  if (!sol_candidate) return false;
  // This organism passed all things it was tested on, so we'll screen it on the full training/testing sets.
  org_t screen_org(org);
  emp_assert(screen_org.GetGenome() == org.GetGenome());
  emp_assert(all_test_case_ids.size() == all_test_cases.size());
  EvaluateOrg(
    screen_org,               // Organism to evaluate
    all_test_cases,           // Test cases to evaluate organism on
    all_test_case_ids,        // Order to evaluate tests in (doesn't super matter)
    all_test_cases.size(),    // Number of tests (all of them for screening)
    true                      // Bail on fail?
  );
  const size_t screen_passes = screen_org.GetPhenotype().num_passes;
  return (screen_passes == all_test_cases.size());
}

void BoolCalcWorld::AnalyzeOrg(const org_t & org, size_t pop_id) {
  // Analyze organism w/knockouts
  org_t test_org(org);

  // Run normally
  EvaluateOrg(
    test_org,
    training_cases,
    training_case_ids,
    training_case_ids.size()
  );
  test_org.GetPhenotype().is_solution = ScreenSolution(test_org);

  TraceOrganism(org, pop_id);

  // Run with knockouts
  // - ko memory
  KO_GLOBAL_MEMORY=true;
  KO_REGULATION=false;
  KO_DOWN_REGULATION=false;
  KO_UP_REGULATION=false;
  org_t ko_mem_org(org);
  EvaluateOrg(
    ko_mem_org,
    training_cases,
    training_case_ids,
    training_case_ids.size()
  );
  ko_mem_org.GetPhenotype().is_solution = ScreenSolution(ko_mem_org);

  // - ko regulation
  KO_GLOBAL_MEMORY=false;
  KO_REGULATION=true;
  KO_DOWN_REGULATION=false;
  KO_UP_REGULATION=false;
  org_t ko_reg_org(org);
  EvaluateOrg(
    ko_reg_org,
    training_cases,
    training_case_ids,
    training_case_ids.size()
  );
  ko_reg_org.GetPhenotype().is_solution = ScreenSolution(ko_reg_org);

  // - ko memory & regulation
  KO_GLOBAL_MEMORY=true;
  KO_REGULATION=true;
  KO_DOWN_REGULATION=false;
  KO_UP_REGULATION=false;
  org_t ko_all_org(org);
  EvaluateOrg(
    ko_all_org,
    training_cases,
    training_case_ids,
    training_case_ids.size()
  );
  ko_all_org.GetPhenotype().is_solution = ScreenSolution(ko_all_org);

  // - ko up regulation
  KO_GLOBAL_MEMORY=false;
  KO_REGULATION=false;
  KO_DOWN_REGULATION=true;
  KO_UP_REGULATION=false;
  org_t ko_down_reg_org(org);
  EvaluateOrg(
    ko_down_reg_org,
    training_cases,
    training_case_ids,
    training_case_ids.size()
  );
  ko_down_reg_org.GetPhenotype().is_solution = ScreenSolution(ko_down_reg_org);

  // - ko down regulation
  KO_GLOBAL_MEMORY=false;
  KO_REGULATION=false;
  KO_DOWN_REGULATION=false;
  KO_UP_REGULATION=true;
  org_t ko_up_reg_org(org);
  EvaluateOrg(
    ko_up_reg_org,
    training_cases,
    training_case_ids,
    training_case_ids.size()
  );
  ko_up_reg_org.GetPhenotype().is_solution = ScreenSolution(ko_up_reg_org);

  // reset knockout variables
  KO_GLOBAL_MEMORY=false;
  KO_REGULATION=false;
  KO_DOWN_REGULATION=false;
  KO_UP_REGULATION=false;

  emp::DataFile analysis_file(
    OUTPUT_DIR + "/analysis_org_" + emp::to_string(pop_id) + "_update_" + emp::to_string(GetUpdate()) + ".csv"
  );

  // solution
  analysis_file.AddFun(
    std::function<bool()>(
      [&test_org]() {
        return test_org.GetPhenotype().IsSolution();
      }
    ),
    "solution"
  );
  // solution_ko_regulation
  analysis_file.AddFun(
    std::function<bool()>(
      [&ko_reg_org]() {
        return ko_reg_org.GetPhenotype().IsSolution();
      }
    ),
    "solution_ko_regulation"
  );
  // solution_ko_global_memory
  analysis_file.AddFun(
    std::function<bool()>(
      [&ko_mem_org]() {
        return ko_mem_org.GetPhenotype().IsSolution();
      }
    ),
    "solution_ko_global_memory"
  );
  // solution_ko_all
  analysis_file.AddFun(
    std::function<bool()>(
      [&ko_all_org]() {
        return ko_all_org.GetPhenotype().IsSolution();
      }
    ),
    "solution_ko_all"
  );
  // solution_ko_up_reg
  analysis_file.AddFun(
    std::function<bool()>(
      [&ko_up_reg_org]() {
        return ko_up_reg_org.GetPhenotype().IsSolution();
      }
    ),
    "solution_ko_up_reg"
  );
  // solution_ko_down_reg
  analysis_file.AddFun(
    std::function<bool()>(
      [&ko_down_reg_org]() {
        return ko_down_reg_org.GetPhenotype().IsSolution();
      }
    ),
    "solution_ko_down_reg"
  );
  // aggregate_score
  analysis_file.AddFun(
    std::function<double()>(
      [&test_org]() {
        return test_org.GetPhenotype().GetAggregateScore();
      }
    ),
    "aggregate_score"
  );
  // ko_regulation_aggregate_score
  analysis_file.AddFun(
    std::function<double()>(
      [&ko_reg_org]() {
        return ko_reg_org.GetPhenotype().GetAggregateScore();
      }
    ),
    "ko_regulation_aggregate_score"
  );
  // ko_global_memory_aggregate_score
  analysis_file.AddFun(
    std::function<double()>(
      [&ko_mem_org]() {
        return ko_mem_org.GetPhenotype().GetAggregateScore();
      }
    ),
    "ko_global_memory_aggregate_score"
  );
  // ko_all_aggregate_score
  analysis_file.AddFun(
    std::function<double()>(
      [&ko_all_org]() {
        return ko_all_org.GetPhenotype().GetAggregateScore();
      }
    ),
    "ko_all_aggregate_score"
  );
  // ko_up_reg_aggregate_score
  analysis_file.AddFun(
    std::function<double()>(
      [&ko_up_reg_org]() {
        return ko_up_reg_org.GetPhenotype().GetAggregateScore();
      }
    ),
    "ko_up_reg_aggregate_score"
  );
  // ko_down_reg_aggregate_score
  analysis_file.AddFun(
    std::function<double()>(
      [&ko_down_reg_org]() {
        return ko_down_reg_org.GetPhenotype().GetAggregateScore();
      }
    ),
    "ko_down_reg_aggregate_score"
  );
  // num_modules
  analysis_file.AddFun(
    std::function<size_t()>(
      [&test_org]() {
        return test_org.GetGenome().GetProgram().GetSize();
      }
    ),
    "num_modules"
  );
  // num_instructions
  analysis_file.AddFun(
    std::function<size_t()>(
      [&test_org]() {
        return test_org.GetGenome().GetProgram().GetInstCount();
      }
    ),
    "num_instructions"
  );
  // program
  analysis_file.AddFun(
    std::function<std::string()>(
      [this, &test_org]() {
        std::ostringstream stream;
        stream << "\"";
        PrintProgramSingleLine(test_org.GetGenome().GetProgram(), stream);
        stream << "\"";
        return stream.str();
      }
    ),
    "program"
  );
  analysis_file.PrintHeaderKeys();
  analysis_file.Update();
}

void BoolCalcWorld::TraceOrganism(const org_t & org, size_t org_id/*=0*/) {
  org_t trace_org(org); // Make a copy of the the given organism so we don't mess with any of the original's
                        // data.
  // Data file to store trace information.
  emp::DataFile trace_file(OUTPUT_DIR + "/trace_org_" + emp::to_string(org_id) + "_update_" + emp::to_string((int)GetUpdate()) + ".csv");
  // Add functions to trace file.
  HardwareStatePrintInfo hw_state_info;

  size_t cpu_step=0;
  size_t cur_test_id=0;
  size_t num_test_inputs=0;
  size_t cur_test_input=0;
  tag_t cur_test_input_tag=tag_t();
  BoolCalcTestInfo::TestSignal cur_test_signal=BoolCalcTestInfo::TestSignal(0, hw_response_type_t::NONE);

  emp::vector<inst_t> executed_instructions;

  inst_lib->OnBeforeInstExec(
    [&executed_instructions](hardware_t & hw, const inst_t & inst) {
      executed_instructions.emplace_back(inst);
    }
  );

  // ----- Timing information -----
  trace_file.template AddFun<size_t>([&cur_test_id]() {
    return cur_test_id;
  }, "cur_test_id");

  trace_file.template AddFun<size_t>([&cpu_step]() {
    return cpu_step;
  }, "cpu_step");

  // test information
  // - test id, input, input-sig-tag, current response

  // ----- Environment Information -----
  //    * num_test_inputs
  trace_file.template AddFun<size_t>([&num_test_inputs]() {
    return num_test_inputs;
  }, "num_test_inputs");

  //    * cur_test_input
  trace_file.template AddFun<size_t>([&cur_test_input]() {
    return cur_test_input;
  }, "cur_test_input");

  //    * cur_test_input_tag
  trace_file.template AddFun<std::string>([&cur_test_input_tag]() {
    std::ostringstream stream;
    cur_test_input_tag.Print(stream);
    return stream.str();
  }, "cur_test_input_tag");

  //    * cur_test_signal
  trace_file.template AddFun<std::string>([&cur_test_signal]() {
    std::ostringstream stream;
    stream << "\"";
    cur_test_signal.Print(stream);
    stream << "\"";
    return stream.str();
  }, "cur_test_signal");

// HasResponse
// GetResponseType
// GetResponseValue
// GetResponseFunctionID
  // ----- Hardware Information -----
  //    * current response
  trace_file.template AddFun<int>([this]() {
    return eval_hardware->GetCustomComponent().GetResponseValue();
  }, "cur_response_value");

  trace_file.template AddFun<std::string>([this]() {
    return BoolCalcTestInfo::ResponseStr(eval_hardware->GetCustomComponent().GetResponseType());
  }, "cur_response_type");

  trace_file.template AddFun<int>([this]() {
    return eval_hardware->GetCustomComponent().GetResponseFunctionID();
  }, "cur_responding_function");

  //    * correct responses
  trace_file.template AddFun<bool>([&trace_org, &cur_test_signal, this]() {
    return cur_test_signal.IsCorrect(
      eval_hardware->GetCustomComponent().GetResponseType(),
      eval_hardware->GetCustomComponent().GetResponseValue()
    );
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

  trace_file.template AddFun<std::string>(
    [&executed_instructions, this]() {
      std::ostringstream stream;
      stream << "\"[";
      for (size_t i = 0; i < executed_instructions.size(); ++i) {
        if (i) stream << ",";
        stream << inst_lib->GetName(executed_instructions[i].GetID());
      }
      stream << "]\"";
      executed_instructions.clear();
      return stream.str();
    },
    "executed_instructions"
  );

  //    * thread_state_str
  trace_file.template AddFun<std::string>([&hw_state_info]() {
    return "\"" + hw_state_info.thread_state_str + "\"";
  }, "thread_state_info");

  trace_file.PrintHeaderKeys();

  // -------------- Do an traced-evaluation --------------

  // If we're down sampling, shuffle the training cases.
  const bool use_samples_by_type = DOWN_SAMPLE && SAMPLE_BY_TEST_TYPE;
  if (use_samples_by_type) {
    ShuffleTrainingSampleByType();
  } else if (DOWN_SAMPLE) {
    emp::Shuffle(*random_ptr, training_case_ids);
  }

  const size_t num_tests = num_eval_tests;
  const emp::vector<test_case_t> & tests = training_cases;
  const emp::vector<size_t> & test_eval_order = (use_samples_by_type) ? sampled_training_case_ids : training_case_ids;

  phenotype_t & phen = trace_org.GetPhenotype();
  phen.Reset(num_tests);

  // Ready the hardware
  eval_hardware->SetProgram(trace_org.GetGenome().program); // This resets the hardware completely.
  // Evaluate program on each training example
  // cpu_step
  // cur_test_id [x]
  // num_test_inputs [x]
  // size_t cur_test_input
  // tag_t cur_test_input_tag
  // TestSignal cur_test_signal
  for (size_t eval_index = 0; eval_index < num_tests; ++eval_index) {
    emp_assert(eval_index < phen.test_scores.size());
    emp_assert(phen.test_scores[eval_index] == 0);
    eval_hardware->ResetMatchBin();       // Reset matchbin (regulation) between tests
    eval_hardware->ResetHardwareState();  // Reset global memory between tests
    // grab the test case id
    const size_t test_id = test_eval_order[eval_index];
    phen.test_ids[eval_index] = test_id;
    cur_test_id = test_id;
    // grab the appropriate training example
    const test_case_t & test_case = tests[test_id];
    num_test_inputs = test_case.test_signals.size();

    // compute amount of partial credit for each correct response to an input signal
    const double partial_credit = 1.0 / (double)test_case.test_signals.size();
    size_t num_correct_sig_resps = 0;
    for (size_t sig_i = 0; sig_i < test_case.test_signals.size(); ++sig_i) {

      const BoolCalcTestInfo::TestSignal & test_sig = test_case.test_signals[sig_i];
      const tag_t & test_sig_tag = test_input_signal_tags[test_sig.GetSignalID()];
      cur_test_input = sig_i;
      cur_test_signal = test_sig;
      cur_test_input_tag = test_sig_tag;

      // Reset the hardware
      eval_hardware->ResetBaseHardwareState(); // Only reset threads, not global memory
      eval_hardware->GetCustomComponent().Reset();
      emp_assert(eval_hardware->ValidateThreadState());
      emp_assert(eval_hardware->GetActiveThreadIDs().size() == 0);
      emp_assert(eval_hardware->GetNumQueuedEvents() == 0);
      // Queue calculator button input
      if (test_sig.IsOperand()) {
        eval_hardware->QueueEvent(
          event_t(event_id_input_sig, test_sig_tag, {{0, (double)test_sig.GetOperand()}})
        );
      } else if (test_sig.IsOperator()) {
        eval_hardware->QueueEvent(
          event_t(event_id_input_sig, test_sig_tag)
        );
      }

      cpu_step = 0;
      hw_state_info = GetHardwareStatePrintInfo(*eval_hardware);
      trace_file.Update();

      // Step the hardware forward to process the signal
      while (cpu_step < CPU_CYCLES_PER_INPUT_SIGNAL) {
        eval_hardware->SingleProcess();
        ++cpu_step;
        hw_state_info = GetHardwareStatePrintInfo(*eval_hardware);
        trace_file.Update();
        // Stop early if no active or pending threads
        const size_t num_active_threads = eval_hardware->GetNumActiveThreads();
        const size_t num_pending_threads = eval_hardware->GetNumPendingThreads();
        if (!( num_active_threads || num_pending_threads )) break;
      }

      // How did the organism respond?
      const bool has_response = eval_hardware->GetCustomComponent().HasResponse();
      const hw_response_type_t resp_type = eval_hardware->GetCustomComponent().GetResponseType();
      const operand_t resp_val = eval_hardware->GetCustomComponent().GetResponseValue();
      const bool is_correct = test_sig.IsCorrect(resp_type, resp_val);
      if (has_response && is_correct) {
        phen.test_scores[eval_index] += partial_credit;
        num_correct_sig_resps += 1;
      } else if (
          has_response &&
          test_sig.GetCorrectResponseType() == hw_response_type_t::NUMERIC &&
          resp_type == hw_response_type_t::NUMERIC
        )
      {
        phen.test_scores[eval_index] += 0.1*partial_credit; // get some credit
      } else {
        // Bail early
        break;
      }
    }

    // Did the program correctly respond to all input signals?
    // Update test score if necessary
    const bool pass = (num_correct_sig_resps == test_case.test_signals.size());
    if (pass) { phen.test_scores[eval_index] = 1.0; }

    // Update number of passes
    phen.num_passes += (size_t)pass;

    // Update aggregate score
    phen.aggregate_score += phen.test_scores[eval_index];
  }

  inst_lib->ResetBeforeInstExecSignal();
}

void BoolCalcWorld::InitConfigs(const config_t & config) {
  // General
  SEED = config.SEED();
  GENERATIONS = config.GENERATIONS();
  POP_SIZE = config.POP_SIZE();
  STOP_ON_SOLUTION = config.STOP_ON_SOLUTION();
  // Evaluation
  TESTING_SET_FILE = config.TESTING_SET_FILE();
  TRAINING_SET_FILE = config.TRAINING_SET_FILE();
  CPU_CYCLES_PER_INPUT_SIGNAL = config.CPU_CYCLES_PER_INPUT_SIGNAL();
  CATEGORICAL_OUTPUT = config.CATEGORICAL_OUTPUT();
  // Selection
  DOWN_SAMPLE = config.DOWN_SAMPLE();
  DOWN_SAMPLE_RATE = config.DOWN_SAMPLE_RATE();
  SAMPLE_BY_TEST_TYPE = config.SAMPLE_BY_TEST_TYPE();
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
  MUT_RATE__INST_TAG_SINGLE_BF = config.MUT_RATE__INST_TAG_SINGLE_BF();
  MUT_RATE__FUNC_TAG_SINGLE_BF = config.MUT_RATE__FUNC_TAG_SINGLE_BF();
  MUT_RATE__INST_TAG_SEQ_RAND = config.MUT_RATE__INST_TAG_SEQ_RAND();
  MUT_RATE__FUNC_TAG_SEQ_RAND = config.MUT_RATE__FUNC_TAG_SEQ_RAND();
  // Data collection
  OUTPUT_DIR = config.OUTPUT_DIR();
  SUMMARY_RESOLUTION = config.SUMMARY_RESOLUTION();
  SNAPSHOT_RESOLUTION = config.SNAPSHOT_RESOLUTION();
  OUTPUT_PROGRAMS = config.OUTPUT_PROGRAMS();
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

    inst_lib->AddInst(
      "FullWorkingToGlobal",
      [this](hardware_t & hw, const inst_t & inst) {
        if (!KO_GLOBAL_MEMORY) sgp::inst_impl::Inst_FullWorkingToGlobal<hardware_t, inst_t>(hw, inst);
      },
      "Push all working memory to global memory"
    );
    inst_lib->AddInst(
      "FullGlobalToWorking",
      [this](hardware_t & hw, const inst_t & inst) {
        if (!KO_GLOBAL_MEMORY) sgp::inst_impl::Inst_FullGlobalToWorking<hardware_t, inst_t>(hw, inst);
      },
      "Pull all global memory into working memory"
    );
  } else {
    inst_lib->AddInst("Nop-WorkingToGlobal", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "Nop");
    inst_lib->AddInst("Nop-GlobalToWorking", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "Nop");
    inst_lib->AddInst("Nop-FullWorkingToGlobal", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "Nop");
    inst_lib->AddInst("Nop-FullGlobalToWorking", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "Nop");
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

  // If output is categorical, create a separate instruction for each output category.
  if (CATEGORICAL_OUTPUT) {
    for (size_t resp : output_categories) {
      inst_lib->AddInst(
        "ExpressResp-" + emp::to_string(resp),
        [resp](hardware_t & hw, const inst_t & inst) {
          auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
          const auto & flow = call_state.GetTopFlow();
          const size_t mp = flow.GetMP();
          // Mark response on hardware.
          hw.GetCustomComponent().ExpressResult(resp, (int)mp);
          // Remove all pending threads
          hw.RemoveAllPendingThreads();
          // Mark all active threads as dead.
          for (size_t thread_id : hw.GetActiveThreadIDs()) {
            hw.GetThread(thread_id).SetDead();
          }
        }, "Express categorical RESULT response."
      );
    }
  } else {
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
  mutator->SetRateInstTagSingleBF(MUT_RATE__INST_TAG_SINGLE_BF);
  mutator->SetRateFuncTagSingleBF(MUT_RATE__FUNC_TAG_SINGLE_BF);
  mutator->SetRateInstTagSeqRand(MUT_RATE__INST_TAG_SEQ_RAND);
  mutator->SetRateFuncTagSeqRand(MUT_RATE__FUNC_TAG_SEQ_RAND);

  // Set world mutation function.
  this->SetMutFun([this](org_t & org, emp::Random & rnd) {
    // org.ResetMutations();                // Reset organism's recorded mutations.
    mutator->ResetLastMutationTracker(); // Reset mutator mutation tracking.
    const size_t mut_cnt = mutator->ApplyAll(rnd, org.GetGenome().program);
    return mut_cnt;
  });
}

void BoolCalcWorld::InitSelection() {

  // (1) Load training and testing sets
  training_cases = LoadTestCases(TRAINING_SET_FILE);
  testing_cases = LoadTestCases(TESTING_SET_FILE);


  // (2) fill out test_set_ids
  training_case_ids.resize(training_cases.size());
  std::iota(training_case_ids.begin(), training_case_ids.end(), 0);

  // (3) Associate tags with each type of input signal
  // - Numerics get a tag & all operator types get a tag
  // - First, find all types of operators
  std::unordered_set<std::string> test_types_set;
  std::unordered_set<std::string> operators;
  operators.emplace("OPERAND");
  for (const auto & test : training_cases) {
    test_types_set.emplace(test.type_str);
    for (const auto & input_signal : test.test_signals) {
      if (input_signal.IsOperator()) {
        operators.emplace(input_signal.GetOperator());
      }
    }
  }
  const size_t training_test_types_detected = test_types_set.size();
  std::cout << "Detected " << training_test_types_detected << " types of training examples." << std::endl;
  for (const auto & test : testing_cases) {
    test_types_set.emplace(test.type_str);
    for (const auto & input_signal : test.test_signals) {
      if (input_signal.IsOperator()) {
        operators.emplace(input_signal.GetOperator());
      }
    }
  }
  if (test_types_set.size() != training_test_types_detected) {
    std::cout << "WARNING: test case types detected in testing set that are not represented in training set." << std::endl;
  }
  if (CATEGORICAL_OUTPUT) {
    std::cout << "Detected " << output_categories.size() << " output categories." << std::endl;
  }
  // Collect all test case types/categories
  test_case_type_ids.clear();
  test_case_types.clear();
  for (const auto & tst_type : test_types_set) {
    test_case_type_ids[tst_type] = test_case_types.size();
    test_case_types.emplace_back(tst_type);
  }
  training_case_ids_by_type.clear();
  training_case_ids_by_type.resize(test_case_types.size(), emp::vector<size_t>());
  for (size_t training_id = 0; training_id < training_cases.size(); ++training_id) {
    test_case_t & test_case = training_cases[training_id];
    const size_t test_type_id = test_case_type_ids[test_case.type_str];
    test_case.type_id = test_type_id;
    training_case_ids_by_type[test_type_id].emplace_back(training_id);
  }

  // Assign each operator type an id
  for (const auto & op : operators) {
    test_input_signal_lu[op] = test_input_signals.size();
    test_input_signals.emplace_back(op);
  }
  // Annotate each testing/training case signal with signal id
  for (auto & test : training_cases) {
    for (auto & input_signal : test.test_signals) {
      if (input_signal.IsOperator()) {
        input_signal.signal_id = test_input_signal_lu[input_signal.GetOperator()];
      } else {
        input_signal.signal_id = test_input_signal_lu["OPERAND"];
      }
    }
  }
  for (auto & test : testing_cases) {
    for (auto & input_signal : test.test_signals) {
      if (input_signal.IsOperator()) {
        input_signal.signal_id = test_input_signal_lu[input_signal.GetOperator()];
      } else {
        input_signal.signal_id = test_input_signal_lu["OPERAND"];
      }
    }
  }

  // Generate random tags for each input signal
  constexpr size_t tag_len = BoolCalcWorldDefs::TAG_LEN;
  test_input_signal_tags = emp::RandomBitSets<tag_len>(*random_ptr, test_input_signals.size(), true);

  // Build combined test bank (used for solution screenings)
  all_test_cases.clear();
  for (const auto & test : testing_cases) { // Put testing cases first to maximize changes to bail screens early
    all_test_cases.emplace_back(test);
  }
  for (const auto & test : training_cases) {
    all_test_cases.emplace_back(test);
  }
  std::cout << "# training cases: " << training_cases.size() << std::endl;
  std::cout << "# testing cases: " << testing_cases.size() << std::endl;
  std::cout << "# total cases: " << all_test_cases.size() << std::endl;
  all_test_case_ids.resize(all_test_cases.size());
  std::iota(all_test_case_ids.begin(), all_test_case_ids.end(), 0);

  // (4) initialize lexicase fitness functions
  //  - Compute number of tests used during evaluation.
  if (DOWN_SAMPLE) {
    emp_assert(DOWN_SAMPLE_RATE <= 1.0 && DOWN_SAMPLE_RATE >= 0.0, "Invalid DOWN_SAMPLE_RATE", DOWN_SAMPLE_RATE);
    num_eval_tests = (size_t)((double)training_case_ids.size() * DOWN_SAMPLE_RATE);
  } else {
    num_eval_tests = training_case_ids.size();
  }
  emp_assert(num_eval_tests > 0);
  // inject support for sampling by test type (guaranteeing we evaluate each organism on each type of test)
  training_case_sample_size_by_test_case_type.resize(test_case_types.size(), 0);
  if (DOWN_SAMPLE && SAMPLE_BY_TEST_TYPE) {
    num_eval_tests = 0; // Recompute this.
    for (const auto & type : test_case_type_ids) {
      // const std::string & type_str = type.first;
      const size_t type_id = type.second;
      const size_t num_type_training_cases = training_case_ids_by_type[type_id].size();
      const size_t num_type_eval =  std::ceil(DOWN_SAMPLE_RATE * (double)num_type_training_cases);
      training_case_sample_size_by_test_case_type[type_id] = num_type_eval;
      num_eval_tests += num_type_eval;
      emp_assert(num_type_eval > 0, "No training case representation for test case type", type.first);
    }
    sampled_training_case_ids.resize(num_eval_tests, 0);
  }

  // Print test case counts by type
  std::cout << "Number of training examples to evaluate each generation: " << num_eval_tests << std::endl;
  for (const auto & type_id : test_case_type_ids) {
    std::cout << "  Test case type: " << type_id.first;
    std::cout << "; TypeID: " << type_id.second;
    std::cout << "; # training cases: " << training_case_ids_by_type[type_id.second].size();
    std::cout << "; # training sample eval: " << training_case_sample_size_by_test_case_type[type_id.second];
    // print associated ids
    // std::cout << "; training ids: " << training_case_ids_by_type[type_id.second];
    std::cout << std::endl;
  }

  //  - Initialize fitness function set
  for (size_t i = 0; i < num_eval_tests; ++i) {
    lexicase_fit_funs.push_back([i](const org_t & org) {
      emp_assert(i < org.GetPhenotype().test_scores.size(), i, org.GetPhenotype().test_scores.size());
      const double score = org.GetPhenotype().test_scores[i];
      return score;
    });
  }

  // (5) Wire up selection
  do_selection_sig.AddAction([this]() {
    emp::LexicaseSelect(*this, lexicase_fit_funs, POP_SIZE);
  });

  // TODO - output environment (as part of configuration)
}

void BoolCalcWorld::InitDataCollection() {
  if (setup) {
    max_fit_file.Delete();
  } else {
    mkdir(OUTPUT_DIR.c_str(), ACCESSPERMS);
    if(OUTPUT_DIR.back() != '/')
        OUTPUT_DIR += '/';
  }
  // ---- generally useful functions ----
  std::function<size_t(void)> get_update = [this]() { return this->GetUpdate(); };
  // ---- fitness file ----
  SetupFitnessFile(OUTPUT_DIR + "/fitness.csv").SetTimingRepeat(SUMMARY_RESOLUTION);
  // ---- setup max fit organism file ----
  max_fit_file = emp::NewPtr<emp::DataFile>(OUTPUT_DIR + "/max_fit_org.csv");
  // -- update --
  max_fit_file->AddFun(get_update, "update");
  // -- pop id --
  max_fit_file->template AddFun<size_t>(
    [this]() { return max_fit_org_id; },
    "pop_id"
  );
  // -- is_solution --
  max_fit_file->template AddFun<bool>(
    [this]() {
      return GetOrg(max_fit_org_id).GetPhenotype().IsSolution();
    }, "is_solution"
  );
  // -- agg fitness --
  max_fit_file->template AddFun<double>(
    [this]() {
      return GetOrg(max_fit_org_id).GetPhenotype().GetAggregateScore();
    },
    "aggregate_fitness"
  );
  // -- num passes --
  max_fit_file->template AddFun<size_t>(
    [this]() {
      return GetOrg(max_fit_org_id).GetPhenotype().num_passes;
    },
    "num_passes"
  );
  // -- total tests evaluated --
  max_fit_file->template AddFun<size_t>(
    [this]() {
      return GetOrg(max_fit_org_id).GetPhenotype().test_scores.size();
    },
    "total_tests"
  );
  // -- scores by test ("[]") --
  max_fit_file->template AddFun<std::string>(
    [this]() {
      std::ostringstream stream;
      const phenotype_t & phen = GetOrg(max_fit_org_id).GetPhenotype();
      stream << "\"[";
      for (size_t i = 0; i < phen.test_scores.size(); ++i) {
        if (i) stream << ",";
        stream << phen.test_scores[i];
      }
      stream << "]\"";
      return stream.str();
    },
    "scores_by_test"
  );
  // -- test ids ("[]") --
  max_fit_file->template AddFun<std::string>(
    [this]() {
      std::ostringstream stream;
      const phenotype_t & phen = GetOrg(max_fit_org_id).GetPhenotype();
      stream << "\"[";
      for (size_t i = 0; i < phen.test_ids.size(); ++i) {
        if (i) stream << ",";
        stream << phen.test_ids[i];
      }
      stream << "]\"";
      return stream.str();
    },
    "test_ids"
  );
  // -- distribution of test case passes --
  max_fit_file->template AddFun<std::string>(
    [this]() {
      const phenotype_t & phen = GetOrg(max_fit_org_id).GetPhenotype();
      std::map<std::string, size_t> distribution(GetPassingTestTypeDistribution(phen));
      std::ostringstream stream;
      stream << "\"{";
      bool comma=false;
      for (const auto & freq : distribution) {
        if (comma) { stream << ","; }
        stream << freq.first << ":" << freq.second;
        comma = true;
      }
      stream << "}\"";
      return stream.str();
    },
    "test_pass_distribution"
  );
  // -- distribution of test case fails --
  max_fit_file->template AddFun<std::string>(
    [this]() {
      const phenotype_t & phen = GetOrg(max_fit_org_id).GetPhenotype();
      std::map<std::string, size_t> distribution(GetEvalTestTypeDistribution(phen));
      std::ostringstream stream;
      stream << "\"{";
      bool comma=false;
      for (const auto & freq : distribution) {
        if (comma) { stream << ","; }
        stream << freq.first << ":" << freq.second;
        comma = true;
      }
      stream << "}\"";
      return stream.str();
    },
    "test_eval_distribution"
  );
  // -- num modules --
  max_fit_file->template AddFun<size_t>(
    [this]() {
      return GetOrg(max_fit_org_id).GetGenome().GetProgram().GetSize();
    },
    "num_modules"
  );
  // -- num instructions --
  max_fit_file->template AddFun<size_t>(
    [this]() {
      return GetOrg(max_fit_org_id).GetGenome().GetProgram().GetInstCount();
    },
    "num_instructions"
  );
  // -- program --
  if (OUTPUT_PROGRAMS) {
    max_fit_file->template AddFun<std::string>(
      [this]() {
        std::ostringstream stream;
        stream << "\"";
        PrintProgramSingleLine(GetOrg(max_fit_org_id).GetGenome().GetProgram(), stream);
        stream << "\"";
        return stream.str();
      },
      "program"
    );
  }
  max_fit_file->PrintHeaderKeys();
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

emp::vector<BoolCalcTestInfo::TestCase> BoolCalcWorld::LoadTestCases(const std::string & path) {
  std::ifstream tests_fstream(path);
  if (!tests_fstream.is_open()) {
    std::cout << "Failed to open test case file (" << path << "). Exiting..." << std::endl;
    exit(-1);
  }
  std::string cur_line;
  emp::vector<std::string> line_components;
  emp::vector<std::string> input_components;
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
  while (!tests_fstream.eof()) {
    std::getline(tests_fstream, cur_line);
    if (cur_line == emp::empty_string()) continue; // skip blank lines
    emp::left_justify(cur_line);       // Remove leading whitespace
    emp::right_justify(cur_line);      // Remove trailing whitespace
    line_components.clear();
    emp::slice(cur_line, line_components, ',');
    test_case_t testcase;
    // (1) Grab, process input
    testcase.input_str = line_components[header_lu["input"]]; // Input for this test case.
    std::string test_input = testcase.input_str;
    input_components.clear();
    emp::slice(test_input, input_components, ';');
    // for each component of the test case input, generate a testcase signal
    for (const std::string & in_comp : input_components) {
      emp::vector<std::string> signal_components;
      emp::slice(in_comp, signal_components, ':');
      emp_assert(signal_components.size() == 2);
      const std::string sig_type = signal_components[0];
      emp_assert(sig_type == "OP" || sig_type == "NUM", "Unrecognized input signal type.", sig_type);
      const std::string sig_val = signal_components[1];
      if (sig_type == "OP") {
        testcase.test_signals.emplace_back(sig_val, hw_response_type_t::WAIT);
      } else if (sig_type == "NUM") {
        testcase.test_signals.emplace_back(emp::from_string<operand_t>(sig_val), hw_response_type_t::WAIT);
      } else {
        std::cout << "Unrecognized input signal type! Exiting." << std::endl;
        exit(-1);
      }
    }

    // (2) Grab, process output
    testcase.output_str = line_components[header_lu["output"]]; // Overall correct output for this test case.
    if (testcase.output_str == "ERROR") {
      testcase.test_signals.back().correct_response_type = hw_response_type_t::ERROR;
    } else {
      testcase.test_signals.back().correct_response_type = hw_response_type_t::NUMERIC;
      testcase.test_signals.back().numeric_response = emp::from_string<operand_t>(testcase.output_str);
    }

    // update final signal to match appropriate output
    // (3) Grab, process test type
    testcase.type_str = line_components[header_lu["type"]]; // Category of test case

    // for (auto & test_sig : testcase.test_signals) {
    //   test_sig.Print();
    //   std::cout << std::endl;
    // }
    testcases.emplace_back(testcase);
  }

  if (CATEGORICAL_OUTPUT) {
    for (auto & testcase : testcases) {
      output_categories.emplace(testcase.test_signals.back().numeric_response);
    }
  }

  return testcases;
}

void BoolCalcWorld::DoPopulationSnapshot() {
  using pop_t = emp::vector<emp::Ptr<org_t>>;
  const size_t cur_update = GetUpdate();
  emp::ContainerDataFile snapshot_file = emp::MakeContainerDataFile(
    std::function<pop_t()>([this]() { return pop; }),
    OUTPUT_DIR + "/pop_" + emp::to_string(cur_update) + ".csv"
  );
  // -- update --
  snapshot_file.AddVar(
    cur_update,
    "update"
  );
  // -- is solution --
  snapshot_file.AddContainerFun(
    std::function<bool(emp::Ptr<org_t>)>(
      [this](emp::Ptr<org_t> org) {
        return org->GetPhenotype().IsSolution();
      }
    ),
    "is_solution"
  );
  // -- agg fitness --
  snapshot_file.AddContainerFun(
    std::function<double(emp::Ptr<org_t>)>(
      [this](emp::Ptr<org_t> org) {
        return org->GetPhenotype().GetAggregateScore();
      }
    ),
    "aggregate_fitness"
  );
  // -- num passes --
  snapshot_file.AddContainerFun(
    std::function<size_t(emp::Ptr<org_t>)>(
      [this](emp::Ptr<org_t> org) {
        return org->GetPhenotype().num_passes;
      }
    ),
    "num_passes"
  );
  // -- total tests evaluated --
  snapshot_file.AddContainerFun(
    std::function<size_t(emp::Ptr<org_t>)>(
      [this](emp::Ptr<org_t> org) {
        return org->GetPhenotype().test_scores.size();
      }
    ),
    "total_tests"
  );
  // -- scores by test ("[]") --
  snapshot_file.AddContainerFun(
    std::function<std::string(emp::Ptr<org_t>)>(
      [this](emp::Ptr<org_t> org) {
        std::ostringstream stream;
        const phenotype_t & phen = org->GetPhenotype();
        stream << "\"[";
        for (size_t i = 0; i < phen.test_scores.size(); ++i) {
          if (i) stream << ",";
          stream << phen.test_scores[i];
        }
        stream << "]\"";
        return stream.str();
      }
    ),
    "scores_by_test"
  );
  // -- test ids ("[]") --
  snapshot_file.AddContainerFun(
    std::function<std::string(emp::Ptr<org_t>)>(
      [this](emp::Ptr<org_t> org) {
        std::ostringstream stream;
        const phenotype_t & phen = org->GetPhenotype();
        stream << "\"[";
        for (size_t i = 0; i < phen.test_ids.size(); ++i) {
          if (i) stream << ",";
          stream << phen.test_ids[i];
        }
        stream << "]\"";
        return stream.str();
      }
    ),
    "test_ids"
  );
  // -- distribution of test case passes --
  snapshot_file.AddContainerFun(
    std::function<std::string(emp::Ptr<org_t>)>(
      [this](emp::Ptr<org_t> org) {
        const phenotype_t & phen = org->GetPhenotype();
        std::map<std::string, size_t> distribution(GetPassingTestTypeDistribution(phen));
        std::ostringstream stream;
        stream << "\"{";
        bool comma=false;
        for (const auto & freq : distribution) {
          if (comma) { stream << ","; }
          stream << freq.first << ":" << freq.second;
          comma = true;
        }
        stream << "}\"";
        return stream.str();
      }
    ),
    "test_pass_distribution"
  );
  // -- distribution of test case fails --
  snapshot_file.AddContainerFun(
    std::function<std::string(emp::Ptr<org_t>)>(
      [this](emp::Ptr<org_t> org) {
        const phenotype_t & phen = org->GetPhenotype();
        std::map<std::string, size_t> distribution(GetEvalTestTypeDistribution(phen));
        std::ostringstream stream;
        stream << "\"{";
        bool comma=false;
        for (const auto & freq : distribution) {
          if (comma) { stream << ","; }
          stream << freq.first << ":" << freq.second;
          comma = true;
        }
        stream << "}\"";
        return stream.str();
      }
    ),
    "test_eval_distribution"
  );
  // -- num modules --
  snapshot_file.AddContainerFun(
    std::function<size_t(emp::Ptr<org_t>)>(
      [this](emp::Ptr<org_t> org) {
        return org->GetGenome().GetProgram().GetSize();
      }
    ),
    "num_modules"
  );
  // -- num instructions --
  snapshot_file.AddContainerFun(
    std::function<size_t(emp::Ptr<org_t>)>(
      [this](emp::Ptr<org_t> org) {
        return org->GetGenome().GetProgram().GetInstCount();
      }
    ),
    "num_instructions"
  );
  // -- program --
  if (OUTPUT_PROGRAMS) {
    snapshot_file.AddContainerFun(
      std::function<std::string(emp::Ptr<org_t>)>(
        [this](emp::Ptr<org_t> org) {
          std::ostringstream stream;
          stream << "\"";
          PrintProgramSingleLine(org->GetGenome().GetProgram(), stream);
          stream << "\"";
          return stream.str();
        }
      ),
      "program"
    );
  }
  snapshot_file.PrintHeaderKeys();
  snapshot_file.Update();
}

void BoolCalcWorld::DoWorldConfigSnapshot(const config_t & config) {
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
  get_cur_value = []() { return emp::to_string(BoolCalcWorldDefs::TAG_LEN); };
  snapshot_file.Update();
  // INST_TAG_CNT
  get_cur_param = []() { return "INST_TAG_CNT"; };
  get_cur_value = []() { return emp::to_string(BoolCalcWorldDefs::INST_TAG_CNT); };
  snapshot_file.Update();
  // INST_ARG_CNT
  get_cur_param = []() { return "INST_ARG_CNT"; };
  get_cur_value = []() { return emp::to_string(BoolCalcWorldDefs::INST_ARG_CNT); };
  snapshot_file.Update();
  // FUNC_NUM_TAGS
  get_cur_param = []() { return "FUNC_NUM_TAGS"; };
  get_cur_value = []() { return emp::to_string(BoolCalcWorldDefs::FUNC_NUM_TAGS); };
  snapshot_file.Update();
  // input signals
  get_cur_param = []() { return "input_signals"; };
  get_cur_value = [this]() {
    std::ostringstream stream;
    stream << "\"[";
    for (size_t i = 0; i < test_input_signals.size(); ++i) {
      if (i) stream << ",";
      stream << test_input_signals[i];
    }
    stream << "]\"";
    return stream.str();
  };
  snapshot_file.Update();
  get_cur_param = []() { return "input_signal_tags"; };
  get_cur_value = [this]() {
    std::ostringstream stream;
    stream << "\"[";
    for (size_t i = 0; i < test_input_signal_tags.size(); ++i) {
      if (i) stream << ",";
      test_input_signal_tags[i].Print(stream);
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

void BoolCalcWorld::PrintProgramSingleLine(const program_t & prog, std::ostream & out) {
  out << "[";
  for (size_t func_id = 0; func_id < prog.GetSize(); ++func_id) {
    if (func_id) out << ",";
    PrintProgramFunction(prog[func_id], out);
  }
  out << "]";
}

void BoolCalcWorld::PrintProgramFunction(const program_function_t & func, std::ostream & out) {
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

void BoolCalcWorld::PrintProgramInstruction(const inst_t & inst, std::ostream & out) {
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

BoolCalcWorld::HardwareStatePrintInfo BoolCalcWorld::GetHardwareStatePrintInfo(hardware_t & hw) {
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
        std::string cur_inst_name=""; // get the name of the current instruction
        if (hw.IsValidProgramPosition(flow.mp, flow.ip)) {
          emp_assert(hw.GetProgram().IsValidPosition(flow.mp, flow.ip));
          const size_t inst_type_id = hw.GetProgram()[flow.mp][flow.ip].GetID();
          cur_inst_name = inst_lib->GetName(inst_type_id);
        } else {
          cur_inst_name = "NONE";
        }
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
        // inst name
        stream << "inst_name:" << cur_inst_name << ",";
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


#endif