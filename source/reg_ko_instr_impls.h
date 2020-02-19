#ifndef KO_REG_INST_IMPLS_H
#define KO_REG_INST_IMPLS_H

namespace inst_impls {
  /// Non-default instruction: SetRegulator
  /// Number of arguments: 2
  /// Description: Sets the regulator of a tag in the matchbin.
  template<typename HARDWARE_T, typename INSTRUCTION_T>
  void Inst_SetRegulator_KO_UP_REG(HARDWARE_T & hw, const INSTRUCTION_T & inst) {
    emp::vector<size_t> best_fun(hw.GetMatchBin().MatchRaw(inst.GetTag(0), 1));
    if (best_fun.size() == 0) { return; }
    auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
    auto & mem_state = call_state.GetMemory();
    const double new_regulator_val = mem_state.AccessWorking(inst.GetArg(0));
    const double old_regulator_val = hw.GetMatchBin().ViewRegulator(best_fun[0]);
    if (new_regulator_val < old_regulator_val) return; // knockout up regulation
    // (+) values down regulate
    // (-) values up regulate
    hw.GetMatchBin().SetRegulator(best_fun[0], new_regulator_val);
  }

  template<typename HARDWARE_T, typename INSTRUCTION_T>
  void Inst_SetRegulator_KO_DOWN_REG(HARDWARE_T & hw, const INSTRUCTION_T & inst) {
    emp::vector<size_t> best_fun(hw.GetMatchBin().MatchRaw(inst.GetTag(0), 1));
    if (best_fun.size() == 0) { return; }
    auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
    auto & mem_state = call_state.GetMemory();
    const double new_regulator_val = mem_state.AccessWorking(inst.GetArg(0));
    const double old_regulator_val = hw.GetMatchBin().ViewRegulator(best_fun[0]);
    if (new_regulator_val > old_regulator_val) return; // knockout down regulation
    // (+) values down regulate
    // (-) values up regulate
    hw.GetMatchBin().SetRegulator(best_fun[0], new_regulator_val);
  }


  /// Non-default instruction: SetOwnRegulator
  /// Number of arguments: 2
  /// Description: Sets the regulator of the currently executing function.
  template<typename HARDWARE_T, typename INSTRUCTION_T>
  static void Inst_SetOwnRegulator_KO_UP_REG(HARDWARE_T & hw, const INSTRUCTION_T & inst) {
    auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
    auto & mem_state = call_state.GetMemory();
    auto & flow = call_state.GetTopFlow();
    const double new_regulator_val = mem_state.AccessWorking(inst.GetArg(0));
    const double old_regulator_val = hw.GetMatchBin().ViewRegulator(flow.GetMP());
    if (new_regulator_val < old_regulator_val) return; // knockout up regulation
    // (+) values down regulate
    // (-) values up regulate
    hw.GetMatchBin().SetRegulator(flow.GetMP(), new_regulator_val);
  }

  template<typename HARDWARE_T, typename INSTRUCTION_T>
  static void Inst_SetOwnRegulator_KO_DOWN_REG(HARDWARE_T & hw, const INSTRUCTION_T & inst) {
    auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
    auto & mem_state = call_state.GetMemory();
    auto & flow = call_state.GetTopFlow();
    const double new_regulator_val = mem_state.AccessWorking(inst.GetArg(0));
    const double old_regulator_val = hw.GetMatchBin().ViewRegulator(flow.GetMP());
    if (new_regulator_val > old_regulator_val) return; // knockout down regulation
    // (+) values down regulate
    // (-) values up regulate
    hw.GetMatchBin().SetRegulator(flow.GetMP(), new_regulator_val);
  }

  template<typename HARDWARE_T, typename INSTRUCTION_T>
  void Inst_ClearRegulator_KO_UP_REG(HARDWARE_T & hw, const INSTRUCTION_T & inst) {
    emp::vector<size_t> best_fun(hw.GetMatchBin().MatchRaw(inst.GetTag(0), 1));
    if (best_fun.size() == 0) { return; }
    const double old_regulator_val = hw.GetMatchBin().ViewRegulator(best_fun[0]);
    if (0 < old_regulator_val) return; // knockout up
    hw.GetMatchBin().SetRegulator(best_fun[0], 0);
  }

