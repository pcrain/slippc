#ifndef ANALYZER_H_
#define ANALYZER_H_

#include <iostream>
#include <fstream>
#include <math.h>    //sqrt

#include "enums.h"
#include "util.h"
#include "replay.h"
#include "analysis.h"

//Version number for the analyzer
const std::string ANALYZER_VERSION = "0.1.0";

const unsigned TIMER_MINS    = 8;     //Assuming a fixed 8 minute time for now (TODO: might need to change later)
const unsigned SHARK_THRES   = 15;    //Minimum frames to be out of hitstun before comboing becomes sharking
const unsigned POKE_THRES    = 30;    //Frames since either player entered hitstun to consider neutral a poke
const float    FOOTSIE_THRES = 10.0f; //Distance cutoff between FOOTSIES and POSITIONING dynamics

namespace slip {

class Analyzer {
private:
  std::ostream* _dout; //Debug output stream

  bool get1v1Ports                (const SlippiReplay &s, Analysis *a) const;
  void analyzeInteractions        (const SlippiReplay &s, Analysis *a) const;
  void analyzeMoves               (const SlippiReplay &s, Analysis *a) const;
  void analyzePunishes            (const SlippiReplay &s, Analysis *a) const;
  void getBasicGameInfo           (const SlippiReplay &s, Analysis *a) const;
  void summarizeInteractions      (const SlippiReplay &s, Analysis *a) const;
  void computeAirtime             (const SlippiReplay &s, Analysis *a) const;
  void countLCancels              (const SlippiReplay &s, Analysis *a) const;
  void countTechs                 (const SlippiReplay &s, Analysis *a) const;
  void countLedgegrabs            (const SlippiReplay &s, Analysis *a) const;
  void countDodges                (const SlippiReplay &s, Analysis *a) const;
  void countDashdances            (const SlippiReplay &s, Analysis *a) const;
  void countAirdodgesAndWavelands (const SlippiReplay &s, Analysis *a) const;

  inline float playerDistance(const SlippiFrame &pf, const SlippiFrame &of) const {
    float xd = pf.pos_x_pre - of.pos_x_pre;
    float yd = pf.pos_y_pre - of.pos_y_pre;
    return sqrt(xd*xd+yd*yd);
  }
  inline bool isOffStage(const SlippiReplay &s, const SlippiFrame &f) const {
    return
      f.pos_x_pre >  Stage::ledge[s.stage] ||
      f.pos_x_pre < -Stage::ledge[s.stage] ||
      f.pos_y_pre <  0;
  }
  inline unsigned deathDirection(const SlippiPlayer &p, const unsigned f) const {
    if (p.frame[f].action_post == Action::DeadDown)  { return Dir::DOWN; }
    if (p.frame[f].action_post == Action::DeadLeft)  { return Dir::LEFT; }
    if (p.frame[f].action_post == Action::DeadRight) { return Dir::RIGHT; }
    if (p.frame[f].action_post <  Action::Sleep)     { return Dir::UP; }
    return Dir::NEUT;
  }
  //NOTE: the next few functions do not check for valid frame indices
  //  This is technically unsafe, but boolean shortcut logic should ensure the unsafe
  //    portions never get called.
  inline bool maybeWavelanding(const SlippiPlayer &p, const unsigned f) const {
    //Code credit to Fizzi
    return p.frame[f].action_pre == Action::LandingFallSpecial && (
      p.frame[f-1].action_pre == Action::EscapeAir || (
        p.frame[f-1].action_pre >= Action::KneeBend &&
        p.frame[f-1].action_pre <= Action::FallAerialB
        )
      );
  }
  inline bool isDashdancing(const SlippiPlayer &p, const unsigned f) const {
    //Code credit to Fizzi. This SHOULD never thrown an exception, since we
    //  should never be in turn animation before frame 2
    return (p.frame[f].action_pre   == Action::Dash)
        && (p.frame[f-1].action_pre == Action::Turn)
        && (p.frame[f-2].action_pre == Action::Dash);
  }
  inline bool isInJumpsquat(const SlippiFrame &f) const {
    return f.action_pre == Action::KneeBend;
  }
  inline bool isSpotdodging(const SlippiFrame &f) const {
    return f.action_pre == Action::Escape;
  }
  inline bool isAirdodging(const SlippiFrame &f) const {
    return f.action_pre == Action::EscapeAir;
  }
  inline bool isDodging(const SlippiFrame &f) const {
    return (f.action_pre >= Action::EscapeF) && (f.action_pre <= Action::Escape);
  }
  inline bool inTumble(const SlippiFrame &f) const {
    return f.action_pre == Action::DamageFall;
  }
  inline bool inDamagedState(const SlippiFrame &f) const {
    return (f.action_pre >= Action::DamageHi1) && (f.action_pre <= Action::DamageFlyRoll);
  }
  inline bool inMissedTechState(const SlippiFrame &f) const {
    return (f.action_pre >= Action::DownBoundU) && (f.action_pre <= Action::DownSpotD);
  }
  //Excludes wall techs, wall jumps, and ceiling techs
  inline bool inFloorTechState(const SlippiFrame &f) const {
    return (f.action_pre >= Action::DownBoundU) && (f.action_pre <= Action::PassiveStandB);
  }
  //Includes wall techs, wall jumps, and ceiling techs
  inline bool inTechState(const SlippiFrame &f) const {
    return (f.action_pre >= Action::DownBoundU) && (f.action_pre <= Action::PassiveCeil);
  }
  inline bool isInShieldstun(const SlippiFrame &f) const {
    return f.action_pre == Action::GuardSetOff;
  }
  inline bool isGrabbed(const SlippiFrame &f) const {
    return (f.action_pre >= Action::CapturePulledHi) && (f.action_pre <= Action::CaptureFoot);
  }
  inline bool isThrown(const SlippiFrame &f) const {
    return (f.action_pre >= Action::ThrownF) && (f.action_pre <= Action::ThrownLwWomen);
  }
  inline bool isOnLedge(const SlippiFrame &f) const {
    return f.action_pre == Action::CliffWait;
  }
  inline bool isAirborne(const SlippiFrame &f) const {
    return f.airborne;
  }
  inline bool isInHitlag(const SlippiFrame &f) const {
    return f.flags_2 & 0x20;
  }
  inline bool isShielding(const SlippiFrame &f) const {
    return f.flags_3 & 0x80;
  }
  inline bool isInHitstun(const SlippiFrame &f) const {
    return f.flags_4 & 0x02;
  }
  inline bool isDead(const SlippiFrame &f) const {
    return (f.flags_5 & 0x10) || f.action_pre < Action::Sleep;
  }
  inline std::string frameAsTimer(unsigned fnum) const {
    const int TIMEFRAMES = 3600*TIMER_MINS;  //Total number of frames in an 8 minute match
    int elapsed          = fnum+LOAD_FRAME;  //Number of frames elapsed since timer started
    if (elapsed < 0) {
      return "08:00:00"; //If the timer hasn't started, just return 08:00:00
    }
    int remaining = TIMEFRAMES-elapsed;
    int lmins     = remaining/3600;
    int lsecs     = (remaining%3600)/60;
    int lframes   = remaining%60;
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
