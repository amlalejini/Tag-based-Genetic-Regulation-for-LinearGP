/***
 * Alternating Signal World
 *
 **/

 // TODO - output file showing how world was initialized

#ifndef _ALT_SIGNAL_WORLD_H
#define _ALT_SIGNAL_WORLD_H

// macros courtesy of Matthew Andres Moreno
#define STRINGVIEWIFY(s) std::string_view(IFY(s))
#define STRINGIFY(s) IFY(s)
#define IFY(s) #s

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
#include "AltSignalOrg.h"
#include "AltSignalConfig.h"
#include "mutation_utils.h"

// TODO - use compile args!
namespace AltSignalWorldDefs {
  constexpr size_t TAG_LEN = 64;      // How many bits per tag?
  constexpr size_t INST_TAG_CNT = 1;  // How many tags per instruction?
  constexpr size_t INST_ARG_CNT = 3;  // How many instruction arguments per instruction?
  constexpr size_t FUNC_NUM_TAGS = 1; // How many tags are associated with each function in a program?
  constexpr int INST_MIN_ARG_VAL = 0; // Minimum argument value?
  constexpr int INST_MAX_ARG_VAL = 7; // Maximum argument value?
  // matchbin <VALUE, METRIC, SELECTOR>
  using matchbin_val_t = size_t;                        // Module ID
  // using matchbin_selector_t = emp::RankedSelector<>;    // 0% min threshold
  using matchbin_selector_t = emp::RankedSelector<std::ratio<TAG_LEN+(TAG_LEN/2), TAG_LEN>>;    // 25% min threshold
  // using matchbin_selector_t = emp::RankedSelector<std::ratio<TAG_LEN+(TAG_LEN/2), TAG_LEN>>;    // 50% min threshold

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
  #else
    emp::StreakMetric<TAG_LEN>;
  #endif

  using org_t = AltSignalOrganism<emp::BitSet<TAG_LEN>,int>;
}

/// Custom Event type!
template<size_t W>
struct Event : public emp::signalgp::BaseEvent {
  using tag_t = emp::BitSet<W>;
  tag_t tag;

  Event(size_t _id, tag_t _tag)
    : BaseEvent(_id), tag(_tag) { ; }

  tag_t & GetTag() { return tag; }
  const tag_t & GetTag() const { return tag; }

  void Print(std::ostream & os) const {
    os << "{id:" << GetID() << ",tag:";
    tag.Print(os);
    os << "}";
  }
};

/// Custom hardware component for SignalGP.
struct CustomHardware {
  int response = -1;

  void Reset() {
    response = -1;
  }
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
  using base_event_t = typename hardware_t::event_t;
  using event_t = Event<AltSignalWorldDefs::TAG_LEN>;
  using inst_lib_t = typename hardware_t::inst_lib_t;
  using inst_t = typename hardware_t::inst_t;
  using inst_prop_t = typename hardware_t::InstProperty;
  using program_t = typename hardware_t::program_t;
  using program_function_t = typename program_t::function_t;
  using mutator_t = MutatorLinearFunctionsProgram<hardware_t, tag_t, inst_arg_t>;
  using phenotype_t = typename org_t::AltSignalPhenotype;

  using mut_landscape_t = emp::datastruct::mut_landscape_info<phenotype_t>;
  using systematics_t = emp::Systematics<org_t, typename org_t::genome_t, mut_landscape_t>;
  using taxon_t = typename systematics_t::taxon_t;

  /// State of the environment during an evaluation.
  struct Environment {
    size_t num_states=0;
    size_t cur_state=0;
    tag_t env_signal_tag=tag_t();

    void ResetEnv() { cur_state = 0; }
    void AdvanceEnv() {
      if (++cur_state >= num_states) cur_state = 0;
    }
  };

protected:
  // Default group
  size_t GENERATIONS;
  size_t POP_SIZE;
  bool STOP_ON_SOLUTION;
  // Environment group
  size_t NUM_SIGNAL_RESPONSES;
  size_t NUM_ENV_CYCLES;
  size_t CPU_TIME_PER_ENV_CYCLE;
  // Program group
  bool USE_FUNC_REGULATION;
  bool USE_GLOBAL_MEMORY;
  emp::Range<size_t> FUNC_CNT_RANGE;
  emp::Range<size_t> FUNC_LEN_RANGE;
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

  Environment eval_environment;

  bool setup = false;                       ///< Has this world been setup already?
  emp::Ptr<inst_lib_t> inst_lib;            ///< Manages SignalGP instruction set.
  emp::Ptr<event_lib_t> event_lib;          ///< Manages SignalGP events.
  emp::Ptr<mutator_t> mutator;

