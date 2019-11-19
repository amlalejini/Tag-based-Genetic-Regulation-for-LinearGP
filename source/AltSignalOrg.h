#ifndef ALT_SIGNAL_DIGITAL_ORGANISM_H
#define ALT_SIGNAL_DIGITAL_ORGANISM_H




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
  };

protected:
  AltSignalPhenotype phenotype;
  genome_t genome;

public:
  AltSignalOrganism(const genome_t & g)
    : phenotype(), genome(g) {}

  AltSignalOrganism(const AltSignalOrganism &) = default;
  AltSignalOrganism(AltSignalOrganism &&) = default;

  genome_t & GetGenome() { return genome; }
  const genome_t & GetGenome() const { return genome; }

  AltSignalPhenotype & GetPhenotype() { return phenotype; }
  const AltSignalPhenotype & GetPhenotype() const { return phenotype; }
};

#endif