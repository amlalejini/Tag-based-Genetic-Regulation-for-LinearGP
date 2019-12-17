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
  using config_t = MCRegConfig;

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
  // Environment group
  size_t NUM_RESPONSE_TYPES;
  size_t DEVELOPMENT_PHASE_CPU_TIME;
  size_t RESPONSE_PHASE_CPU_TIME;
  // Program group
  bool USE_FUNC_REGULATION;
  bool USE_GLOBAL_MEMORY;
  emp::Range<size_t> FUNC_CNT_RANGE;
  emp::Range<size_t> FUNC_LEN_RANGE;
  // Hardware group
  size_t DEME_WIDTH;
  size_t DEME_HEIGHT;
  size_t PROPAGULE_SIZE;
  std::string PROPAGULE_LAYOUT;
  size_t MAX_ACTIVE_THREAD_CNT;
  size_t MAX_THREAD_CAPACITY;
  bool EPIGENETIC_INHERITANCE;
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
  // Output group
  std::string OUTPUT_DIR;
  size_t SUMMARY_RESOLUTION;
  size_t SNAPSHOT_RESOLUTION;

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

  bool found_solution = false;              ///< Have we stumbled onto a 'solution' yet?
  double MAX_RESPONSE_SCORE=0;              ///< What is the maximum score an organism can achieve?
  size_t max_fit_org_id=0;                  ///< What is the most fit organism ID for this generation?
  bool CLUMPY_PROPAGULES=false;

  size_t event_id__propagule_sig=0;
  size_t event_id__response_sig=0;
  size_t event_id__birth_sig=0;

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

  // -- utilities --
  void DoPopulationSnapshot();
  void DoWorldConfigSnapshot(const config_t & config);

public:
  MCRegWorld(emp::Random & r) : emp::World<org_t>(r) {}

  ~MCRegWorld() {
    if (setup) {
      inst_lib.Delete();
      event_lib.Delete();
      eval_deme.Delete();
      mutator.Delete();
    }
  }

  /// Setup world!
  void Setup(const config_t & config);

  /// Advance the world by a single time step (generation)
  void RunStep();

  /// Run world for configured number of generations
  void Run();

  // -- printing utilities --
  void PrintProgramSingleLine(const program_t & prog, std::ostream & out);
  void PrintProgramFunction(const program_function_t & func, std::ostream & out);
  void PrintProgramInstruction(const inst_t & inst, std::ostream & out);
};

// ----- Utilities -----

/// Evaluate a single organism.
void MCRegWorld::EvaluateOrg(org_t & org) {
  // std::cout << "==== EVALUATE ORG ====" << std::endl;
  // Reset the environment.
  eval_environment.ResetEnv();
  // Set environment to development phase
  eval_environment.SetPhase(ENV_STATE::DEVELOPMENT);
  // Reset organism's phenotype.
  org.GetPhenotype().Reset();
  // Ready the evaluation hardware (deme)
  eval_deme->ActivatePropagule(org.GetGenome().program, PROPAGULE_SIZE, CLUMPY_PROPAGULES);
  // eval_deme->PrintActive();
  // Evaluate developmental phase
  //  - Note: propagule cells have already received a 'start' event for the development phase
  for (size_t step = 0; step < DEVELOPMENT_PHASE_CPU_TIME; ++step) {
    if (!eval_deme->SingleAdvance()) break;
  }
  // TODO - do we want to kill all cell activity before the response phase?
  // Evaluate response phase
  eval_environment.SetPhase(ENV_STATE::RESPONSE);
  // - Queue environment response phase event on all active cells
  for (hardware_t & cell_hw : eval_deme->GetCells()) {
    if (!cell_hw.GetCustomComponent().IsActive()) continue;
    // Remove all pending threads.
    cell_hw.RemoveAllPendingThreads();
    // Mark all active threads as dead.
    for (size_t thread_id : cell_hw.GetActiveThreadIDs()) {
      cell_hw.GetThread(thread_id).SetDead();
    }
    // Clear event queue!
    cell_hw.ClearEventQueue();
    cell_hw.SingleProcess(); // Execute to clean up dead threads
    emp_assert(cell_hw.GetNumActiveThreads() == 0 || cell_hw.GetNumPendingThreads() == 0 || cell_hw.GetNumQueuedEvents() == 0);
    cell_hw.QueueEvent(event_t(event_id__response_sig, eval_environment.response_signal_tag));
  }
  for (size_t step = 0; step < RESPONSE_PHASE_CPU_TIME; ++step) {
    if (!eval_deme->SingleAdvance()) break;
  }
  // Update org's phenotype
  std::unordered_set<size_t> unique_responses;
  for (hardware_t & cell_hw : eval_deme->GetCells()) {
    if (!cell_hw.GetCustomComponent().IsActive()) continue;
    org.GetPhenotype().num_active_cells += 1;
    if (cell_hw.GetCustomComponent().response == -1) continue;
    org.GetPhenotype().num_resp += 1;
    unique_responses.emplace(cell_hw.GetCustomComponent().GetResponse());
  }
  org.GetPhenotype().num_unique_resp = unique_responses.size();
  org.GetPhenotype().resources_consumed = org.GetPhenotype().num_unique_resp * org.GetPhenotype().num_resp;
  // eval_deme->PrintResponses();
}

