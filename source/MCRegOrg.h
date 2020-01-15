#ifndef MC_REG_DIGITAL_ORGANISM_H
#define MC_REG_DIGITAL_ORGANISM_H

#include <tuple>
#include <utility>

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
    using loc_t = std::pair<size_t, size_t>;
    double score=0.0;
    size_t num_unique_resp=0;
    size_t num_resp=0;
    size_t num_active_cells=0;
    emp::vector<size_t> response_cnts;                ///< Response counts by type.
    emp::vector< emp::vector<loc_t> > response_locs;  ///< Response locations by type
    emp::vector<double> clumpyness_ratings;           ///< By type. Higher is clumpy-er.

    void Reset(size_t response_types_cnt=1) {
      score=0.0;
      num_unique_resp=0;
      num_resp=0;
      num_active_cells=0;
      response_cnts.resize(response_types_cnt);
      response_locs.resize(response_types_cnt);
      clumpyness_ratings.resize(response_types_cnt);
      for (size_t i = 0; i < response_cnts.size(); ++i) {
        response_locs[i].clear();
        response_cnts[i] = 0;
        clumpyness_ratings[i] = 0.0;
      }
    }

    bool operator==(const MCRegPhenotype & o) const {
      return std::tie(score,
                      num_unique_resp,
                      num_resp,
                      num_active_cells,
                      response_cnts,
                      response_locs,
                      clumpyness_ratings) ==
             std::tie(o.score,
                      o.num_unique_resp,
                      o.num_resp,
                      o.num_active_cells,
                      o.response_cnts,
                      o.response_locs,
                      o.clumpyness_ratings);
    }

    bool operator!=(const MCRegPhenotype & o) const {
      return !(*this == o);
    }

    bool operator<(const MCRegPhenotype & o) const {
      return std::tie(score,
                      num_unique_resp,
                      num_resp,
                      num_active_cells,
                      response_cnts,
                      response_locs,
                      clumpyness_ratings) <
             std::tie(o.score,
                      o.num_unique_resp,
                      o.num_resp,
                      o.num_active_cells,
                      o.response_cnts,
                      o.response_locs,
                      o.clumpyness_ratings);
    }

    double GetScore() const { return score; }
    size_t GetUniqueResponseCnt() const { return num_unique_resp; }
    size_t GetResponseCnt() const { return num_resp; }
    size_t GetActiveCellCnt() const { return num_active_cells; }
    emp::vector<size_t> & GetResponsesByType() { return response_cnts; }

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