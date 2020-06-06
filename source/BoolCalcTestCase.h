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

    input_sig_t signal_type;          ///< What type of input signal is this? Operand or Operator?
    std::string operator_str;         ///< String representation of operator (only when signal_type == operator)
    size_t signal_id;                 ///< ID of this type of input signal (assigned by world)
    operand_t operand;                ///< If this is an operand signal, what is the operand's value?
    response_t correct_response_type; ///< What is the correct type of response to this signal?
    operand_t numeric_response;       ///< If the correct response type is numeric, what value is correct?

    TestSignal(operand_t num, response_t resp)
      : signal_type(input_sig_t::OPERAND),
        operator_str(""),
        signal_id(0),
        operand(num),
        correct_response_type(resp),
        numeric_response(0)
    { ; }

    TestSignal(const std::string & op, response_t resp)
      : signal_type(input_sig_t::OPERATOR),
        operator_str(op),
        signal_id(0),
        operand(0),
        correct_response_type(resp),
        numeric_response(0)
    { ; }

    const std::string & GetOperator() const { return operator_str; }
    operand_t GetOperand() const { return operand; }
    input_sig_t GetSignalType() const { return signal_type; }
    size_t GetSignalID() const { return signal_id; }
    response_t GetCorrectResponseType() const { return correct_response_type; }
    operand_t GetNumericResponse() const { return numeric_response; }
    bool IsOperator() const { return signal_type == input_sig_t::OPERATOR; }
    bool IsOperand() const { return signal_type == input_sig_t::OPERAND; }
    bool IsCorrect(response_t resp, operand_t num=0) const {
      return ( resp == correct_response_type ) &&
             ( (correct_response_type!=response_t::NUMERIC) || (num==numeric_response) );
    }

    void Print(std::ostream & out=std::cout) {
      out << "{";
      out << "signal-type:" << InputSignalTypeStr(signal_type) << ",";
      out << "signal-id:" << signal_id << ",";
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
    std::string input_str;  // original input string descriptor
    std::string output_str;
    std::string type_str;
    size_t type_id;         // filled out by the world
    emp::vector<TestSignal> test_signals; // Interpretted test signal sequence.
  };

}




#endif