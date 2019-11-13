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
#include "hardware/SignalGP/utils/linear_program_instructions_impls.h"
#include "hardware/SignalGP/utils/linear_functions_program_instructions_impls.h"
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
  using inst_t = typename hardware_t::inst_t;
  using inst_prop_t = typename hardware_t::InstProperty;

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
  void InitInstLib();

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
  InitInstLib();
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