void MCRegWorld::DoEvaluation() {
  max_fit_org_id = 0;
  for (size_t org_id = 0; org_id < this->GetSize(); ++org_id) {
    emp_assert(this->IsOccupied(org_id));
    EvaluateOrg(this->GetOrg(org_id)); // Each organism can be evaluated independently of other organisms
    if (CalcFitnessID(org_id) > CalcFitnessID(max_fit_org_id)) max_fit_org_id = org_id;
    // Record the phenotype information for this taxon.
    after_eval_sig.Trigger(org_id);
  }
}

void MCRegWorld::DoSelection() {
  // Keeping it simple with tournament selection!
  emp::TournamentSelect(*this, TOURNAMENT_SIZE, POP_SIZE);
}

void MCRegWorld::DoUpdate() {
  // Log current update, best fitness found this generation
  const double max_fit = CalcFitnessID(max_fit_org_id);
  found_solution = GetOrg(max_fit_org_id).GetPhenotype().GetResourcesConsumed() == MAX_RESPONSE_SCORE;
  std::cout << "update: " << GetUpdate() << "; ";
  std::cout << "best score (" << max_fit_org_id << "): " << max_fit << "; ";
  std::cout << "solution found: " << found_solution << std::endl;
  const size_t cur_update = GetUpdate();
  if (SUMMARY_RESOLUTION) {
    if (!(cur_update % SUMMARY_RESOLUTION) || cur_update == GENERATIONS || (STOP_ON_SOLUTION & found_solution)) max_fit_file->Update();
  }
  if (SNAPSHOT_RESOLUTION) {
    if (!(cur_update % SNAPSHOT_RESOLUTION) || cur_update == GENERATIONS || (STOP_ON_SOLUTION & found_solution)) {
      DoPopulationSnapshot();
      if (cur_update) systematics_ptr->Snapshot(OUTPUT_DIR + "/phylo_" + emp::to_string(cur_update) + ".csv");
    }
  }

  // --- For debugging => eval best org again w/prints ---
  std::cout << "Best Org details:" << std::endl;
  EvaluateOrg(this->GetOrg(max_fit_org_id));
  eval_deme->PrintActive();
  eval_deme->PrintResponses();
  std::cout << "=====================" << std::endl;

  Update();
  ClearCache();
}

void MCRegWorld::InitConfigs(const config_t & config) {
  // default group
  GENERATIONS = config.GENERATIONS();
  POP_SIZE = config.POP_SIZE();
  STOP_ON_SOLUTION = config.STOP_ON_SOLUTION();
  // environment group
  NUM_RESPONSE_TYPES = config.NUM_RESPONSE_TYPES();
  DEVELOPMENT_PHASE_CPU_TIME = config.DEVELOPMENT_PHASE_CPU_TIME();
  RESPONSE_PHASE_CPU_TIME = config.RESPONSE_PHASE_CPU_TIME();
  // program group
  USE_FUNC_REGULATION = config.USE_FUNC_REGULATION();
  USE_GLOBAL_MEMORY = config.USE_GLOBAL_MEMORY();
  FUNC_CNT_RANGE = {config.MIN_FUNC_CNT(), config.MAX_FUNC_CNT()};
  FUNC_LEN_RANGE = {config.MIN_FUNC_INST_CNT(), config.MAX_FUNC_INST_CNT()};
  // hardware group
  DEME_WIDTH = config.DEME_WIDTH();
  DEME_HEIGHT = config.DEME_HEIGHT();
  PROPAGULE_SIZE = config.PROPAGULE_SIZE();
  PROPAGULE_LAYOUT = config.PROPAGULE_LAYOUT();
  MAX_ACTIVE_THREAD_CNT = config.MAX_ACTIVE_THREAD_CNT();
  MAX_THREAD_CAPACITY = config.MAX_THREAD_CAPACITY();
  EPIGENETIC_INHERITANCE = config.EPIGENETIC_INHERITANCE();
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
  // output group
  OUTPUT_DIR = config.OUTPUT_DIR();
  SUMMARY_RESOLUTION = config.SUMMARY_RESOLUTION();
  SNAPSHOT_RESOLUTION = config.SNAPSHOT_RESOLUTION();
}

