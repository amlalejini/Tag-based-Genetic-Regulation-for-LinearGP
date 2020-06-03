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
#include "BoolCalcWorld.h"
#include "Event.h"
#include "reg_ko_instr_impls.h"

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
    std::conditional<STRINGVIEWIFY(MATCH_METRIC) == "hash",
      emp::UnifMod<emp::CryptoHashMetric<TAG_LEN>>,
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
  using operand_t = uint32_t; // Problem operand type
}

/// Custom hardware component for SignalGP
struct BoolCalcCustomHardware {
  using operand_t = typename BoolCalcWorldDefs::operand_t;

  enum class RESPONSE_TYPE { NONE=0, WAIT, ERROR, NUMERIC };

  RESPONSE_TYPE response_type=RESPONSE_TYPE::NONE;
  operand_t response_value=0;
  bool responded=false;

  void Reset() {
    response_type = RESPONSE_TYPE::NONE;
    response_value = 0
    responded = false;
  }

  void ExpressWait() {
    response_type = RESPONSE_TYPE::WAIT;
    responded = true;
  }

  void ExpressError() {
    response_type = RESPONSE_TYPE::ERROR;
    responded = true;
  }

  void ExpressNumeric(operand_t val) {
    response_type = RESPONSE_TYPE::NUMERIC;
    response_value = val;
    responded = false;
  }

  bool HasResponse() const { return responded; }
  RESPONSE_TYPE GetResponseType() const { return response_type; }
  operand_t GetResponseValue() const { return response_value; }

};

class BoolCalcWorld : public emp::World<BoolCalcWorldDefs::org_t> {
public:
  using tag_t = emp::BitSet<BoolCalcWorldDefs::TAG_LEN>;
  using inst_arg_t = int;
  using config_t = BoolCalcConfig;
  using custom_comp_t = BoolCalcCustomHardware;

  using org_t = BoolCalcOrganism<tag_t, inst_arg_t>;
  using phenotype_t = typename org_t::phenotype_t;

  using matchbin_t = emp::MatchBin<BoolCalcWorldDefs::matchbin_val_t,
                                   BoolCalcWorldDefs::matchbin_metric_t,
                                   BoolCalcWorldDefs::matchbin_selector_t,
                                   BoolCalcWorldDefs::matchbin_regulator_t>;

  using mem_model_t = SimpleMemoryModel;
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

protected:
public:
}

#endif