  template<typename HARDWARE_T, typename INSTRUCTION_T>
  void Inst_ClearRegulator_KO_DOWN_REG(HARDWARE_T & hw, const INSTRUCTION_T & inst) {
    emp::vector<size_t> best_fun(hw.GetMatchBin().MatchRaw(inst.GetTag(0), 1));
    if (best_fun.size() == 0) { return; }
    const double old_regulator_val = hw.GetMatchBin().ViewRegulator(best_fun[0]);
    if (0 > old_regulator_val) return; // knockout down
    hw.GetMatchBin().SetRegulator(best_fun[0], 0);
  }

  template<typename HARDWARE_T, typename INSTRUCTION_T>
  static void Inst_ClearOwnRegulator_KO_UP_REG(HARDWARE_T & hw, const INSTRUCTION_T & inst) {
    auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
    auto & flow = call_state.GetTopFlow();
    const double old_regulator_val = hw.GetMatchBin().ViewRegulator(flow.GetMP());
    if (0 < old_regulator_val) return; // knockout up reg
    hw.GetMatchBin().SetRegulator(flow.GetMP(), 0);
  }

  template<typename HARDWARE_T, typename INSTRUCTION_T>
  static void Inst_ClearOwnRegulator_KO_DOWN_REG(HARDWARE_T & hw, const INSTRUCTION_T & inst) {
    auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
    auto & flow = call_state.GetTopFlow();
    const double old_regulator_val = hw.GetMatchBin().ViewRegulator(flow.GetMP());
    if (0 > old_regulator_val) return; // knockout down reg
    hw.GetMatchBin().SetRegulator(flow.GetMP(), 0);
  }

  /// Non-default instruction: AdjRegulator
  /// Number of arguments: 3
  template<typename HARDWARE_T, typename INSTRUCTION_T>
  static void Inst_AdjRegulator_KO_UP_REG(HARDWARE_T & hw, const INSTRUCTION_T & inst) {
    // const State & state = hw.GetCurState();
    emp::vector<size_t> best_fun = hw.GetMatchBin().MatchRaw(inst.GetTag(0), 1);
    if (!best_fun.size()) return;
    auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
    auto & mem_state = call_state.GetMemory();
    const double adj = mem_state.AccessWorking(inst.GetArg(0));
    if (adj < 0) return; // knockout up regulation
    hw.GetMatchBin().AdjRegulator(best_fun[0], adj);
  }

  /// Non-default instruction: AdjRegulator
  /// Number of arguments: 3
  template<typename HARDWARE_T, typename INSTRUCTION_T>
  static void Inst_AdjRegulator_KO_DOWN_REG(HARDWARE_T & hw, const INSTRUCTION_T & inst) {
    // const State & state = hw.GetCurState();
    emp::vector<size_t> best_fun = hw.GetMatchBin().MatchRaw(inst.GetTag(0), 1);
    if (!best_fun.size()) return;
    auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
    auto & mem_state = call_state.GetMemory();
    const double adj = mem_state.AccessWorking(inst.GetArg(0));
    if (adj > 0) return;
    hw.GetMatchBin().AdjRegulator(best_fun[0], adj);
  }

  /// Non-default instruction: AdjOwnRegulator
  /// Number of arguments: 3
  template<typename HARDWARE_T, typename INSTRUCTION_T>
  static void Inst_AdjOwnRegulator_KO_UP_REG(HARDWARE_T & hw, const INSTRUCTION_T & inst) {
    auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
    auto & mem_state = call_state.GetMemory();
    auto & flow = call_state.GetTopFlow();
    const double adj = mem_state.AccessWorking(inst.GetArg(0));
    if (adj < 0) return;
    hw.GetMatchBin().AdjRegulator(flow.GetMP(), adj);
  }

  /// Non-default instruction: AdjOwnRegulator
  /// Number of arguments: 3
  template<typename HARDWARE_T, typename INSTRUCTION_T>
  static void Inst_AdjOwnRegulator_KO_DOWN_REG(HARDWARE_T & hw, const INSTRUCTION_T & inst) {
    auto & call_state = hw.GetCurThread().GetExecState().GetTopCallState();
    auto & mem_state = call_state.GetMemory();
    auto & flow = call_state.GetTopFlow();
    const double adj = mem_state.AccessWorking(inst.GetArg(0));
    if (adj > 0) return;
    hw.GetMatchBin().AdjRegulator(flow.GetMP(), adj);
  }
}

#endif