void MCRegWorld::InitInstLib() {
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
    inst_lib->AddInst("WorkingToGlobal", sgp::inst_impl::Inst_WorkingToGlobal<hardware_t, inst_t>, "");
    inst_lib->AddInst("GlobalToWorking", sgp::inst_impl::Inst_GlobalToWorking<hardware_t, inst_t>, "");
  } else {
    inst_lib->AddInst("Nop-WorkingToGlobal", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
    inst_lib->AddInst("Nop-GlobalToWorking", sgp::inst_impl::Inst_Nop<hardware_t, inst_t>, "");
  }
  // If we can use regulation, add instructions. Otherwise, nops.
  if (USE_FUNC_REGULATION) {
    inst_lib->AddInst("SetRegulator", sgp::inst_impl::Inst_SetRegulator<hardware_t, inst_t>, "");
    inst_lib->AddInst("SetOwnRegulator", sgp::inst_impl::Inst_SetOwnRegulator<hardware_t, inst_t>, "");
    inst_lib->AddInst("AdjRegulator", sgp::inst_impl::Inst_AdjRegulator<hardware_t, inst_t>, "");
    inst_lib->AddInst("AdjOwnRegulator", sgp::inst_impl::Inst_AdjOwnRegulator<hardware_t, inst_t>, "");
    inst_lib->AddInst("SenseRegulator", sgp::inst_impl::Inst_SenseRegulator<hardware_t, inst_t>, "");
    inst_lib->AddInst("SenseOwnRegulator", sgp::inst_impl::Inst_SenseOwnRegulator<hardware_t, inst_t>, "");
    inst_lib->AddInst("IncRegulator", sgp::inst_impl::Inst_IncRegulator<hardware_t, inst_t>, "");
    inst_lib->AddInst("IncOwnRegulator", sgp::inst_impl::Inst_IncOwnRegulator<hardware_t, inst_t>, "");
    inst_lib->AddInst("DecRegulator", sgp::inst_impl::Inst_DecRegulator<hardware_t, inst_t>, "");
    inst_lib->AddInst("DecOwnRegulator", sgp::inst_impl::Inst_DecOwnRegulator<hardware_t, inst_t>, "");
  } else {
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
  // TODO - add deme instructions
  //   - repro
  inst_lib->AddInst("Reproduce", [this](hardware_t & hw, const inst_t & inst) {
    // Reproduction is only allowed in development phase
    if (eval_environment.GetPhase() != ENV_STATE::DEVELOPMENT) return;
    // Get cell_id, facing
    const size_t cell_id = hw.GetCustomComponent().GetCellID();
    const auto cell_facing = hw.GetCustomComponent().GetFacing();
    const size_t facing_id = eval_deme->GetNeighboringCellID(cell_id, cell_facing);
    // If cell in front of us is active, bail out.
    if (eval_deme->IsActive(facing_id)) return;
    // If we're here, this cell is facing an empty cell && we're in the development phase.
    eval_deme->DoReproduction(facing_id, cell_id);
    // If we're here, cells[facing_id] is 'new born', queue repro event!
    eval_deme->GetCell(facing_id).QueueEvent(event_t(event_id__birth_sig, inst.GetTag(0)));
  }, "Executing this instruction triggers reproduction.");
  //   - actuation (rotation)
  inst_lib->AddInst("RotateCW", [this](hardware_t & hw, const inst_t & inst) {
    hw.GetCustomComponent().RotateCW();
  }, "Rotate cell 45 degrees in clockwise direction.");
  inst_lib->AddInst("RotateCCW", [this](hardware_t & hw, const inst_t & inst) {
    hw.GetCustomComponent().RotateCCW();
  }, "Rotate cell 45 degrees in counter-clockwise direction.");
  //   - rot to empty
  inst_lib->AddInst("RotateToEmpty", [this](hardware_t & hw, const inst_t & inst) {
    const deme_t::Facing cur_facing = hw.GetCustomComponent().GetFacing();
    const size_t cell_id = hw.GetCustomComponent().GetCellID();
    for (size_t i = 0; i < deme_t::NUM_DIRECTIONS + 1; ++i) {
      const deme_t::Facing next_dir = deme_t::Dir[((size_t)cur_facing + i) % deme_t::NUM_DIRECTIONS];
      if (!eval_deme->IsActive(eval_deme->GetNeighboringCellID(cell_id, next_dir))) {
        hw.GetCustomComponent().SetFacing(next_dir);
        break;
      }
    }
  }, "Rotate cell to next empty cell (if there is one).");
  //   - sense current dir
  inst_lib->AddInst("GetDir", [](hardware_t & hw, const inst_t & inst) {
    auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
    call_state.GetMemory().SetWorking(inst.GetArg(0), (double)hw.GetCustomComponent().GetFacing());
  }, "WorkingMemory[arg0] = Facing Direction");
  //   - sense facing empty
  inst_lib->AddInst("IsNeighborEmpty", [this](hardware_t & hw, const inst_t & inst) {
    auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
    const size_t cell_id = hw.GetCustomComponent().GetCellID();
    const deme_t::Facing cell_facing = hw.GetCustomComponent().GetFacing();
    const size_t neighbor_id = eval_deme->GetNeighboringCellID(cell_id, cell_facing);
    call_state.GetMemory().SetWorking(inst.GetArg(0), (double)eval_deme->IsActive(neighbor_id));
  }, "WorkingMemory[arg0] = IsActive(faced neighbor)");
  // Add response-phase instructions
  for (size_t i = 0; i < NUM_RESPONSE_TYPES; ++i) {
    inst_lib->AddInst("Response-" + emp::to_string(i), [this, i](hardware_t & hw, const inst_t & inst) {
      // Not allowed to response in development phase!
      if (eval_environment.GetPhase() != ENV_STATE::RESPONSE) return;
      // Mark response on hardware
      hw.GetCustomComponent().SetResponse((int)i);
      // Remove all pending threads.
      hw.RemoveAllPendingThreads();
      // Mark all active threads as dead.
      for (size_t thread_id : hw.GetActiveThreadIDs()) {
        hw.GetThread(thread_id).SetDead();
      }
      // Clear event queue!
      hw.ClearEventQueue();
    }, "Set cell response if in response phase.");
  }
  // TODO - add messaging instructions
}

void MCRegWorld::InitEventLib() {
  if (!setup) event_lib = emp::NewPtr<event_lib_t>();
  event_lib->Clear();
  // Args: name, handler_fun, dispatchers, desc
  // Setup event: Cell Response Signal
  event_id__propagule_sig = event_lib->AddEvent("PropaguleSignal",
                                          [this](hardware_t & hw, const base_event_t & e) {
                                            const event_t & event = static_cast<const event_t&>(e);
                                            emp_assert(eval_environment.propagule_start_tag == event.GetTag());
                                            hw.SpawnThreadWithTag(event.GetTag());
                                          });
  event_id__birth_sig = event_lib->AddEvent("BirthSignal",
                                          [this](hardware_t & hw, const base_event_t & e) {
                                            const event_t & event = static_cast<const event_t&>(e);
                                            hw.SpawnThreadWithTag(event.GetTag());
                                          });
  event_id__response_sig = event_lib->AddEvent("ResponseSignal",
                                          [this](hardware_t & hw, const base_event_t & e) {
                                            const event_t & event = static_cast<const event_t&>(e);
                                            emp_assert(eval_environment.response_signal_tag == event.GetTag());
                                            hw.SpawnThreadWithTag(event.GetTag());
                                          });
  // TODO - setup messaging events

}

void MCRegWorld::InitHardware() {
  // If being configured for the first time, create a new hardware object.
  if (!setup) {
    eval_deme = emp::NewPtr<deme_t>(DEME_WIDTH, DEME_HEIGHT,
                                    *random_ptr, *inst_lib, *event_lib);
    // Configure OnPropaguleActivate
    eval_deme->OnPropaguleActivate([this](hardware_t & hw) {
      // Queue an event!
      hw.QueueEvent(event_t(event_id__propagule_sig, eval_environment.propagule_start_tag));
      hw.GetCustomComponent().SetFacing(deme_t::Facing::N);
    });
    // Configure on reproduction event
    eval_deme->OnReproActivate([this](hardware_t & offspring_hw, hardware_t & parent_hw) {
      // Face parent
      const auto parent_facing = parent_hw.GetCustomComponent().GetFacing();
      const auto face_parent = deme_t::Dir[(parent_facing + (deme_t::NUM_DIRECTIONS / 2)) % deme_t::NUM_DIRECTIONS];
      offspring_hw.GetCustomComponent().SetFacing(face_parent);
      // std::cout << eval_deme->FacingToStr(face_parent) << "=>";
      // std::cout << eval_deme->FacingToStr(parent_facing) << std::endl;
      // Todo - maybe copy regulation over
      if (EPIGENETIC_INHERITANCE) {
         offspring_hw.GetMatchBin().ImprintRegulators(parent_hw.GetMatchBin());
        // TODO - check epigenetic inheritance!
      }
    });
  }
  emp_assert(PROPAGULE_LAYOUT == "random" || PROPAGULE_LAYOUT == "clumpy");
  CLUMPY_PROPAGULES = (PROPAGULE_LAYOUT == "clumpy");
  eval_deme->ResetCells();
  eval_deme->ConfigureCells(MAX_ACTIVE_THREAD_CNT, MAX_THREAD_CAPACITY);
  // todo - any extra configuration we need to do for evaluation!
}

void MCRegWorld::InitEnvironment() {
  // Setup environment signal tags
  emp::vector<tag_t> env_bitsets(emp::RandomBitSets<MCRegWorldDefs::TAG_LEN>(*random_ptr, 2, true));
  emp_assert(env_bitsets.size() == 2);
  eval_environment.propagule_start_tag = env_bitsets[0];
  eval_environment.response_signal_tag = env_bitsets[1];
  eval_environment.ResetEnv();
}

void MCRegWorld::InitMutator() {
  // Setup mutator
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

void MCRegWorld::InitPop() {
  this->Clear();
  InitPop_Random();
}

/// Initialize the population with randomly generated organisms
void MCRegWorld::InitPop_Random() {
  for (size_t i = 0; i < POP_SIZE; ++i) {
    this->Inject({sgp::GenRandLinearFunctionsProgram<hardware_t, MCRegWorldDefs::TAG_LEN>
                                  (*random_ptr, *inst_lib,
                                   FUNC_CNT_RANGE,
                                   MCRegWorldDefs::FUNC_NUM_TAGS,
                                   FUNC_LEN_RANGE,
                                   MCRegWorldDefs::INST_TAG_CNT,
                                   MCRegWorldDefs::INST_ARG_CNT,
                                   {MCRegWorldDefs::INST_MIN_ARG_VAL,MCRegWorldDefs::INST_MAX_ARG_VAL})
                  }, 1);
  }
}

void MCRegWorld::InitDataCollection() {
  emp_assert(!setup);
  if (setup) {
    max_fit_file.Delete();
  } else {
    mkdir(OUTPUT_DIR.c_str(), ACCESSPERMS);
    if(OUTPUT_DIR.back() != '/')
        OUTPUT_DIR += '/';
  }
  std::function<size_t(void)> get_update = [this]() { return this->GetUpdate(); };
  // -- Setup the fitness file --
  SetupFitnessFile(OUTPUT_DIR + "/fitness.csv").SetTimingRepeat(SUMMARY_RESOLUTION);
  // -- Systematics tracking --
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
    const phenotype_t & phen = taxon.GetData().GetPhenotype();
    const bool is_sol = (phen.num_unique_resp * phen.num_resp) == MAX_RESPONSE_SCORE;
    return (is_sol) ? "1" : "0";
  }, "is_solution", "Is this a solution?");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) {
    return emp::to_string(taxon.GetData().GetPhenotype().GetResourcesConsumed());
  }, "resources_collected", "How many resources did most recent member of taxon collect?");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) {
    return emp::to_string(taxon.GetData().GetPhenotype().GetUniqueResponseCnt());
  }, "unique_responses", "How many unique responses did deme exhibit?");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) {
    return emp::to_string(taxon.GetData().GetPhenotype().GetResponseCnt());
  }, "total_responses", "How many total responses did deme produce?");
  systematics_ptr->AddSnapshotFun([](const taxon_t & taxon) {
    return emp::to_string(taxon.GetData().GetPhenotype().GetActiveCellCnt());
  }, "active_cell_cnt", "How many active cells in deme after development?");
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
  // -- Dominant file --
  max_fit_file = emp::NewPtr<emp::DataFile>(OUTPUT_DIR + "/max_fit_org.csv");
  max_fit_file->AddFun(get_update, "update");
  max_fit_file->template AddFun<size_t>([this]() { return max_fit_org_id; }, "pop_id");
  max_fit_file->template AddFun<bool>([this]() {
    const phenotype_t & phen = this->GetOrg(max_fit_org_id).GetPhenotype();
    const bool is_sol = (phen.num_unique_resp * phen.num_resp) == MAX_RESPONSE_SCORE;
    return (is_sol) ? "1" : "0";
  }, "solution");
  max_fit_file->template AddFun<double>([this]() {
    return this->GetOrg(max_fit_org_id).GetPhenotype().GetResourcesConsumed();
  }, "score");
  max_fit_file->template AddFun<size_t>([this]() {
    return this->GetOrg(max_fit_org_id).GetPhenotype().GetUniqueResponseCnt();
  }, "unique_responses");
  max_fit_file->template AddFun<size_t>([this]() {
    return this->GetOrg(max_fit_org_id).GetPhenotype().GetResponseCnt();
  }, "total_responses");
  max_fit_file->template AddFun<size_t>([this]() {
    return this->GetOrg(max_fit_org_id).GetPhenotype().GetActiveCellCnt();
  }, "active_cell_cnt");
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

