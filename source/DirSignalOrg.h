#ifndef DIR_SIG_DIGITAL_ORGANISM_H
#define DIR_SIG_DIGITAL_ORGANISM_H

#include <tuple>
#include <algorithm>

// New signalgp includes
#include "hardware/SignalGP/utils/LinearFunctionsProgram.h"

// local includes
#include "mutation_utils.h"

template<typename TAG_T, typename INST_ARG_T>
class DirSigOrganism {
public:
  struct DirSigGenome;
  struct DirSigPhenotype;

  using program_t = sgp::LinearFunctionsProgram<TAG_T, INST_ARG_T>;
  using genome_t = DirSigGenome;
  using phenotype_t = DirSigPhenotype;

  struct DirSigGenome {
    using this_t = DirSigGenome;
    program_t program;
    DirSigGenome(const program_t & p) : program(p) {}
    DirSigGenome(const this_t &) = default;
    DirSigGenome(this_t &&) = default;

    bool operator==(const this_t & other) const {
      return program == other.program;
    }

    bool operator!=(const this_t & other) const {
      return !(*this == other);
    }

    bool operator<(const this_t & other) const {
      return program < other.program;
    }

    const program_t & GetProgram() const { return program; }
    program_t & GetProgram() { return program; }

  };

  // struct TrialProfile {
  //   double score;
  // };

  struct DirSigPhenotype {
    using this_t = DirSigPhenotype;

    emp::vector<double> test_scores;
    double aggregate_score=0.0;

    void Reset(size_t tests=1) {
      aggregate_score=0.0;         // Reset aggregate score.
      test_scores.resize(tests); // Make sure trial scores is right size.
      std::fill(test_scores.begin(), test_scores.end(), 0); // Fill with zeroes.
    }

    bool operator==(const this_t & o) const {
      return std::tie(test_scores,
                      aggregate_score)
          == std::tie(o.trial_scores,
                      o.aggregate_score);
    }

    bool operator!=(const this_t & o) const {
      return !(*this == o);
    }

    bool operator<(const this_t & o) const {
      return std::tie(test_scores,
                      aggregate_score)
            < std::tie(o.test_scores,
                       o.aggregate_score);
    }

    double GetAggregateScore() const { return aggregate_score; }

  };

protected:
  phenotype_t phenotype;
  genome_t genome;

  std::unordered_map<std::string, int> mutations; ///< Mutations from parent.

public:
  DirSigOrganism(const genome_t & g)
    : phenotype(), genome(g) {}

  DirSigOrganism(const DirSigOrganism &) = default;
  DirSigOrganism(DirSigOrganism &&) = default;

  genome_t & GetGenome() { return genome; }
  const genome_t & GetGenome() const { return genome; }

  phenotype_t & GetPhenotype() { return phenotype; }
  const phenotype_t & GetPhenotype() const { return phenotype; }

  std::unordered_map<std::string, int> & GetMutations() { return mutations; }
  const std::unordered_map<std::string, int> & GetMutations() const { return mutations; }

  void ResetMutations() {
    for (auto & pair : mutations) {
      pair.second = 0;
    }
  }

};

#endif