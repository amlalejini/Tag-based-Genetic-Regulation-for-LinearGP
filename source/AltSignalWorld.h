/***
 * Alternating Signal World
 *
 **/

 // TODO - output file showing how world was initialized

#ifndef _ALT_SIGNAL_WORLD_H
#define _ALT_SIGNAL_WORLD_H

// C++ std
#include <functional>
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
#include "hardware/SignalGP/utils/MemoryModel.h"
#include "hardware/SignalGP/utils/linear_program_instructions_impls.h"
#include "hardware/SignalGP/utils/linear_functions_program_instructions_impls.h"
// Local includes
#include "AltSignalOrg.h"
#include "AltSignalConfig.h"


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
  using matchbin_metric_t = emp::StreakMetric<TAG_LEN>; // How should we measure tag similarity?
  using matchbin_selector_t = emp::RankedSelector<>;    // 0% min threshold

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

  // using event_handler_fun_t = std::function<void(hardware_t &, const event_t &)>;     ///< Type alias for event-handler functions.
  // using event_dispatcher_fun_t = std::function<void(hardware_t &, const event_t &)>;  ///< Type alias for event-dispatcher functions.
  // using event_dispatcher_set_t = emp::FunctionSet<void(hardware_t &, const event_t &)>;    ///< Type alias for dispatcher function set type.

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
  // Environment group
  size_t NUM_SIGNAL_RESPONSES;
  size_t NUM_ENV_CYCLES;
  size_t CPU_TIME_PER_ENV_CYCLE;
  // Program group
  size_t MIN_FUNC_CNT;
  size_t MAX_FUNC_CNT;
  size_t MIN_FUNC_INST_CNT;
  size_t MAX_FUNC_INST_CNT;
  // Hardware group
  size_t MAX_ACTIVE_THREAD_CNT;
  size_t MAX_THREAD_CAPACITY;
  // Selection group
  size_t TOURNAMENT_SIZE;

  Environment eval_environment;

  bool setup = false;                       ///< Has this world been setup already?
  emp::Ptr<inst_lib_t> inst_lib;            ///< Manages SignalGP instruction set.
  emp::Ptr<event_lib_t> event_lib;          ///< Manages SignalGP events.

  size_t event_id__env_sig;

  emp::Ptr<hardware_t> eval_hardware;       ///< Used to evaluate programs.

  emp::Signal<void(void)> end_setup_sig;

  size_t best_org_id=0;

  void InitConfigs(const AltSignalConfig & config);
  void InitInstLib();
  void InitEventLib();
  void InitHardware();
  void InitEnvironment();
  void InitMutator();

  void InitPop();
  void InitPop_Random();

  void DoEvaluation();
  void DoSelection();
  void DoUpdate();

  void EvaluateOrg(org_t & org);

