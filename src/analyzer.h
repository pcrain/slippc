#ifndef ANALYZER_H_
#define ANALYZER_H_

#include <iostream>
#include <fstream>
#include <unistd.h>  //usleep
#include <math.h>    //sqrt

#include "enums.h"
#include "util.h"
#include "replay.h"
#include "analysis.h"

//Size of combo buffer
// #define CB_SIZE 255

const std::string ANALYZER_VERSION = "0.1.0";

const unsigned TIMER_MINS    = 8;      //Assuming a fixed 8 minute time for now (TODO: might need to change later)
const unsigned SHARK_THRES   = 15;     //Minimum frames to be out of hitstun before comboing becomes sharking
const unsigned POKE_THRES    = 30;     //Frames since either player entered hitstun to consider neutral a poke
const float    FOOTSIE_THRES = 10.0f;  //Distance cutoff between FOOTSIES and POSITIONING dynamics

//First actionable frame of the match (assuming frame 0 == internal frame -123)
const int      FIRST_FRAME   = START_FRAMES+PLAYABLE_FRAME;

namespace slip {

class Analyzer {
private:
  std::ostream* _dout; //Debug output stream

  //Writeout functions (arguments passed by reference may be written to)
  bool get1v1Ports                (const SlippiReplay &s, Analysis *a) const;
  void analyzeInteractions        (const SlippiReplay &s, Analysis *a) const;
  void analyzeMoves               (const SlippiReplay &s, Analysis *a) const;
  void analyzePunishes            (const SlippiReplay &s, Analysis *a) const;

  //Self-contained functions (variables assigned only within functions)
  // void findAllCombos              (const SlippiReplay &s, const uint8_t (&ports)[2], uint8_t i) const;
  // void printCombo                 (const unsigned cur_combo, const uint8_t (&combo_moves)[CB_SIZE], const unsigned (&combo_frames)[CB_SIZE]) const;
  void getBasicGameInfo           (const SlippiReplay &s, Analysis *a) const;
  void summarizeInteractions      (const SlippiReplay &s, Analysis *a) const;
  void computeAirtime             (const SlippiReplay &s, Analysis *a) const;
  void countLCancels              (const SlippiReplay &s, Analysis *a) const;
  void countTechs                 (const SlippiReplay &s, Analysis *a) const;
  void countLedgegrabs            (const SlippiReplay &s, Analysis *a) const;
  void countDodges                (const SlippiReplay &s, Analysis *a) const;
  void countDashdances            (const SlippiReplay &s, Analysis *a) const;
  void countAirdodgesAndWavelands (const SlippiReplay &s, Analysis *a) const;

  //Inline read-only convenience functions
  inline std::string stateName(const SlippiFrame &f) const {
    return Action::name[f.action_pre];
  }

  inline float playerDistance(const SlippiFrame &pf, const SlippiFrame &of) const {
    float xd = pf.pos_x_pre - of.pos_x_pre;
    float yd = pf.pos_y_pre - of.pos_y_pre;
    return sqrt(xd*xd+yd*yd);
  }

  inline unsigned deathDirection(const SlippiPlayer &p, const unsigned f) const {
    if (p.frame[f].action_post == 0x0000) { return Dir::DOWN; }
    if (p.frame[f].action_post == 0x0001) { return Dir::LEFT; }
    if (p.frame[f].action_post == 0x0002) { return Dir::RIGHT; }
    if (p.frame[f].action_post <= 0x000A) { return Dir::UP; }
    return Dir::NEUT;
  }