  size_t event_id__env_sig;

  emp::Ptr<hardware_t> eval_hardware;       ///< Used to evaluate programs.

  emp::Signal<void(size_t)> after_eval_sig; ///< Triggered after organism (ID given by size_t argument) evaluation
  emp::Signal<void(void)> end_setup_sig;

  emp::Ptr<emp::DataFile> max_fit_file;
  emp::Ptr<systematics_t> sys_ptr; ///< Short cut to correctly-typed systematics manager. Base class will be responsible for memory management.

  // size_t best_org_id=0;

  /// tracking information for max fitness organism in the population.
  struct {
    size_t org_id=0;
  } max_fit_org_tracker;
  bool found_solution = false;

  void InitConfigs(const AltSignalConfig & config);
  void InitInstLib();
  void InitEventLib();
  void InitHardware();
  void InitEnvironment();
  void InitMutator();
  void InitDataCollection();

  void InitPop();
  void InitPop_Random();

  void DoEvaluation();
  void DoSelection();
  void DoUpdate();

  void EvaluateOrg(org_t & org);

  // -- Utilities --
  void DoPopulationSnapshot();
  void PrintProgramSingleLine(const program_t & prog, std::ostream & out);
  void PrintProgramFunction(const program_function_t & func, std::ostream & out);
  void PrintProgramInstruction(const inst_t & inst, std::ostream & out);

public:
  AltSignalWorld() {}
  AltSignalWorld(emp::Random & r) : emp::World<org_t>(r) {}

  ~AltSignalWorld() {
    if (setup) {
      inst_lib.Delete();
      event_lib.Delete();
      eval_hardware.Delete();
      mutator.Delete();
      max_fit_file.Delete();
    }
  }

  /// Setup world!
  void Setup(const AltSignalConfig & config);

  /// Advance world by a single time step (generation).
  void RunStep();

  /// Run world for configured number of generations.
  void Run();
};

// ---- PROTECTED IMPLEMENTATIONS ----

/// Localize configuration parameters (as member variables) from given AltSignalConfig
/// object.
void AltSignalWorld::InitConfigs(const AltSignalConfig & config) {
  // default group
  GENERATIONS = config.GENERATIONS();
  POP_SIZE = config.POP_SIZE();
  STOP_ON_SOLUTION = config.STOP_ON_SOLUTION();
  // environment group
  NUM_SIGNAL_RESPONSES = config.NUM_SIGNAL_RESPONSES();
  NUM_ENV_CYCLES = config.NUM_ENV_CYCLES();
  CPU_TIME_PER_ENV_CYCLE = config.CPU_TIME_PER_ENV_CYCLE();
  // program group
  USE_FUNC_REGULATION = config.USE_FUNC_REGULATION();
  USE_GLOBAL_MEMORY = config.USE_GLOBAL_MEMORY();
  FUNC_CNT_RANGE = {config.MIN_FUNC_CNT(), config.MAX_FUNC_CNT()};
  FUNC_LEN_RANGE = {config.MIN_FUNC_INST_CNT(), config.MAX_FUNC_INST_CNT()};
  // hardware group
  MAX_ACTIVE_THREAD_CNT = config.MAX_ACTIVE_THREAD_CNT();
  MAX_THREAD_CAPACITY = config.MAX_THREAD_CAPACITY();
  // selection group
  TOURNAMENT_SIZE = config.TOURNAMENT_SIZE();
  // mutation group
  MUT_RATE__INST_ARG_SUB = config.MUT_RATE__INST_ARG_SUB();
  MUT_RATE__INST_TAG_BF = config.MUT_RATE__INST_TAG_BF();
  MUT_RATE__INST_SUB = config.MUT_RATE__INST_SUB();
  MUT_RATE__INST_INS = config.MUT_RATE__INST_INS();
  MUT_RATE__INST_DEL = config.MUT_RATE__INST_DEL();
  MUT_RATE__SEQ_SLIP = config.MUT_RATE__SEQ_SLIP();
  MUT_RATE__FUNC_DUP = config.MUT_RATE__FUNC_DUP();
  MUT_RATE__FUNC_DEL = config.MUT_RATE__FUNC_DEL();
  MUT_RATE__FUNC_TAG_BF = config.MUT_RATE__FUNC_TAG_BF();
  // data collection group
  OUTPUT_DIR = config.OUTPUT_DIR();
  SUMMARY_RESOLUTION = config.SUMMARY_RESOLUTION();
  SNAPSHOT_RESOLUTION = config.SNAPSHOT_RESOLUTION();
}

