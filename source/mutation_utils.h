#ifndef _SIGNALGP_MUTATION_UTILS_H
#define _SIGNALGP_MUTATION_UTILS_H

#include "tools/BitSet.h"
#include "tools/Range.h"

///
template<typename HARDWARE_T, typename TAG_T, typename ARGUMENT_T>
class MutatorLinearFunctionsProgram {
public:
  MutatorLinearFunctionsProgram() {
    emp_assert(false, "Not implemented!");
  }
};

template<typename HARDWARE_T, size_t TAG_W>
class MutatorLinearFunctionsProgram<HARDWARE_T, emp::BitSet<TAG_W>, int> {
public:
  using tag_t = emp::BitSet<TAG_W>;
  using arg_t = int;
  using program_t = emp::signalgp::LinearFunctionsProgram<tag_t, arg_t>;
  using function_t = typename program_t::function_t;
  using inst_t = typename program_t::inst_t;
  using hardware_t = HARDWARE_T;
  using inst_lib_t = typename HARDWARE_T::inst_lib_t;

protected:
  // Need an instruction library to know valid
  inst_lib_t & inst_lib;
  // Program constraints.
  emp::Range<size_t> prog_func_cnt_range = {0, 4}; ///< min/max # functions in a program
  emp::Range<size_t> prog_func_inst_range = {0, 8}; ///< min/max # of instructions a function can have.
  emp::Range<int> prog_inst_arg_val_range = {0, 15};
  size_t prog_func_num_tags=1;  ///< Number of tags associated with each function.
  size_t prog_total_inst=4*8;   ///< How many total instructions is a program allowed to have?
  size_t prog_inst_num_args=3;  ///< How many arguments should each instruction have?
  size_t prog_inst_num_tags=1;  ///< How many tags should each instruction have?
  // Rates
  double rate_inst_arg_sub=0.0;
  double rate_inst_tag_bit_flip=0.0;
  double rate_inst_sub=0.0;
  double rate_inst_ins=0.0;
  double rate_inst_del=0.0;
  double rate_seq_slip=0.0;
  double rate_func_dup=0.0;
  double rate_func_del=0.0;
  double rate_func_tag_bit_flip=0.0;

public:
  MutatorLinearFunctionsProgram(inst_lib_t & ilib)
    : inst_lib(ilib)
  { std::cout << "Hi there" << std::endl; }

  /// Apply bit flips to tag @ per-bit rate.
  size_t ApplyTagBitFlips(emp::Random & rnd, tag_t & tag, double rate) {
    size_t mut_cnt = 0;
    for (size_t k = 0; k < tag.GetSize(); ++k) {
      if (rnd.P(rate)) {
        tag.Toggle(k);
        ++mut_cnt;
      }
    }
    return mut_cnt;
  }

  /// Apply instruction substitutions (operator, argument, tag).
  size_t ApplyInstSubs(emp::Random & rnd, program_t & program) {
    size_t mut_cnt = 0;
    for (size_t fID = 0; fID < program.GetSize(); ++fID) {
      for (size_t iID = 0; iID < program[fID].GetSize(); ++iID) {
        inst_t & inst = program[fID][iID];
        // Mutate instruction tag(s).
        for (tag_t & tag : inst.GetTags()) {
          mut_cnt += ApplyTagBitFlips(rnd, tag, rate_inst_tag_bit_flip);
        }
        // Mutate instruction operation.
        if (rnd.P(rate_inst_sub)) {
          inst.id = rnd.GetUInt(inst_lib.GetSize());
          ++mut_cnt;
        }
        // Mutate instruction arguments.
        for (size_t k = 0; k < inst.GetArgs().size(); ++k) {
          if (rnd.P(rate_inst_arg_sub)) {
            inst.GetArgs()[k] = rnd.GetInt(prog_inst_arg_val_range.GetLower(),
                                           prog_inst_arg_val_range.GetUpper()+1);
            ++mut_cnt;
          }
        }
      }
    }
    return mut_cnt;
  }

  /// Apply single-instruction insertions and deletions.
  size_t ApplyInstInDels(emp::Random & rnd, program_t & program) {
    size_t mut_cnt = 0;
    size_t expected_prog_len = program.GetInstCount();
    // Perform single-instruction insertion/deletions.
    for (size_t fID = 0; fID < program.GetSize(); ++fID) {
      function_t new_function(program[fID].GetTags()); // Copy over tags
      size_t expected_func_len = program[fID].GetSize();
      // Compute number and location of insertions.
      const uint32_t num_ins = rnd.GetRandBinomial(program[fID].GetSize(), rate_inst_ins);
      emp::vector<size_t> ins_locs;
      if (num_ins > 0) {
        ins_locs = emp::RandomUIntVector(rnd, num_ins, 0, program[fID].GetSize());
        std::sort(ins_locs.rbegin(), ins_locs.rend());
      }
      size_t read_head = 0;
      while (read_head < program[fID].GetSize()) {
        // Should we insert?
        if (ins_locs.size() > 0) {
          if (read_head >= ins_locs.back() &&
              expected_func_len < prog_func_inst_range.GetUpper() &&
              expected_prog_len < prog_total_inst)
          {
            // Insert a new random instruction.
            new_function.PushInst(emp::signalgp::GenRandInst<hardware_t, TAG_W>(rnd,inst_lib, prog_inst_num_tags,prog_inst_num_args,prog_inst_arg_val_range.GetLower(),prog_inst_arg_val_range.GetUpper()));
            ++mut_cnt;
            ++expected_func_len;
            ++expected_prog_len;
            ins_locs.pop_back();
            continue;
          }
        }
        // Should we delete this instruction?
        if (rnd.P(rate_inst_del) && expected_func_len > prog_func_inst_range.GetLower()) {
          ++mut_cnt;
          --expected_func_len;
          --expected_prog_len;
        } else {
          new_function.PushInst(program[fID][read_head]);
        }
        ++read_head;
      }
      program[fID] = new_function;
    }
    return mut_cnt;
  }

  /// Apply slip mutations to program sequences (per-function instruction sequence).
  size_t ApplySeqSlips(emp::Random & rnd, program_t & program);
  /// Apply function duplications to program (per-function).
  size_t ApplyFuncDup(emp::Random & rnd, program_t & program);
  /// Apply function deletions to program (per-function).
  size_t ApplyFuncDel(emp::Random & rnd, program_t & program);
  /// Apply function tag bit-flip mutations.
  size_t ApplyFuncTagBF(emp::Random & rnd, program_t & program);



};

  // template<typename Hardware>
  // class SignalGPMutatorFacade : public SignalGPMutator<Hardware::affinity_width, typename Hardware::trait_t, typename Hardware::matchbin_t> { } ;

#endif