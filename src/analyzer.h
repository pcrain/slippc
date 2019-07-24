#ifndef ANALYZER_H_
#define ANALYZER_H_

#include <iostream>
#include <fstream>
#include <unistd.h>  //usleep
#include <math.h>    //sqrt

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
  void analyzeInteractions(SlippiReplay &s, uint8_t (&ports)[2], unsigned *all_dynamics);
  void summarizeInteractions(SlippiReplay &s, uint8_t (&ports)[2], unsigned *all_dynamics);

  void printCombo(unsigned cur_combo, uint8_t (&combo_moves)[CB_SIZE], unsigned (&combo_frames)[CB_SIZE]);

  inline std::string stateName(SlippiFrame &f) {
    return Action::name[f.action_pre];
  }

  inline float playerDistance(SlippiFrame &pf, SlippiFrame &of) {
    float xd = pf.pos_x_pre - of.pos_x_pre;
    float yd = pf.pos_y_pre - of.pos_y_pre;
    return sqrt(xd*xd+yd*yd);
  }
  inline bool inTechState(SlippiFrame &f) const {
    return (f.action_pre >= 0x00B7) && (f.action_pre <= 0x00C9);
  }
  inline bool isShielding(SlippiFrame &f) const {
    return f.flags_3 & 0x80;
  }
  inline bool isInShieldstun(SlippiFrame &f) const {
    return f.action_pre == 0x00B5;
  }
  inline bool isGrabbed(SlippiFrame &f) const {
    return (f.action_pre >= 0x00DF) && (f.action_pre <= 0x00E8);
    // return f.action_pre == 0x00E3;
  }
  inline bool isThrown(SlippiFrame &f) const {
    return (f.action_pre >= 0x00EF) && (f.action_pre <= 0x00F3);
  }
  inline bool isAirborne(SlippiFrame &f) const {
    return f.airborne;
  }
  inline bool isInHitstun(SlippiFrame &f) const {
    return f.flags_4 & 0x02;
  }
  inline bool isInHitlag(SlippiFrame &f) const {
    return f.flags_2 & 0x20;
  }
  inline bool isDead(SlippiFrame &f) const {
    return f.flags_5 & 0x10;
  }
  inline bool isOnLedge(SlippiFrame &f) const {
    return f.action_pre == 0x00FD;
  }
  inline bool isOffStage(SlippiReplay &s, SlippiFrame &f) {
    return
      f.pos_x_pre >  Stage::ledge[s.stage] ||
      f.pos_x_pre < -Stage::ledge[s.stage] ||
      f.pos_y_pre <  0
      ;
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
