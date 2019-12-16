#ifndef MC_REG_DIGITAL_ORGANISM_H
#define MC_REG_DIGITAL_ORGANISM_H

#include <tuple>

// New signalgp includes
#include "hardware/SignalGP/utils/LinearFunctionsProgram.h"

// local includes
#include "mutation_utils.h"

template<typename TAG_T, typename INST_ARG_T>
class MCRegOrganism {
public:
  struct MCRegGenome;

  using program_t = sgp::LinearFunctionsProgram<TAG_T, INST_ARG_T>;
  using genome_t = MCRegGenome;

  struct MCRegGenome {
    program_t program;
    MCRegGenome(const program_t & p) : program(p) {}
    MCRegGenome(const MCRegGenome &) = default;
    MCRegGenome(MCRegGenome &&) = default;

    bool operator==(const MCRegGenome & other) const {
      return program == other.program;
    }

    bool operator!=(const MCRegGenome & other) const {
      return !(*this == other);
    }

    bool operator<(const MCRegGenome & other) const {
      return program < other.program;
    }

    const program_t & GetProgram() const { return program; }
    program_t & GetProgram() { return program; }

  };

  struct MCRegPhenotype {
    double resources_consumed=0.0;
    size_t num_unique_resp=0;
    size_t num_resp=0;
    size_t num_active_cells=0;
    // todo - add development pattern!

    void Reset() {
      resources_consumed=0.0;
      num_unique_resp=0;
      num_resp=0;
      num_active_cells=0;
    }

    bool operator==(const MCRegPhenotype & o) const {
      return std::tie(resources_consumed,
                      num_unique_resp,
                      num_resp,
                      num_active_cells) ==
             std::tie(o.resources_consumed,
                      o.num_unique_resp,
                      o.num_resp,
                      o.num_active_cells);
    }

    bool operator!=(const MCRegPhenotype & o) const {
      return !(*this == o);
    }

    bool operator<(const MCRegPhenotype & o) const {
      return std::tie(resources_consumed,
                      num_unique_resp,
                      num_resp,
                      num_active_cells) <
             std::tie(o.resources_consumed,
                      o.num_unique_resp,
                      o.num_resp,
                      o.num_active_cells);
    }

    double GetResourcesConsumed() const { return resources_consumed; }
    size_t GetUniqueResponseCnt() const { return num_unique_resp; }
    size_t GetResponseCnt() const { return num_resp; }
    size_t GetActiveCellCnt() const { return num_active_cells; }

  };

protected:
  MCRegPhenotype phenotype;
  genome_t genome;

  std::unordered_map<std::string, int> mutations; ///< Mutations from parent.

public:
  MCRegOrganism(const genome_t & g)
    : phenotype(), genome(g) {}

  MCRegOrganism(const MCRegOrganism &) = default;
  MCRegOrganism(MCRegOrganism &&) = default;

  genome_t & GetGenome() { return genome; }
  const genome_t & GetGenome() const { return genome; }

  MCRegPhenotype & GetPhenotype() { return phenotype; }
  const MCRegPhenotype & GetPhenotype() const { return phenotype; }

  std::unordered_map<std::string, int> & GetMutations() { return mutations; }
  const std::unordered_map<std::string, int> & GetMutations() const { return mutations; }

  void ResetMutations() {
    for (auto & pair : mutations) {
      pair.second = 0;
    }
  }

};

#endif