void MCRegWorld::DoPopulationSnapshot() {
  // Make a new data file for snapshot.
  emp::DataFile snapshot_file(OUTPUT_DIR + "/pop_" + emp::to_string((int)GetUpdate()) + ".csv");
  size_t cur_org_id = 0;
  // Add functions.
  snapshot_file.AddFun<size_t>([this]() { return this->GetUpdate(); }, "update");
  snapshot_file.AddFun<size_t>([this, &cur_org_id]() { return cur_org_id; }, "pop_id");
  // max_fit_file->AddFun(, "genotype_id");
  snapshot_file.AddFun<bool>([this, &cur_org_id]() {
    org_t & org = this->GetOrg(cur_org_id);
    return org.GetPhenotype().GetResourcesConsumed() == MAX_RESPONSE_SCORE;
  }, "solution");
  snapshot_file.template AddFun<double>([this, &cur_org_id]() {
    return this->GetOrg(cur_org_id).GetPhenotype().GetResourcesConsumed();
  }, "score");
  snapshot_file.template AddFun<size_t>([this, &cur_org_id]() {
    return this->GetOrg(cur_org_id).GetPhenotype().GetUniqueResponseCnt();
  }, "unique_responses");
  snapshot_file.template AddFun<size_t>([this, &cur_org_id]() {
    return this->GetOrg(cur_org_id).GetPhenotype().GetResponseCnt();
  }, "total_responses");
  snapshot_file.template AddFun<size_t>([this, &cur_org_id]() {
    return this->GetOrg(cur_org_id).GetPhenotype().GetActiveCellCnt();
  }, "active_cell_cnt");
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

void MCRegWorld::DoWorldConfigSnapshot(const config_t & config) {
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
  get_cur_value = []() { return emp::to_string(MCRegWorldDefs::TAG_LEN); };
  snapshot_file.Update();
  // INST_TAG_CNT
  get_cur_param = []() { return "INST_TAG_CNT"; };
  get_cur_value = []() { return emp::to_string(MCRegWorldDefs::INST_TAG_CNT); };
  snapshot_file.Update();
  // INST_ARG_CNT
  get_cur_param = []() { return "INST_ARG_CNT"; };
  get_cur_value = []() { return emp::to_string(MCRegWorldDefs::INST_ARG_CNT); };
  snapshot_file.Update();
  // FUNC_NUM_TAGS
  get_cur_param = []() { return "FUNC_NUM_TAGS"; };
  get_cur_value = []() { return emp::to_string(MCRegWorldDefs::FUNC_NUM_TAGS); };
  snapshot_file.Update();
  // INST_MIN_ARG_VAL
  get_cur_param = []() { return "INST_MIN_ARG_VAL"; };
  get_cur_value = []() { return emp::to_string(MCRegWorldDefs::INST_MIN_ARG_VAL); };
  snapshot_file.Update();
  // INST_MAX_ARG_VAL
  get_cur_param = []() { return "INST_MAX_ARG_VAL"; };
  get_cur_value = []() { return emp::to_string(MCRegWorldDefs::INST_MAX_ARG_VAL); };
  snapshot_file.Update();
  // environment response phase signal
  get_cur_param = []() { return "environment_response_tag"; };
  get_cur_value = [this]() {
    std::ostringstream stream;
    eval_environment.response_signal_tag.Print(stream);
    return stream.str();
  };
  snapshot_file.Update();
  // propagule signal
  get_cur_param = []() { return "environment_propagule_tag"; };
  get_cur_value = [this]() {
    std::ostringstream stream;
    eval_environment.propagule_start_tag.Print(stream);
    return stream.str();
  };
  snapshot_file.Update();

  // configuration parameters
  for (const auto & entry : config) {
    get_cur_param = [&entry]() { return entry.first; };
    get_cur_value = [&entry]() { return emp::to_string(entry.second->GetValue()); };
    snapshot_file.Update();
  }
}

// ----- PUBLIC IMPLEMENTATIONS -----
void MCRegWorld::Setup(const MCRegConfig & config) {
  // Localize configuration parameters.
  InitConfigs(config);

  // Create instruction/event libraries.
  InitInstLib();
  InitEventLib();
  // Initialize evaluation hardware (eval_deme)
  InitHardware();
  if (eval_deme->GetSize()) {
    std::cout << "Configured hardware matchbin: " << eval_deme->GetCell(0).GetMatchBin().name() << std::endl;
  } else {
    std::cout << "Initialized hardware, but evaluation deme has no cells!" << std::endl;
  }
  // Initialize the evaluation environment
  InitEnvironment();
  // Initialize the organism mutator!
  InitMutator();
  // How should the population be initialized?
  end_setup_sig.AddAction([this]() {
    std::cout << "Initializing population...";
    InitPop();
    std::cout << " Done" << std::endl;
    this->SetAutoMutate(); // Set to automutate after initializing population!
  });
  this->SetPopStruct_Mixed(true); // Population is well-mixed with synchronous generations.
  this->SetFitFun([this](org_t & org) { // TODO - fix fitness function!
    return org.GetPhenotype().resources_consumed;
  });
  MAX_RESPONSE_SCORE = (DEME_WIDTH * DEME_HEIGHT) * NUM_RESPONSE_TYPES;
  std::cout << "Maximum possible score in this environment = " << MAX_RESPONSE_SCORE << std::endl;
  // Initialize data collection
  InitDataCollection();
  DoWorldConfigSnapshot(config);
  end_setup_sig.Trigger(); // Trigger events to happen at end of world setup
  setup = true;
}

void MCRegWorld::Run() {
  for (size_t u = 0; u <= GENERATIONS; ++u) {
    RunStep();
    if (STOP_ON_SOLUTION & found_solution) break;
  }
}

void MCRegWorld::RunStep() {
  // (1) evaluate population, (2) select parents, (3) update world
  DoEvaluation();
  DoSelection();
  DoUpdate();
}

void MCRegWorld::PrintProgramSingleLine(const program_t & prog, std::ostream & out) {
  out << "[";
  for (size_t func_id = 0; func_id < prog.GetSize(); ++func_id) {
    if (func_id) out << ",";
    PrintProgramFunction(prog[func_id], out);
  }
  out << "]";
}

void MCRegWorld::PrintProgramFunction(const program_function_t & func, std::ostream & out) {
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

void MCRegWorld::PrintProgramInstruction(const inst_t & inst, std::ostream & out) {
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

#endif