/// Initialize hardware object.
void AltSignalWorld::InitHardware() {
  // If being configured for the first time, create a new hardware object.
  if (!setup) {
    eval_hardware = emp::NewPtr<hardware_t>(*random_ptr, inst_lib, event_lib);
  }
  // Configure SignalGP CPU
  eval_hardware->Reset();
  eval_hardware->SetActiveThreadLimit(MAX_ACTIVE_THREAD_CNT);
  eval_hardware->SetThreadCapacity(MAX_THREAD_CAPACITY);
  emp_assert(eval_hardware->ValidateThreadState());
}

/// Initialize the environment.
void AltSignalWorld::InitEnvironment() {
  eval_environment.num_states = NUM_SIGNAL_RESPONSES;
  eval_environment.env_signal_tag = emp::BitSet<AltSignalWorldDefs::TAG_LEN>(*random_ptr, 0.5);
  eval_environment.ResetEnv();
  std::cout << "--- ENVIRONMENT SETUP ---" << std::endl;
  std::cout << "Environment tag = ";
  eval_environment.env_signal_tag.Print();
  std::cout << std::endl;
}

/// Create and initialize instruction set with default instructions.
void AltSignalWorld::InitInstLib() {
  if (!setup) inst_lib = emp::NewPtr<inst_lib_t>();
  inst_lib->Clear(); // Reset the instruction library
  inst_lib->AddInst("Nop", [](hardware_t & hw, const inst_t & inst) { ; }, "No operation!");
  inst_lib->AddInst("Inc", emp::signalgp::inst_impl::Inst_Inc<hardware_t, inst_t>, "Increment!");
  inst_lib->AddInst("Dec", emp::signalgp::inst_impl::Inst_Dec<hardware_t, inst_t>, "Decrement!");
  inst_lib->AddInst("Not", emp::signalgp::inst_impl::Inst_Not<hardware_t, inst_t>, "Logical not of ARG[0]");
  inst_lib->AddInst("Add", emp::signalgp::inst_impl::Inst_Add<hardware_t, inst_t>, "");
  inst_lib->AddInst("Sub", emp::signalgp::inst_impl::Inst_Sub<hardware_t, inst_t>, "");
  inst_lib->AddInst("Mult", emp::signalgp::inst_impl::Inst_Mult<hardware_t, inst_t>, "");
  inst_lib->AddInst("Div", emp::signalgp::inst_impl::Inst_Div<hardware_t, inst_t>, "");
  inst_lib->AddInst("Mod", emp::signalgp::inst_impl::Inst_Mod<hardware_t, inst_t>, "");
  inst_lib->AddInst("TestEqu", emp::signalgp::inst_impl::Inst_TestEqu<hardware_t, inst_t>, "");
  inst_lib->AddInst("TestNEqu", emp::signalgp::inst_impl::Inst_TestNEqu<hardware_t, inst_t>, "");
  inst_lib->AddInst("TestLess", emp::signalgp::inst_impl::Inst_TestLess<hardware_t, inst_t>, "");
  inst_lib->AddInst("TestLessEqu", emp::signalgp::inst_impl::Inst_TestLessEqu<hardware_t, inst_t>, "");
  inst_lib->AddInst("TestGreater", emp::signalgp::inst_impl::Inst_TestGreater<hardware_t, inst_t>, "");
  inst_lib->AddInst("TestGreaterEqu", emp::signalgp::inst_impl::Inst_TestGreaterEqu<hardware_t, inst_t>, "");
  inst_lib->AddInst("SetMem", emp::signalgp::inst_impl::Inst_SetMem<hardware_t, inst_t>, "");
  inst_lib->AddInst("Close", emp::signalgp::inst_impl::Inst_Close<hardware_t, inst_t>, "", {inst_prop_t::BLOCK_CLOSE});
  inst_lib->AddInst("Break", emp::signalgp::inst_impl::Inst_Break<hardware_t, inst_t>, "");
  inst_lib->AddInst("Call", emp::signalgp::inst_impl::Inst_Call<hardware_t, inst_t>, "");
  inst_lib->AddInst("Return", emp::signalgp::inst_impl::Inst_Return<hardware_t, inst_t>, "");
  inst_lib->AddInst("CopyMem", emp::signalgp::inst_impl::Inst_CopyMem<hardware_t, inst_t>, "");
  inst_lib->AddInst("SwapMem", emp::signalgp::inst_impl::Inst_SwapMem<hardware_t, inst_t>, "");
  inst_lib->AddInst("InputToWorking", emp::signalgp::inst_impl::Inst_InputToWorking<hardware_t, inst_t>, "");
  inst_lib->AddInst("WorkingToOutput", emp::signalgp::inst_impl::Inst_WorkingToOutput<hardware_t, inst_t>, "");
  inst_lib->AddInst("Fork", emp::signalgp::inst_impl::Inst_Fork<hardware_t, inst_t>, "");
  inst_lib->AddInst("Terminate", emp::signalgp::inst_impl::Inst_Terminate<hardware_t, inst_t>, "");
  inst_lib->AddInst("If", emp::signalgp::lfp_inst_impl::Inst_If<hardware_t, inst_t>, "", {inst_prop_t::BLOCK_DEF});
  inst_lib->AddInst("While", emp::signalgp::lfp_inst_impl::Inst_While<hardware_t, inst_t>, "", {inst_prop_t::BLOCK_DEF});
  inst_lib->AddInst("Countdown", emp::signalgp::lfp_inst_impl::Inst_Countdown<hardware_t, inst_t>, "", {inst_prop_t::BLOCK_DEF});
  inst_lib->AddInst("Routine", emp::signalgp::lfp_inst_impl::Inst_Routine<hardware_t, inst_t>, "");
  inst_lib->AddInst("Terminal", emp::signalgp::inst_impl::Inst_Terminal<hardware_t, inst_t>, "");

  // If we can use global memory, give programs access. Otherwise, nops.
  if (USE_GLOBAL_MEMORY) {
    inst_lib->AddInst("WorkingToGlobal", emp::signalgp::inst_impl::Inst_WorkingToGlobal<hardware_t, inst_t>, "");
    inst_lib->AddInst("GlobalToWorking", emp::signalgp::inst_impl::Inst_GlobalToWorking<hardware_t, inst_t>, "");
  } else {
    inst_lib->AddInst("Nop-WorkingToGlobal", emp::signalgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-GlobalToWorking", emp::signalgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
  }

  // if (allow regulation)
  // If we can use regulation, add instructions. Otherwise, nops.
  if (USE_FUNC_REGULATION) {
    inst_lib->AddInst("SetRegulator", emp::signalgp::inst_impl::Inst_SetRegulator<hardware_t, inst_t>, "");
    inst_lib->AddInst("SetOwnRegulator", emp::signalgp::inst_impl::Inst_SetOwnRegulator<hardware_t, inst_t>, "");
    inst_lib->AddInst("AdjRegulator", emp::signalgp::inst_impl::Inst_AdjRegulator<hardware_t, inst_t>, "");
    inst_lib->AddInst("AdjOwnRegulator", emp::signalgp::inst_impl::Inst_AdjOwnRegulator<hardware_t, inst_t>, "");
    inst_lib->AddInst("ExtRegulator", emp::signalgp::inst_impl::Inst_ExtRegulator<hardware_t, inst_t>, "");
    inst_lib->AddInst("SenseRegulator", emp::signalgp::inst_impl::Inst_SenseRegulator<hardware_t, inst_t>, "");
    inst_lib->AddInst("SenseOwnRegulator", emp::signalgp::inst_impl::Inst_SenseOwnRegulator<hardware_t, inst_t>, "");
  } else {
    inst_lib->AddInst("Nop-SetRegulator", emp::signalgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-SetOwnRegulator", emp::signalgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-AdjRegulator", emp::signalgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-AdjOwnRegulator", emp::signalgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-ExtRegulator", emp::signalgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-SenseRegulator", emp::signalgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-SenseOwnRegulator", emp::signalgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
  }

  // Add response instructions
  for (size_t i = 0; i < NUM_SIGNAL_RESPONSES; ++i) {
    inst_lib->AddInst("Response-" + emp::to_string(i), [this, i](hardware_t & hw, const inst_t & inst) {
      // Mark response in hardware.
      hw.GetCustomComponent().response = i;
      // Remove all pending threads.
      hw.RemoveAllPendingThreads();
      // Mark all active threads as dead.
      for (size_t thread_id : hw.GetActiveThreadIDs()) {
        hw.GetThread(thread_id).SetDead();
      }
      // emp_assert(hw.ValidateThreadState());
    }, "Set organism response to environment.");
  }
}

/// Create and initialize event library.
void AltSignalWorld::InitEventLib() {
  if (!setup) event_lib = emp::NewPtr<event_lib_t>();
  event_lib->Clear();
  // Setup event: EnvSignal
  // Args: name, handler_fun, dispatchers, desc
  event_id__env_sig = event_lib->AddEvent("EnvironmentSignal",
                                          [this](hardware_t & hw, const base_event_t & e) {
                                            const event_t & event = static_cast<const event_t&>(e);
                                            emp_assert(eval_environment.env_signal_tag == event.GetTag());
                                            hw.SpawnThreadWithTag(event.GetTag());
                                          });
}

void AltSignalWorld::InitMutator() {
  if (!setup) { mutator = emp::NewPtr<mutator_t>(*inst_lib); }
  mutator->ResetLastMutationTracker();
  // Set program constraints
  mutator->SetProgFunctionCntRange(FUNC_CNT_RANGE);
  mutator->SetProgFunctionInstCntRange(FUNC_LEN_RANGE);
  mutator->SetProgInstArgValueRange({AltSignalWorldDefs::INST_MIN_ARG_VAL, AltSignalWorldDefs::INST_MAX_ARG_VAL});
  mutator->SetTotalInstLimit(2*FUNC_LEN_RANGE.GetUpper()*FUNC_CNT_RANGE.GetUpper());
  mutator->SetFuncNumTags(AltSignalWorldDefs::FUNC_NUM_TAGS);
  mutator->SetInstNumTags(AltSignalWorldDefs::INST_TAG_CNT);
  mutator->SetInstNumArgs(AltSignalWorldDefs::INST_ARG_CNT);
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

void AltSignalWorld::InitPop() {
  this->Clear();
  InitPop_Random();
}

/// Initialize population with randomly generated orgranisms.
void AltSignalWorld::InitPop_Random() {
  for (size_t i = 0; i < POP_SIZE; ++i) {
    this->Inject({emp::signalgp::GenRandLinearFunctionsProgram<hardware_t, AltSignalWorldDefs::TAG_LEN>
                                  (*random_ptr, *inst_lib,
                                   FUNC_CNT_RANGE,
                                   AltSignalWorldDefs::FUNC_NUM_TAGS,
                                   FUNC_LEN_RANGE,
                                   AltSignalWorldDefs::INST_TAG_CNT,
                                   AltSignalWorldDefs::INST_ARG_CNT,
                                   {AltSignalWorldDefs::INST_MIN_ARG_VAL,AltSignalWorldDefs::INST_MAX_ARG_VAL})
                  }, 1);
  }
}

/// Initialize data collection.
void AltSignalWorld::InitDataCollection() {
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


  // Fitness file
  // Max fitness file (solution should be last line(?))
  // Phylogeny file
  //  - mutation distribution from parent
  // Population snapshot

  // --- Fitness File ---
  SetupFitnessFile(OUTPUT_DIR + "/fitness.csv").SetTimingRepeat(SUMMARY_RESOLUTION);

  // --- Systematics tracking ---
  sys_ptr = emp::NewPtr<systematics_t>([](const org_t & o) { return o.GetGenome(); });
  // We want to record phenotype information AFTER organism is evaluated.
  // - for this, we need to find the appropriate taxon post-evaluation
  after_eval_sig.AddAction([this](size_t pop_id) {
    emp::Ptr<taxon_t> taxon = sys_ptr->GetTaxonAt(pop_id);
    taxon->GetData().RecordFitness(this->CalcFitnessID(pop_id));
    taxon->GetData().RecordPhenotype(this->GetOrg(pop_id).GetPhenotype());
  });
  // We want to record mutations when organism is added to the population
  // - because mutations are applied automatically by this->DoBirth => this->AddOrgAt => sys->OnNew
  std::function<void(emp::Ptr<taxon_t>, org_t&)> record_taxon_mut_data =
    [this](emp::Ptr<taxon_t> taxon, org_t & org) {
      taxon->GetData().RecordMutation(org.GetMutations());
    };
  sys_ptr->OnNew(record_taxon_mut_data);
  // Add snapshot functions
  // - fitness information (taxon->GetFitness)
  // - phenotype information
  //   - res collected, correct resp, no resp
  // - mutations (counts by type)
  sys_ptr->AddSnapshotFun([](const taxon_t & taxon) {
    return emp::to_string(taxon.GetData().GetFitness());
  }, "fitness", "Taxon fitness");
  sys_ptr->AddSnapshotFun([this](const taxon_t & taxon) -> std::string {
    const bool is_sol = taxon.GetData().GetPhenotype().GetCorrectResponses() == NUM_ENV_CYCLES;
    return (is_sol) ? "1" : "0";
  }, "is_solution", "Is this a solution?");
  sys_ptr->AddSnapshotFun([](const taxon_t & taxon) {
    return emp::to_string(taxon.GetData().GetPhenotype().GetResources());
  }, "resources_collected", "How many resources did most recent member of taxon collect?");
  sys_ptr->AddSnapshotFun([](const taxon_t & taxon) {
    return emp::to_string(taxon.GetData().GetPhenotype().GetCorrectResponses());
  }, "correct_signal_responses", "How many correct responses did most recent member of taxon give?");
  sys_ptr->AddSnapshotFun([](const taxon_t & taxon) {
    return emp::to_string(taxon.GetData().GetPhenotype().GetNoResponses());
  }, "no_signal_responses", "How many correct responses did most recent member of taxon give?");
  sys_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
    if (taxon.GetData().HasMutationType("inst_arg_sub")) {
      return emp::to_string(taxon.GetData().GetMutationCount("inst_arg_sub"));
    } else {
      return "0";
    }
  }, "inst_arg_sub_mut_cnt", "How many mutations from parent taxon to this taxon?");
  sys_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("inst_tag_bit_flip")) {
        return emp::to_string(taxon.GetData().GetMutationCount("inst_tag_bit_flip"));
      } else {
        return "0";
      }
    }, "inst_tag_bit_flip_mut_cnt", "Mutation count");
  sys_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("inst_sub")) {
        return emp::to_string(taxon.GetData().GetMutationCount("inst_sub"));
      } else {
        return "0";
      }
    }, "inst_sub_mut_cnt", "Mutation count");
  sys_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("inst_ins")) {
        return emp::to_string(taxon.GetData().GetMutationCount("inst_ins"));
      } else {
        return "0";
      }
    }, "inst_ins_mut_cnt", "Mutation count");
  sys_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("inst_del")) {
        return emp::to_string(taxon.GetData().GetMutationCount("inst_del"));
      } else {
        return "0";
      }
    }, "inst_del_mut_cnt", "Mutation count");
  sys_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("seq_slip_dup")) {
        return emp::to_string(taxon.GetData().GetMutationCount("seq_slip_dup"));
      } else {
        return "0";
      }
    }, "seq_slip_dup_mut_cnt", "Mutation count");
  sys_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("seq_slip_del")) {
        return emp::to_string(taxon.GetData().GetMutationCount("seq_slip_del"));
      } else {
        return "0";
      }
    }, "seq_slip_del_mut_cnt", "Mutation count");
  sys_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("func_dup")) {
        return emp::to_string(taxon.GetData().GetMutationCount("func_dup"));
      } else {
        return "0";
      }
    }, "func_dup_mut_cnt", "Mutation count");
  sys_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("func_del")) {
        return emp::to_string(taxon.GetData().GetMutationCount("func_del"));
      } else {
        return "0";
      }
    }, "func_del_mut_cnt", "Mutation count");
  sys_ptr->AddSnapshotFun([](const taxon_t & taxon) -> std::string {
      if (taxon.GetData().HasMutationType("func_tag_bit_flip")) {
        return emp::to_string(taxon.GetData().GetMutationCount("func_tag_bit_flip"));
      } else {
        return "0";
      }
    }, "func_tag_bit_flip_mut_cnt", "Mutation count");
  sys_ptr->AddSnapshotFun([this](const taxon_t & taxon) {
    std::ostringstream stream;
    stream << "\"";
    this->PrintProgramSingleLine(taxon.GetInfo().GetProgram(), stream);
    stream << "\"";
    return stream.str();
  }, "program", "Program representing this taxon");
  AddSystematics(sys_ptr);
  SetupSystematicsFile(0, OUTPUT_DIR + "/systematics.csv").SetTimingRepeat(SUMMARY_RESOLUTION);

  // --- Dominant File ---
  max_fit_file = emp::NewPtr<emp::DataFile>(OUTPUT_DIR + "/max_fit_org.csv");
  max_fit_file->AddFun(get_update, "update");
  max_fit_file->template AddFun<size_t>([this]() { return max_fit_org_tracker.org_id; }, "pop_id");
  // max_fit_file->AddFun(, "genotype_id");
  max_fit_file->template AddFun<bool>([this]() {
    org_t & org = this->GetOrg(max_fit_org_tracker.org_id);
    return org.GetPhenotype().GetCorrectResponses() == NUM_ENV_CYCLES;
  }, "solution");
  max_fit_file->template AddFun<double>([this]() {
    return this->GetOrg(max_fit_org_tracker.org_id).GetPhenotype().GetResources();
  }, "score");
  max_fit_file->template AddFun<size_t>([this]() {
    return this->GetOrg(max_fit_org_tracker.org_id).GetPhenotype().GetCorrectResponses();
  }, "num_correct_responses");
  max_fit_file->template AddFun<size_t>([this]() {
    return this->GetOrg(max_fit_org_tracker.org_id).GetPhenotype().GetNoResponses();
  }, "num_no_responses");
  max_fit_file->template AddFun<size_t>([this]() {
    return this->GetOrg(max_fit_org_tracker.org_id).GetGenome().GetProgram().GetSize();
  }, "num_modules");
  max_fit_file->template AddFun<size_t>([this]() {
    return this->GetOrg(max_fit_org_tracker.org_id).GetGenome().GetProgram().GetInstCount();
  }, "num_instructions");
  max_fit_file->template AddFun<std::string>([this]() {
    std::ostringstream stream;
    stream << "\"";
    this->PrintProgramSingleLine(this->GetOrg(max_fit_org_tracker.org_id).GetGenome().GetProgram(), stream);
    stream << "\"";
    return stream.str();
  }, "program");
  max_fit_file->PrintHeaderKeys();

}

