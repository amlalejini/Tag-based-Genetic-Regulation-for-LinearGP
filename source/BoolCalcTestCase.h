#ifndef BOOL_CALC_TEST_CASE_H
#define BOOL_CALC_TEST_CASE_H

#include <string>
#include "base/vector.h"

namespace BoolCalcTestInfo {

  enum class RESPONSE_TYPE { NONE=0, WAIT, ERROR, NUMERIC };
  enum class INPUT_SIGNAL_TYPE { OPERATOR=0, OPERAND };
  using operand_t = uint32_t; // Problem operand type

  std::string ResponseStr(RESPONSE_TYPE resp_type) {
    switch (resp_type) {
      case RESPONSE_TYPE::NONE: {
        return "NONE";
      }
      case RESPONSE_TYPE::WAIT: {
        return "WAIT";
      }
      case RESPONSE_TYPE::ERROR: {
        return "ERROR";
      }
      case RESPONSE_TYPE::NUMERIC: {
        return "NUMERIC";
      }
      default: {
        return "UNKNOWN";
      }
    }
  }

  std::string InputSignalTypeStr(INPUT_SIGNAL_TYPE in_type) {
    switch (in_type) {
      case INPUT_SIGNAL_TYPE::OPERATOR: {
        return "OPERATOR";
      }
      case INPUT_SIGNAL_TYPE::OPERAND: {
        return "OPERAND";
      }
      default: {
        return "UNKNOWN";
      }
    }
  }

  /// Describes a single 'button press' from a boolean calculator test case
  struct TestSignal {
    using input_sig_t = INPUT_SIGNAL_TYPE;
    using response_t = RESPONSE_TYPE;

    input_sig_t signal_type;
    std::string operator_str;
    operand_t operand;
    response_t correct_response_type;
    operand_t numeric_response;

    TestSignal(operand_t num, response_t resp)
      : signal_type(input_sig_t::OPERAND),
        operator_str(""),
        operand(num),
        correct_response_type(resp),
        numeric_response(0)
    { ; }

    TestSignal(const std::string & op, response_t resp)
      : signal_type(input_sig_t::OPERATOR),
        operator_str(op),
        operand(0),
        correct_response_type(resp),
        numeric_response(0)
    { ; }

    const std::string & GetOperator() const { return operator_str; }
    operand_t GetOperand() const { return operand; }
    input_sig_t GetSignalType() const { return signal_type; }
    response_t GetCorrectResponseType() const { return correct_response_type; }
    operand_t GetNumericResponse() const { return numeric_response; }
    bool IsOperator() const { return signal_type == input_sig_t::OPERATOR; }
    bool IsOperand() const { return signal_type == input_sig_t::OPERAND; }

    void Print(std::ostream & out=std::cout) {
      out << "{";
      out << "signal-type:" << InputSignalTypeStr(signal_type) << ",";
      if (IsOperator())
        out << "operator:" <<  operator_str << ",";
      if (IsOperand())
        out << "operand:" << operand << ",";
      out << "resp-type:" << ResponseStr(correct_response_type);
      if (correct_response_type==response_t::NUMERIC)
        out << ",resp-val:" << numeric_response;
      out << "}";
    }
  };

  /// Represents a single test case for the boolean logic calculator problem.
  struct TestCase {
    std::string input_str; // original input string descriptor
    std::string output_str;
    std::string type_str;
    emp::vector<TestSignal> test_signals; // Interpretted test signal sequence.
  };

}




#endif