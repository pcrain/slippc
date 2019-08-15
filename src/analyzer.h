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

  bool     get1v1Ports                (const SlippiReplay &s, Analysis *a) const;
  void     analyzeInteractions        (const SlippiReplay &s, Analysis *a) const;
  void     analyzeMoves               (const SlippiReplay &s, Analysis *a) const;
  void     analyzePunishes            (const SlippiReplay &s, Analysis *a) const;
  void     getBasicGameInfo           (const SlippiReplay &s, Analysis *a) const;
  void     summarizeInteractions      (const SlippiReplay &s, Analysis *a) const;
  void     computeAirtime             (const SlippiReplay &s, Analysis *a) const;
  void     countLCancels              (const SlippiReplay &s, Analysis *a) const;
  void     countTechs                 (const SlippiReplay &s, Analysis *a) const;
  void     countDashdances            (const SlippiReplay &s, Analysis *a) const;
  void     countAirdodgesAndWavelands (const SlippiReplay &s, Analysis *a) const;
  void     countBasicAnimations       (const SlippiReplay &s, Analysis *a) const;
  void     showActionStates           (const SlippiReplay &s, Analysis *a) const;
  unsigned countTransitions           (const SlippiReplay &s, Analysis *a, unsigned pnum, bool (*cb)(const SlippiFrame&)) const;
  unsigned countTransitions           (const SlippiReplay &s, Analysis *a, unsigned pnum, bool (*cb)(const SlippiPlayer &, const unsigned)) const;

  static inline float getHitStun(const SlippiFrame &f) {
    return f.hitstun;
  }
  static inline float playerDistance(const SlippiFrame &pf, const SlippiFrame &of) {
    float xd = pf.pos_x_pre - of.pos_x_pre;
    float yd = pf.pos_y_pre - of.pos_y_pre;
    return sqrt(xd*xd+yd*yd);
  }
  static inline bool isOffStage(const SlippiReplay &s, const SlippiFrame &f) {
    return
      f.pos_x_pre >  Stage::ledge[s.stage] ||
      f.pos_x_pre < -Stage::ledge[s.stage] ||
      f.pos_y_pre <  0;
  }
  static inline bool wasShieldStabbed(const SlippiPlayer &p, const unsigned f) {
    return p.frame[f-1].action_post >= Action::GuardOn
      && p.frame[f-1].action_post <= Action::GuardReflect
      && p.frame[f].percent_post > p.frame[f-1].percent_post;
  }
  static inline bool didEdgeCancelAerial(const SlippiFrame &f) {
    return f.action_post >= Action::Fall
      && f.action_post <= Action::FallB
      && f.action_pre >= Action::LandingAirN
      && f.action_pre <= Action::LandingAirLw;
  }
  static inline bool didEdgeCancelSpecial(const SlippiFrame &f) {
    return f.action_post >= Action::Fall
      && f.action_post <= Action::FallB
      && f.action_pre == Action::LandingFallSpecial;
  }
  static inline bool didMeteorCancel(const SlippiPlayer &p, const unsigned f) {
    return isInHitstun(p.frame[f-1])
      && (getHitStun(p.frame[f-1]) >= 2.0f)
      && (!isInHitstun(p.frame[f]))
      && (p.frame[f].action_post > Action::Wait1
       || p.frame[f].action_post == Action::JumpAerialF
       || p.frame[f].action_post == Action::JumpAerialB);
  }
  static inline unsigned deathDirection(const SlippiPlayer &p, const unsigned f) {
    if (p.frame[f].action_post == Action::DeadDown)  { return Dir::DOWN; }
    if (p.frame[f].action_post == Action::DeadLeft)  { return Dir::LEFT; }
    if (p.frame[f].action_post == Action::DeadRight) { return Dir::RIGHT; }
    if (p.frame[f].action_post <  Action::Sleep)     { return Dir::UP; }
    return Dir::NEUT;
  }
  //NOTE: the next few functions do not check for valid frame indices
  //  This is technically unsafe, but boolean shortcut logic should ensure the unsafe
  //    portions never get called.
  static inline bool maybeWavelanding(const SlippiPlayer &p, const unsigned f) {
    //Code credit to Fizzi
    return p.frame[f].action_pre == Action::LandingFallSpecial && (
      p.frame[f-1].action_pre == Action::EscapeAir || (
        p.frame[f-1].action_pre >= Action::KneeBend &&
        p.frame[f-1].action_pre <= Action::FallAerialB
        )
      );
  }
  static inline bool isDashdancing(const SlippiPlayer &p, const unsigned f) {
    //Code credit to Fizzi. This SHOULD never thrown an exception, since we
    //  should never be in turn animation before frame 2
    return (p.frame[f].action_pre   == Action::Dash)
        && (p.frame[f-1].action_pre == Action::Turn)
        && (p.frame[f-2].action_pre == Action::Dash);
  }
  static inline bool isShieldBroken(const SlippiFrame &f) {
    return f.action_pre == Action::ShieldBreakFly
      || f.action_pre == Action::ShieldBreakFall;
  }
  static inline bool isInJumpsquat(const SlippiFrame &f) {
    return f.action_pre == Action::KneeBend;
  }
  static inline bool isSpotdodging(const SlippiFrame &f) {
    return f.action_pre == Action::Escape;
  }
  static inline bool isAirdodging(const SlippiFrame &f) {
    return f.action_pre == Action::EscapeAir;
  }
  static inline bool isGrabbing(const SlippiFrame &f) {
    return (f.action_pre >= Action::CatchPull) && (f.action_pre <= Action::CatchAttack);
  }
  static inline bool isTaunting(const SlippiFrame &f) {
    return (f.action_pre == Action::AppealR) || (f.action_pre == Action::AppealL);
  }
  static inline bool isReleasing(const SlippiFrame &f) {
    return f.action_pre == Action::CatchCut;
  }
  static inline bool isRolling(const SlippiFrame &f) {
    return (f.action_pre == Action::EscapeF)|| (f.action_pre == Action::EscapeB);
  }
  static inline bool isDodging(const SlippiFrame &f) {
    return (f.action_pre >= Action::EscapeF) && (f.action_pre <= Action::Escape);
  }
  static inline bool inTumble(const SlippiFrame &f) {
    return f.action_pre == Action::DamageFall;
  }
  static inline bool inDamagedState(const SlippiFrame &f) {
    return (f.action_pre >= Action::DamageHi1) && (f.action_pre <= Action::DamageFlyRoll);
  }
  static inline bool inMissedTechState(const SlippiFrame &f) {
    return (f.action_pre >= Action::DownBoundU) && (f.action_pre <= Action::DownSpotD);
  }
  //Excludes wall techs, wall jumps, and ceiling techs
  static inline bool inFloorTechState(const SlippiFrame &f) {
    return (f.action_pre >= Action::DownBoundU) && (f.action_pre <= Action::PassiveStandB);
  }
  //Includes wall techs, wall jumps, and ceiling techs
  static inline bool inTechState(const SlippiFrame &f) {
    return (f.action_pre >= Action::DownBoundU) && (f.action_pre <= Action::PassiveCeil);
  }
  static inline bool isInShieldstun(const SlippiFrame &f) {
    return f.action_pre == Action::GuardSetOff;
  }
  static inline bool isGrabbed(const SlippiFrame &f) {
    return (f.action_pre >= Action::CapturePulledHi) && (f.action_pre <= Action::CaptureFoot);
  }
  static inline bool isThrown(const SlippiFrame &f) {
    return (f.action_pre >= Action::ThrownF) && (f.action_pre <= Action::ThrownLwWomen);
  }
  static inline bool isOnLedge(const SlippiFrame &f) {
    return f.action_pre == Action::CliffWait;
  }
  static inline bool isAirborne(const SlippiFrame &f) {
    return f.airborne;
  }
  static inline bool isInHitlag(const SlippiFrame &f) {
    return f.flags_2 & 0x20;
  }
  static inline bool isShielding(const SlippiFrame &f) {
    return f.flags_3 & 0x80;
  }
  static inline bool isInHitstun(const SlippiFrame &f) {
    return f.flags_4 & 0x02;
  }
  static inline bool isPowershielding(const SlippiFrame &f) {
    return f.flags_4 & 0x20;
  }
  static inline bool isDead(const SlippiFrame &f) {
    return (f.flags_5 & 0x10) || f.action_pre < Action::Sleep;
  }
  static inline std::string frameAsTimer(unsigned fnum) {
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