/// Evaluate entire population.
void AltSignalWorld::DoEvaluation() {
  max_fit_org_tracker.org_id = 0;
  for (size_t org_id = 0; org_id < this->GetSize(); ++org_id) {
    emp_assert(this->IsOccupied(org_id));
    EvaluateOrg(this->GetOrg(org_id));
    if (CalcFitnessID(org_id) > CalcFitnessID(max_fit_org_tracker.org_id)) max_fit_org_tracker.org_id = org_id;
    // Record phenotype information for this taxon
    after_eval_sig.Trigger(org_id);
  }
}

void AltSignalWorld::DoSelection() {
  // Keeping it simple with tournament selection!
  emp::TournamentSelect(*this, TOURNAMENT_SIZE, POP_SIZE);
}

void AltSignalWorld::DoUpdate() {
  // Log current update, Best fitness
  const double max_fit = CalcFitnessID(max_fit_org_tracker.org_id);
  found_solution = GetOrg(max_fit_org_tracker.org_id).GetPhenotype().correct_resp_cnt == NUM_ENV_CYCLES;
  std::cout << "update: " << GetUpdate() << "; ";
  std::cout << "best score (" << max_fit_org_tracker.org_id << "): " << max_fit << "; ";
  std::cout << "solution found: " << found_solution << std::endl;
  const size_t cur_update = GetUpdate();
  if (SUMMARY_RESOLUTION) {
    if (!(cur_update % SUMMARY_RESOLUTION) || cur_update == GENERATIONS || (STOP_ON_SOLUTION & found_solution)) max_fit_file->Update();
  }

  if (SNAPSHOT_RESOLUTION) {
    if (!(cur_update % SNAPSHOT_RESOLUTION) || cur_update == GENERATIONS || (STOP_ON_SOLUTION & found_solution)) {
      DoPopulationSnapshot();
      if (cur_update) sys_ptr->Snapshot(OUTPUT_DIR + "/phylo_" + emp::to_string(cur_update) + ".csv");
    }
  }

  Update();
  ClearCache();
}