  //NOTE: the next few functions do not check for valid frame indices
  //  This is technically unsafe, but boolean shortcut logic should ensure the unsafe
  //    portions never get called.
  inline bool maybeWavelanding(const SlippiPlayer &p, const unsigned f) const {
    //Code credit to Fizzi
    return p.frame[f].action_pre == 0x002B && (
      p.frame[f-1].action_pre == 0x00EC ||
      (p.frame[f-1].action_pre >= 0x0018 && p.frame[f-1].action_pre <= 0x0022)
      );
  }
  inline bool isDashdancing(const SlippiPlayer &p, const unsigned f) const {
    //Code credit to Fizzi
    //This should never thrown an exception, since we should never be in turn animation
    //  before frame 2
    return (p.frame[f].action_pre   == 0x0014)
        && (p.frame[f-1].action_pre == 0x0012)
        && (p.frame[f-2].action_pre == 0x0014);
  }
  inline bool isInJumpsquat(const SlippiFrame &f) const {
    return f.action_pre == 0x0018;
  }
  inline bool isSpotdodging(const SlippiFrame &f) const {
    return f.action_pre == 0x00EB;
  }
  inline bool isAirdodging(const SlippiFrame &f) const {
    return f.action_pre == 0x00EC;
  }
  inline bool isDodging(const SlippiFrame &f) const {
    return (f.action_pre >= 0x00E9) && (f.action_pre <= 0x00EB);
  }
  inline bool inTumble(const SlippiFrame &f) const {
    return f.action_pre == 0x0026;
  }
  inline bool inDamagedState(const SlippiFrame &f) const {
    return (f.action_pre >= 0x004B) && (f.action_pre <= 0x005B);
  }
  inline bool inMissedTechState(const SlippiFrame &f) const {
    return (f.action_pre >= 0x00B7) && (f.action_pre <= 0x00C6);
  }
  //Excluding walltechs, walljumps, and ceiling techs
  inline bool inFloorTechState(const SlippiFrame &f) const {
    return (f.action_pre >= 0x00B7) && (f.action_pre <= 0x00C9);
  }
  //Including walltechs, walljumps, and ceiling techs
  inline bool inTechState(const SlippiFrame &f) const {
    return (f.action_pre >= 0x00B7) && (f.action_pre <= 0x00CC);
  }
  inline bool isShielding(const SlippiFrame &f) const {
    return f.flags_3 & 0x80;
  }
  inline bool isInShieldstun(const SlippiFrame &f) const {
    return f.action_pre == 0x00B5;
  }
  inline bool isGrabbed(const SlippiFrame &f) const {
    return (f.action_pre >= 0x00DF) && (f.action_pre <= 0x00E8);
    // return f.action_pre == 0x00E3;
  }
  inline bool isThrown(const SlippiFrame &f) const {
    return (f.action_pre >= 0x00EF) && (f.action_pre <= 0x00F3);
  }
  inline bool isAirborne(const SlippiFrame &f) const {
    return f.airborne;
  }
  inline bool isInHitstun(const SlippiFrame &f) const {
    return f.flags_4 & 0x02;
  }
  inline bool isInHitlag(const SlippiFrame &f) const {
    return f.flags_2 & 0x20;
  }
  inline bool isDead(const SlippiFrame &f) const {
    return (f.flags_5 & 0x10) || f.action_pre <= 0x000A;
  }
  inline bool isOnLedge(const SlippiFrame &f) const {
    return f.action_pre == 0x00FD;
  }
  inline bool isOffStage(const SlippiReplay &s, const SlippiFrame &f) const {
    return
      f.pos_x_pre >  Stage::ledge[s.stage] ||
      f.pos_x_pre < -Stage::ledge[s.stage] ||
      f.pos_y_pre <  0
      ;
  }

  //TODO: can be fairly easily optimized by changing elapsed frames at the beginning to frames left
  inline std::string frameAsTimer(unsigned fnum) const {
    int elapsed  = fnum-START_FRAMES;
    elapsed      = (elapsed < 0) ? 0 : elapsed;
    int mins     = elapsed/3600;
    int secs     = (elapsed/60)-(mins*60);
    int frames   = elapsed-(60*secs)-(3600*mins);

    //Convert from elapsed to left
    int lmins = TIMER_MINS-mins;
    if (secs > 0 || frames > 0) {
      lmins -= 1;
    }
    int lsecs = 60-secs;
    if (frames > 0) {
      lsecs -= 1;
    }
    int lframes = (frames > 0) ? 60-frames : 0;

    return "0" + std::to_string(lmins) + ":"
      + (lsecs   < 10 ? "0" : "") + std::to_string(lsecs) + ":"
      + (lframes < 6  ? "0" : "") + std::to_string(int(100*(float)lframes/60.0f));
  }
public:
  Analyzer(std::ostream* dout);
  ~Analyzer();
  Analysis* analyze(const SlippiReplay &s);
};

}

#endif /* ANALYZER_H_ */