public:
  AltSignalWorld() {}
  AltSignalWorld(emp::Random & r) : emp::World<org_t>(r) {}

  ~AltSignalWorld() {
    if (setup) {
      inst_lib.Delete();
      event_lib.Delete();
      eval_hardware.Delete();
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
  // environment group
  NUM_SIGNAL_RESPONSES = config.NUM_SIGNAL_RESPONSES();
  NUM_ENV_CYCLES = config.NUM_ENV_CYCLES();
  CPU_TIME_PER_ENV_CYCLE = config.CPU_TIME_PER_ENV_CYCLE();
  // program group
  MIN_FUNC_CNT = config.MIN_FUNC_CNT();
  MAX_FUNC_CNT = config.MAX_FUNC_CNT();
  MIN_FUNC_INST_CNT = config.MIN_FUNC_INST_CNT();
  MAX_FUNC_INST_CNT = config.MAX_FUNC_INST_CNT();
  // hardware group
  MAX_ACTIVE_THREAD_CNT = config.MAX_ACTIVE_THREAD_CNT();
  MAX_THREAD_CAPACITY = config.MAX_THREAD_CAPACITY();
  // selection group
  TOURNAMENT_SIZE = config.TOURNAMENT_SIZE();
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
  inst_lib->AddInst("WorkingToGlobal", emp::signalgp::inst_impl::Inst_WorkingToGlobal<hardware_t, inst_t>, "");
  inst_lib->AddInst("GlobalToWorking", emp::signalgp::inst_impl::Inst_GlobalToWorking<hardware_t, inst_t>, "");
  inst_lib->AddInst("Fork", emp::signalgp::inst_impl::Inst_Fork<hardware_t, inst_t>, "");
  inst_lib->AddInst("Terminate", emp::signalgp::inst_impl::Inst_Terminate<hardware_t, inst_t>, "");
  inst_lib->AddInst("If", emp::signalgp::lfp_inst_impl::Inst_If<hardware_t, inst_t>, "", {inst_prop_t::BLOCK_DEF});
  inst_lib->AddInst("While", emp::signalgp::lfp_inst_impl::Inst_While<hardware_t, inst_t>, "", {inst_prop_t::BLOCK_DEF});
  inst_lib->AddInst("Countdown", emp::signalgp::lfp_inst_impl::Inst_Countdown<hardware_t, inst_t>, "", {inst_prop_t::BLOCK_DEF});
  inst_lib->AddInst("Routine", emp::signalgp::lfp_inst_impl::Inst_Routine<hardware_t, inst_t>, "");

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
  // TODO! (after we figure out how evaluation will work)
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
                                   MIN_FUNC_CNT, MAX_FUNC_CNT,
                                   AltSignalWorldDefs::FUNC_NUM_TAGS,
                                   MIN_FUNC_INST_CNT, MAX_FUNC_INST_CNT,
                                   AltSignalWorldDefs::INST_TAG_CNT,
                                   AltSignalWorldDefs::INST_ARG_CNT,
                                   AltSignalWorldDefs::INST_MIN_ARG_VAL,
                                   AltSignalWorldDefs::INST_MAX_ARG_VAL)
                  }, 1);
  }
}

/// Evaluate entire population.
void AltSignalWorld::DoEvaluation() {
  best_org_id = 0;
  for (size_t org_id = 0; org_id < this->GetSize(); ++org_id) {
    emp_assert(this->IsOccupied(org_id));
    // std::cout << "-- Evaluate Organism ("<<org_id<<") --" << std::endl;
    EvaluateOrg(this->GetOrg(org_id));
    if (CalcFitnessID(org_id) > CalcFitnessID(best_org_id)) best_org_id = org_id;
  }
}

void AltSignalWorld::DoSelection() {
  // Keeping it simple with tournament selection!
  emp::TournamentSelect(*this, TOURNAMENT_SIZE, POP_SIZE);
}

void AltSignalWorld::DoUpdate() {
  // TODO
  // Log current update, Best fitness
  const double max_fit = CalcFitnessID(best_org_id);
  const bool found_sol = GetOrg(best_org_id).GetPhenotype().correct_resp_cnt == NUM_ENV_CYCLES;
  std::cout << "update: " << this->GetUpdate() << "; ";
  std::cout << "best score (" << best_org_id << "): " << max_fit << "; ";
  std::cout << "solution found: " << found_sol << std::endl;
  this->Update();
  this->ClearCache();
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
    eval_hardware->GetCustomComponent().Reset();
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
// ---- PUBLIC IMPLEMENTATIONS ----

void AltSignalWorld::Setup(const AltSignalConfig & config) {
  // Localize configuration parameters.
  InitConfigs(config);

  // Create instruction/event libraries.
  InitInstLib();
  InitEventLib();
  // Init evaluation hardware
  InitHardware();
  // Init evaluation environment
  InitEnvironment();
  // Initialize organism mutators!.
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

  end_setup_sig.Trigger();
  setup = true;
}

void AltSignalWorld::Run() {
  for (size_t u = 0; u <= GENERATIONS; ++u) {
    RunStep();
  }
}

void AltSignalWorld::RunStep() {
  // (1) evaluate pop, (2) select parents, (3) update world
  DoEvaluation();
  DoSelection();
  DoUpdate();
}

#endif