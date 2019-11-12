#ifndef ALT_SIGNAL_DIGITAL_ORGANISM_H
#define ALT_SIGNAL_DIGITAL_ORGANISM_H

struct AltSignalPhenotype {
  double resources_consumed=0.0;
};

struct AltSignalGenome {

};

class AltSignalOrganism {
public:


  // using hardware_t = ;
  // using program_t = ;

protected:
  AltSignalPhenotype phenotype;
  AltSignalGenome genome;

public:

};

#endif