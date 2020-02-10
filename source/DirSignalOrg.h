#ifndef DIR_SIG_DIGITAL_ORGANISM_H
#define DIR_SIG_DIGITAL_ORGANISM_H

#include <tuple>

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

  struct DirSigPhenotype {
    using this_t = DirSigPhenotype;
    double score=0.0;
    size_t env_matches=0;
    size_t env_misses=0;
    size_t no_responses=0;

    void Reset() {
      score=0.0;
      env_matches=0;
      env_misses=0;
      no_responses=0;
    }

    bool operator==(const this_t & o) const {
      return std::tie(score,
                      env_matches,
                      env_misses,
                      no_responses)
          == std::tie(o.score,
                      o.env_matches,
                      o.env_misses,
                      o.no_responses);
    }

    bool operator!=(const this_t & o) const {
      return !(*this == o);
    }

    bool operator<(const this_t & o) const {
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