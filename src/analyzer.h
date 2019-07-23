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
public:
  Analyzer();
  ~Analyzer();
  void analyze(SlippiReplay &s, std::ostream* _dout);
};

}

#endif /* ANALYZER_H_ */
