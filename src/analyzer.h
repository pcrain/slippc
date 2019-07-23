#ifndef ANALYZER_H_
#define ANALYZER_H_

#include <iostream>
#include <fstream>

#include "enums.h"
#include "util.h"
#include "replay.h"

namespace slip {

class Analyzer {
private:
  std::ostream* _dout; //Debug output stream
  bool is1v1(SlippiReplay &s, uint8_t (&ports)[2]);
  void computeAirtime(SlippiReplay &s, uint8_t p);
  void computeMaxCombo(SlippiReplay &s, uint8_t p);
  void showGameHeader(SlippiReplay &s, uint8_t (&ports)[2]);
public:
  Analyzer(std::ostream* _dout);
  ~Analyzer();
  void analyze(SlippiReplay &s);
};

}

#endif /* ANALYZER_H_ */
