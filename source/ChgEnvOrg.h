#ifndef CHG_ENV_DIGITAL_ORGANISM_H
#define CHG_ENV_DIGITAL_ORGANISM_H

#include <tuple>
#include "hardware/SignalGP/utils/LinearFunctionsProgram.h"
#include "mutation_utils.h"

/// Organism class for the changing signal task.
template<typename TAG_T, typename INST_ARG_T>
class ChgEnvOrganism {
public:
  struct ChgEnvGenome;
  struct ChgEnvPhenotype;

  using program_t = sgp::LinearFunctionsProgram<TAG_T, INST_ARG_T>;
  using genome_t = ChgEnvGenome;
  using phenotype_t = ChgEnvPhenotype;

  struct ChgEnvGenome {
    program_t program;
    ChgEnvGenome(const program_t & p) : program(p) {}
    ChgEnvGenome(const ChgEnvGenome &) = default;
    ChgEnvGenome(ChgEnvGenome &&) = default;

    bool operator==(const ChgEnvGenome & other) const {
      return program == other.program;
    }

    bool operator!=(const ChgEnvGenome & other) const {
      return !(*this == other);
    }

    bool operator<(const ChgEnvGenome & other) const {
      return program < other.program;
    }

    const program_t & GetProgram() const { return program; }
    program_t & GetProgram() { return program; }

  };

  struct ChgEnvPhenotype {
    double score=0.0;       ///< i.e., fitness
    size_t env_matches=0;   ///< How many signals did organism respond correctly to?
    size_t env_misses=0;    ///< How many signals did organism respond incorrectly to?
    size_t no_responses=0;  ///< How many signals did organism not respond to?

    void Reset() {
      score=0.0;
      env_matches=0;
      env_misses=0;
      no_responses=0;
    }

    bool operator==(const ChgEnvPhenotype & o) const {
      return std::tie(score,
                      env_matches,
                      env_misses,
                      no_responses)
          == std::tie(o.score,
                      o.env_matches,
                      o.env_misses,
                      o.no_responses);
    }

    bool operator!=(const ChgEnvPhenotype & o) const {
      return !(*this == o);
    }

    bool operator<(const ChgEnvPhenotype & o) const {
      return std::tie(score,
                      env_matches,
                      env_misses,
                      no_responses)
           < std::tie(o.score,
                      o.env_matches,
                      o.env_misses,
                      o.no_responses);
    }

    double GetScore() const { return score; }
    size_t GetEnvMatches() const { return env_matches; }
    size_t GetEnvMisses() const { return env_misses; }
    size_t GetNoResponses() const { return no_responses; }
  };

protected:
  phenotype_t phenotype;
  genome_t genome;

  std::unordered_map<std::string, int> mutations; ///< Mutations from parent.

public:
  ChgEnvOrganism(const genome_t & g)
    : phenotype(), genome(g) {}

  ChgEnvOrganism(const ChgEnvOrganism &) = default;
  ChgEnvOrganism(ChgEnvOrganism &&) = default;

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