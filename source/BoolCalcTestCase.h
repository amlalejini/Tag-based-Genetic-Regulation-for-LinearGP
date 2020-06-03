#ifndef BOOL_CALC_TEST_CASE_H
#define BOOL_CALC_TEST_CASE_H

#include <string>
#include "base/vector.h"

namespace BoolCalcTestInfo {

  enum class RESPONSE_TYPE { NONE=0, WAIT, ERROR, NUMERIC };
  enum class INPUT_SIGNAL_TYPE { OPERATOR=0, OPERAND };
  using operand_t = uint32_t; // Problem operand type

  /// Describes a single 'button press' from a boolean calculator test case
  struct BoolCalcTestSignal {
    using input_sig_t = INPUT_SIGNAL_TYPE;

    input_sig_t type;
    std::string operator_str;
    operand_t operand;

    BoolCalcTestSignal(operand_t num)
      : type(input_sig_t::OPERAND), operator_str(""), operand(num) { ; }
    BoolCalcTestSignal(const std::string & op)
      : type(input_sig_t::OPERATOR), operator_str(op), operand(0) { ; }

    const std::string & GetOperator() const { return operator_str; }
    operand_t GetOperand() const { return operand; }
  };

  struct BoolCalcTestCase {
    std::string input_str; // original input string descriptor
    // emp::vector<

  };

}




#endif