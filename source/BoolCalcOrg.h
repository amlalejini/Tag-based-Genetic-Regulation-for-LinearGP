#ifndef DIR_SIG_DIGITAL_ORGANISM_H
#define DIR_SIG_DIGITAL_ORGANISM_H

#include <tuple>
#include <algorithm>
#include "hardware/SignalGP/utils/LinearFunctionsProgram.h"
// #include "mutation_utils.h"

template<typename TAG_T, typename INST_ARG_T>
class BoolCalcOrganism {
public:
  struct BoolCalcGenome;
  struct BoolCalcPhenotype;

  using program_t = sgp::LinearFunctionsProgram<TAG_T, INST_ARG_T>;
  using genome_t = BoolCalcGenome;
  using phenotype_t = BoolCalcPhenotype;

  struct BoolCalcGenome {
    using this_t = BoolCalcGenome;
    program_t program;

    BoolCalcGenome(const program_t & p) : program(p) {}
    BoolCalcGenome(const this_t &) = default;
    BoolCalcGenome(this_t &&) = default;

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

  /// Phenotype definition for directional signal task organisms.
  struct BoolCalcPhenotype {
    using this_t = BoolCalcPhenotype;

    emp::vector<double> test_scores;  ///< Scores on tests. Each test corresponds to a different signal sequence.
    emp::vector<size_t> test_ids;     ///< Sequence IDs of tests (aligned with test_scores)
    double aggregate_score=0.0;       ///< Aggregate score across all signal sequences tested on (i.e., sum(test_scores))
    size_t num_passes=0;

    void Reset(size_t tests=1) {
      aggregate_score=0.0;         // Reset aggregate score.
      num_passes=0;
      test_scores.resize(tests);   // Make sure trial scores is right size.
      test_ids.resize(tests);
      std::fill(test_scores.begin(), test_scores.end(), 0); // Fill with zeroes.
      std::fill(test_ids.begin(), test_ids.end(), 0);
    }

    bool operator==(const this_t & o) const {
      return std::tie(test_scores,
                      test_ids,
                      aggregate_score,
                      num_passes
                      )
          == std::tie(o.test_scores,
                      o.test_ids,
                      o.aggregate_score,
                      o.num_passes
                      );
    }

    bool operator!=(const this_t & o) const {
      return !(*this == o);
    }

    bool operator<(const this_t & o) const {
      return std::tie(test_scores,
                      test_ids,
                      aggregate_score,
                      num_passes
                    )
            < std::tie(o.test_scores,
                       o.test_ids,
                       o.aggregate_score,
                       o.num_passes
                      );
    }

    double GetAggregateScore() const { return aggregate_score; }
  };

protected:
  phenotype_t phenotype;
  genome_t genome;

public:
  BoolCalcOrganism(const genome_t & g)
    : phenotype(), genome(g) {}

  BoolCalcOrganism(const BoolCalcOrganism &) = default;
  BoolCalcOrganism(BoolCalcOrganism &&) = default;

  genome_t & GetGenome() { return genome; }
  const genome_t & GetGenome() const { return genome; }

  phenotype_t & GetPhenotype() { return phenotype; }
  const phenotype_t & GetPhenotype() const { return phenotype; }

};

#endif