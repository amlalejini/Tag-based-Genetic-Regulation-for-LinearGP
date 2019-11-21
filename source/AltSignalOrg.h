#ifndef ALT_SIGNAL_DIGITAL_ORGANISM_H
#define ALT_SIGNAL_DIGITAL_ORGANISM_H

#include <tuple>

// New signalgp includes
#include "hardware/SignalGP/utils/LinearFunctionsProgram.h"

// local includes
#include "mutation_utils.h"

template<typename TAG_T, typename INST_ARG_T>
class AltSignalOrganism {
public:
  struct AltSignalGenome;

  using program_t = emp::signalgp::LinearFunctionsProgram<TAG_T, INST_ARG_T>;
  using genome_t = AltSignalGenome;

  struct AltSignalGenome {
    program_t program;
    AltSignalGenome(const program_t & p) : program(p) {}
    AltSignalGenome(const AltSignalGenome &) = default;
    AltSignalGenome(AltSignalGenome &&) = default;

    bool operator==(const AltSignalGenome & other) const {
      return program == other.program;
    }

    bool operator!=(const AltSignalGenome & other) const {
      return !(*this == other);
    }

    bool operator<(const AltSignalGenome & other) const {
      return program < other.program;
    }

    const program_t & GetProgram() const { return program; }
    program_t & GetProgram() { return program; }

  };

  struct AltSignalPhenotype {
    double resources_consumed=0.0;
    size_t correct_resp_cnt=0;      ///< How many correct responses did organism achieve during evaluation?
    size_t no_resp_cnt=0;           ///< How many 'no response' updates did organism have during evaluation?

    void Reset() {
      resources_consumed = 0;
      correct_resp_cnt = 0;
      no_resp_cnt = 0;
    }

    bool operator==(const AltSignalPhenotype & o) const {
      return std::tie(resources_consumed, correct_resp_cnt, no_resp_cnt) == std::tie(o.resources_consumed, o.correct_resp_cnt, o.no_resp_cnt);
    }

    bool operator!=(const AltSignalPhenotype & o) const {
      return !(*this == o);
    }

    bool operator<(const AltSignalPhenotype & o) const {
      return std::tie(resources_consumed, correct_resp_cnt, no_resp_cnt) < std::tie(o.resources_consumed, o.correct_resp_cnt, o.no_resp_cnt);
    }

    double GetResources() const { return resources_consumed; }
    size_t GetCorrectResponses() const { return correct_resp_cnt; }
    size_t GetNoResponses() const { return no_resp_cnt; }
  };

protected:
  AltSignalPhenotype phenotype;
  genome_t genome;

  std::unordered_map<std::string, int> mutations; ///< Mutations from parent.

public:
  AltSignalOrganism(const genome_t & g)
    : phenotype(), genome(g) {}

  AltSignalOrganism(const AltSignalOrganism &) = default;
  AltSignalOrganism(AltSignalOrganism &&) = default;

  genome_t & GetGenome() { return genome; }
  const genome_t & GetGenome() const { return genome; }

  AltSignalPhenotype & GetPhenotype() { return phenotype; }
  const AltSignalPhenotype & GetPhenotype() const { return phenotype; }

  std::unordered_map<std::string, int> & GetMutations() { return mutations; }
  const std::unordered_map<std::string, int> & GetMutations() const { return mutations; }

  void ResetMutations() {
    for (auto & pair : mutations) {
      pair.second = 0;
    }
  }

};

#endif