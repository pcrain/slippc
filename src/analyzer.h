#ifndef ANALYZER_H_
#define ANALYZER_H_

#include <iostream>
#include <fstream>

#include "enums.h"
#include "util.h"
#include "replay.h"

//Size of combo buffer
#define CB_SIZE 255

namespace slip {

class Analyzer {
private:
  std::ostream* _dout; //Debug output stream
  bool is1v1(SlippiReplay &s, uint8_t (&ports)[2]);
  void computeAirtime(SlippiReplay &s, uint8_t p);
  void computeMaxCombo(SlippiReplay &s, uint8_t p);
  void findAllCombos(SlippiReplay &s, uint8_t (&ports)[2], uint8_t i);
  void showGameHeader(SlippiReplay &s, uint8_t (&ports)[2]) const;
  void analyzeInteractions(SlippiReplay &s, uint8_t (&ports)[2]);

  void printCombo(unsigned cur_combo, uint8_t (&combo_moves)[CB_SIZE], unsigned (&combo_frames)[CB_SIZE]);

  inline bool isInHitstun(SlippiFrame *f) const {
    return f->flags_4 & 0x02;
  }
  inline bool isInHitlag(SlippiFrame *f) const {
    return f->flags_2 & 0x20;
  }
  inline bool isDead(SlippiFrame *f) const {
    return f->flags_5 & 0x10;
  }

  inline std::string frameAsTimer(unsigned fnum) const {
    int elapsed  = fnum-START_FRAMES;
    elapsed      = (elapsed < 0) ? 0 : elapsed;
    int secs     = elapsed/60;
    int frames   = elapsed-(60*secs);
    int mins     = secs/60;
    secs        -= (60*mins);

    //Convert from elapsed to left
    int lmins = 8-mins;
    if (secs > 0 || frames > 0) {
      lmins -= 1;
    }
    int lsecs = 60-secs;
    if (frames > 0) {
      lsecs -= 1;
    }
    int lframes = 60-frames;

    return "0" + std::to_string(lmins) + ":"
      + (lsecs   < 10 ? "0" : "") + std::to_string(lsecs) + ":"
      + (lframes < 6  ? "0" : "") + std::to_string(int(100*(float)lframes/60.0f));
  }
public:
  Analyzer(std::ostream* _dout);
  ~Analyzer();
  void analyze(SlippiReplay &s);
};

}

#endif /* ANALYZER_H_ */