/// Evaluate a single organism.
void AltSignalWorld::EvaluateOrg(org_t & org) {
  eval_environment.ResetEnv();
  org.GetPhenotype().Reset();
  // Ready the hardware! Load organism program, reset the custom hardware component.
  // eval_hardware->Reset();
  eval_hardware->SetProgram(org.GetGenome().program);
  emp_assert(eval_hardware->ValidateThreadState());
  emp_assert(eval_hardware->GetActiveThreadIDs().size() == 0);
  // Evaluate organism in the environment!
  for (size_t cycle = 0; cycle < NUM_ENV_CYCLES; ++cycle) {
    eval_hardware->BaseResetState(); // Reset threads every cycle.
    eval_hardware->GetCustomComponent().Reset();
    emp_assert(eval_hardware->GetActiveThreadIDs().size() == 0);
    eval_hardware->QueueEvent(event_t(event_id__env_sig, eval_environment.env_signal_tag));
    // Step hardware! If at any point there are no active || pending threads, we're done!
    for (size_t step = 0; step < CPU_TIME_PER_ENV_CYCLE; ++step) {
      eval_hardware->SingleProcess();
      if (!(eval_hardware->GetNumActiveThreads() || eval_hardware->GetNumPendingThreads())) break;
    }
    // Did hardware consume the resource?
    const int org_response = eval_hardware->GetCustomComponent().response;
    if (org_response == (int)eval_environment.cur_state) {
      // Correct response!
      org.GetPhenotype().resources_consumed += 1;
      org.GetPhenotype().correct_resp_cnt += 1;
    } else if (org_response == -1) {
      // No response!
      org.GetPhenotype().no_resp_cnt += 1;
    } else {
      // Incorrect response, end evaluation.
      break;
    }
    eval_environment.AdvanceEnv();
  }
  // std::cout << "Final - resources: "<< org.GetPhenotype().resources_consumed <<" ; correct: "<< org.GetPhenotype().correct_resp_cnt << " ; no resp: " << org.GetPhenotype().no_resp_cnt  << std::endl;
}

// -- utilities --

void AltSignalWorld::DoPopulationSnapshot() {
  // Make a new data file for snapshot.
  emp::DataFile snapshot_file(OUTPUT_DIR + "/pop_" + emp::to_string((int)GetUpdate()) + ".csv");
  size_t cur_org_id = 0;
  // Add functions.
  snapshot_file.AddFun<size_t>([this]() { return this->GetUpdate(); }, "update");
  snapshot_file.AddFun<size_t>([this, &cur_org_id]() { return cur_org_id; }, "pop_id");
  // max_fit_file->AddFun(, "genotype_id");
  snapshot_file.AddFun<bool>([this, &cur_org_id]() {
    org_t & org = this->GetOrg(cur_org_id);
    return org.GetPhenotype().GetCorrectResponses() == NUM_ENV_CYCLES;
  }, "solution");
  snapshot_file.template AddFun<double>([this, &cur_org_id]() {
    return this->GetOrg(cur_org_id).GetPhenotype().GetResources();
  }, "score");
  snapshot_file.template AddFun<size_t>([this, &cur_org_id]() {
    return this->GetOrg(cur_org_id).GetPhenotype().GetCorrectResponses();
  }, "num_correct_responses");
  snapshot_file.template AddFun<size_t>([this, &cur_org_id]() {
    return this->GetOrg(cur_org_id).GetPhenotype().GetNoResponses();
  }, "num_no_responses");
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

void AltSignalWorld::PrintProgramSingleLine(const program_t & prog, std::ostream & out) {
  out << "[";
  for (size_t func_id = 0; func_id < prog.GetSize(); ++func_id) {
    if (func_id) out << ",";
    PrintProgramFunction(prog[func_id], out);
  }
  out << "]";
}

void AltSignalWorld::PrintProgramFunction(const program_function_t & func, std::ostream & out) {
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

void AltSignalWorld::PrintProgramInstruction(const inst_t & inst, std::ostream & out) {
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

// ---- PUBLIC IMPLEMENTATIONS ----

void AltSignalWorld::Setup(const AltSignalConfig & config) {
  // Localize configuration parameters.
  InitConfigs(config);

  // Create instruction/event libraries.
  InitInstLib();
  InitEventLib();
  // Init evaluation hardware
  InitHardware();
  // Print how matchbin is actually configured.
  #ifdef MATCH_METRIC
  // Print matchbin metric
  std::cout << "Requested MatchBin Metric: " << STRINGVIEWIFY(MATCH_METRIC) << std::endl;
  #endif
  std::cout << "Configured MatchBin: " << eval_hardware->GetMatchBin().name() << std::endl;

  // Init evaluation environment
  InitEnvironment();
  // Initialize organism mutators!
  InitMutator();

  // How should population be initialized?
  end_setup_sig.AddAction([this]() {
    std::cout << "Initializing population...";
    InitPop();
    std::cout << " Done" << std::endl;
    this->SetAutoMutate(); // Set to automutate after initializing population!
  });

  this->SetPopStruct_Mixed(true); // Population is well-mixed with synchronous generations.
  this->SetFitFun([this](org_t & org) {
    return org.GetPhenotype().resources_consumed;
  });

  InitDataCollection();

  end_setup_sig.Trigger();
  setup = true;
}

void AltSignalWorld::Run() {
  for (size_t u = 0; u <= GENERATIONS; ++u) {
    RunStep();
    if (STOP_ON_SOLUTION & found_solution) break;
  }
}

void AltSignalWorld::RunStep() {
  // (1) evaluate pop, (2) select parents, (3) update world
  DoEvaluation();
  DoSelection();
  DoUpdate();
}

#endif