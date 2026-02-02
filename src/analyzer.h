#ifndef ANALYZER_H_
#define ANALYZER_H_

#include <math.h>    //sqrt

#include "enums.h"
#include "replay.h"
#include "analysis.h"

//Version number for the analyzer
const unsigned TIMER_MINS    = 8;     //Assuming a fixed 8 minute time for now (TODO: might need to change later)
const unsigned SHARK_THRES   = 15;    //Minimum frames to be out of hitstun before comboing becomes sharking
const unsigned POKE_THRES    = 30;    //Frames since either player entered hitstun to consider neutral a poke
const float    FOOTSIE_THRES = 10.0f; //Distance cutoff between FOOTSIES and POSITIONING dynamics
const unsigned MAX_WAIT      = 15;    //If we don't act out of wait or stun for this many frames, we're not trying to move

namespace slip {

enum JoystickRegion {
  DZ = 0,
  NE = 1,
  SE = 2,
  SW = 3,
  NW = 4,
  N = 5,
  E = 6,
  S = 7,
  W = 8,
};

class Analyzer {
private:
  int _debug; //Current debug level

  bool     get1v1Ports                (const SlippiReplay &s, Analysis *a) const;
  void     analyzeInteractions        (const SlippiReplay &s, Analysis *a) const;
  void     analyzePunishes            (const SlippiReplay &s, Analysis *a) const;
  void     analyzeCancels             (const SlippiReplay &s, Analysis *a) const;
  void     analyzeShield              (const SlippiReplay &s, Analysis *a) const;
  void     getBasicGameInfo           (const SlippiReplay &s, Analysis *a) const;
  void     summarizeInteractions      (const SlippiReplay &s, Analysis *a) const;
  void     computeAirtime             (const SlippiReplay &s, Analysis *a) const;
  void     countLCancels              (const SlippiReplay &s, Analysis *a) const;
  void     countButtons               (const SlippiReplay &s, Analysis *a) const;
  void     countTechs                 (const SlippiReplay &s, Analysis *a) const;
  void     countDashdances            (const SlippiReplay &s, Analysis *a) const;
  void     countAirdodgesAndWavelands (const SlippiReplay &s, Analysis *a) const;
  void     countBasicAnimations       (const SlippiReplay &s, Analysis *a) const;
  void     countPhantoms              (const SlippiReplay &s, Analysis *a) const;
  void     countLedgedashes           (const SlippiReplay &s, Analysis *a) const;
  void     countActionability         (const SlippiReplay &s, Analysis *a) const;
  void     countMoves                 (const SlippiReplay &s, Analysis *a) const;
  void     showActionStates           (const SlippiReplay &s, Analysis *a) const;
  void     computeTrivialInfo         (const SlippiReplay &s, Analysis *a) const;
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
    return isAirborne(f) && (
      f.pos_x_pre >  Stage::ledge[s.stage] ||
      f.pos_x_pre < -Stage::ledge[s.stage] ||
      f.pos_y_pre <  -10.0f);  //Smaller than zero to account for ECB shenanigans
  }
  static inline bool wasHitByPhantom(const SlippiPlayer &p, const SlippiPlayer &o, const unsigned f) {
    //Phantom detection:
      //Defender is in hitlag at least 2 frames before taking damage
      //Attacker is NOT in hitlag at least 2 frames before taking damage
      //Defender is NOT in hitstun -> TODO: misses edge cases where players are hit by phantoms while in hitstun from another attack
      //Attacker is not throwing
      //Defender's damage increased since last frame
    return isInHitlag(p.frame[f-2]) &&
      (not isInHitlag(o.frame[f-2])) &&
      (not isInHitstun(p.frame[f])) &&
      (not isThrowing(o.frame[f])) &&
      p.frame[f-1].percent_pre < p.frame[f].percent_post
      ;
  }
  static inline bool wasShieldStabbed(const SlippiPlayer &p, const unsigned f) {
    return p.frame[f-1].action_post >= Action::GuardOn
      && p.frame[f-1].action_post <= Action::GuardReflect
      && p.frame[f].percent_post > p.frame[f-1].percent_post;
  }
  static inline bool wasStageSpiked(const SlippiFrame &f) {
    return (f.action_pre == Action::FlyReflectWall || f.action_pre == Action::FlyReflectCeil)
      && f.action_post <= Action::DeadUpFallHitCameraIce;
  }
  static inline bool didEdgeCancelAerial(const SlippiFrame &f) {
    return f.action_post >= Action::Fall
      && f.action_post <= Action::FallB
      && f.action_pre >= Action::LandingAirN
      && f.action_pre <= Action::LandingAirLw;
  }
  static inline bool didTeeterCancelAerial(const SlippiFrame &f) {
    return f.action_post >= Action::Ottotto
      && f.action_post <= Action::OttottoWait
      && f.action_pre >= Action::LandingAirN
      && f.action_pre <= Action::LandingAirLw;
  }
  static inline bool didAutoCancelAerial(const SlippiFrame &f) {
    return f.action_post == Action::Landing
      && f.action_pre >= Action::AttackAirN
      && f.action_pre <= Action::AttackAirLw;
  }
  static inline bool didNoImpactLand(const SlippiFrame &f) {
    return f.action_pre >= Action::JumpF
      && f.action_pre <= Action::JumpAerialB
      && f.action_post == Action::Wait;
  }
  static inline bool didShieldDrop(const SlippiFrame &f) {
    return f.action_pre >= Action::GuardOn
      && f.action_pre <= Action::GuardOff
      && f.action_post == Action::Pass;
  }
  static inline bool didEdgeCancelSpecial(const SlippiFrame &f) {
    return f.action_post >= Action::Fall
      && f.action_post <= Action::FallB
      && f.action_pre == Action::LandingFallSpecial;
  }
  static inline bool didTeeterCancelSpecial(const SlippiFrame &f) {
    return f.action_post >= Action::Ottotto
      && f.action_post <= Action::OttottoWait
      && f.action_pre == Action::LandingFallSpecial;
  }
  static inline bool didPivot(const SlippiPlayer &p, const unsigned f) {
    return p.frame[f].action_pre == Action::Turn
      && p.frame[f-1].action_pre == Action::Dash
      && p.frame[f].action_post != Action::Dash
      && (not isInHitstun(p.frame[f]))
      ;
  }
  static inline bool isJumpHeld(const SlippiPlayer &p, const unsigned f) {
    return p.frame[f].buttons & 0x0C00; //0000 1100 0000 0000
  }
  static inline bool didHop(const SlippiPlayer &p, const unsigned f) {
    return p.frame[f-1].action_post == Action::KneeBend
      && (p.frame[f].action_post == Action::JumpF || p.frame[f].action_post == Action::JumpB);
  }
  static inline bool didShortHop(const SlippiPlayer &p, const unsigned f) {
    return didHop(p,f) && (not isJumpHeld(p,f));
  }
  static inline bool didPowerShield(const SlippiPlayer &p, const unsigned f) {
    return (p.frame[f].flags_4 & 0x20) && (not(p.frame[f-1].flags_4 & 0x20));
  }
  static inline bool didMeteorCancel(const SlippiPlayer &p, const unsigned f) {
    return isInHitstun(p.frame[f-1])
      && (getHitStun(p.frame[f-1]) >= 2.0f)
      && (!isInHitstun(p.frame[f]))
      && (p.frame[f].action_post > Action::Wait1
       || p.frame[f].action_post == Action::JumpAerialF
       || p.frame[f].action_post == Action::JumpAerialB);
  }
  static inline bool didCliffCatchEnd(const SlippiPlayer &p, const unsigned f) {
    return p.frame[f-1].action_pre == Action::CliffCatch && p.frame[f].action_pre != Action::CliffCatch;
  }
  static inline bool didReleaseLedge(const SlippiPlayer &p, const unsigned f) {
    return (p.frame[f-1].action_pre == Action::CliffWait || p.frame[f-1].action_pre == Action::CliffCatch)
      && p.frame[f].action_pre == Action::Fall;
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
  static inline bool isLanding(const SlippiFrame &f) {
    return (f.action_post == Action::Landing) || (f.action_post == Action::LandingFallSpecial);
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
  static inline bool isInShield(const SlippiFrame &f) {
    return f.action_pre >= Action::GuardOn && f.action_pre <= Action::GuardReflect;
  }
  static inline bool isInShieldstun(const SlippiFrame &f) {
    return f.action_pre == Action::GuardSetOff;
  }
  static inline bool isGrabbed(const SlippiFrame &f) {
    return
      ((f.action_pre >= Action::CapturePulledHi) && (f.action_pre <= Action::CaptureFoot)) ||
      ((f.action_pre >= Action::CaptureCaptain) && (f.action_pre <= Action::ThrownKirby));
  }
  static inline bool isThrown(const SlippiFrame &f) {
    return (f.action_pre >= Action::ThrownF) && (f.action_pre <= Action::ThrownLwWomen);
  }
  static inline bool isThrowing(const SlippiFrame &f) {
    return (f.action_pre >= Action::ThrowF) && (f.action_pre <= Action::ThrowLw);
  }
  static inline bool isUsingNormalMove(const SlippiFrame &f) {
    return (f.action_pre >= Action::Attack11) && (f.action_pre <= Action::AttackAirLw);
  }
  static inline bool isUsingSpecialMove(const SlippiFrame &f, const unsigned pid) {
    for(unsigned i = 0; CharExt::special[pid][i] > 0; ++i) {
      if (f.action_pre == CharExt::special[pid][i]) {
        return true;
      }
    }
    return false;
  }
  static inline bool isUsingMiscMove(const SlippiFrame &f) {
    return
      f.action_pre == Action::DownAttackU      ||  //Getup attack up
      f.action_pre == Action::DownAttackD      ||  //Getup attack down
      f.action_pre == Action::CliffAttackSlow  ||  //Ledge attack >=100%
      f.action_pre == Action::CliffAttackQuick     //Ledge attack <100%
      ;
  }
  static inline bool isUsingGrab(const SlippiFrame &f) {
    return f.action_pre == Action::Catch;
  }
  static inline bool isUsingPummel(const SlippiFrame &f) {
    return f.action_pre == Action::CatchAttack;
  }
  static inline bool isInWait(const SlippiFrame &f) {
    return f.action_pre == Action::Wait;
  }
  static inline bool isInAnyWait(const SlippiFrame &f) {
    return f.action_pre == Action::Wait || ((f.action_pre >= Action::Wait1) && (f.action_pre <= Action::SquatWaitItem));
  }
  static inline bool isOnLedge(const SlippiFrame &f) {
    return f.action_pre == Action::CliffWait;
  }
  static inline bool didActionStateChange(const SlippiFrame &f) {
    return f.action_pre != f.action_post;
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
  static inline bool isInDamageAnimation(const SlippiFrame &f) {
    return f.action_pre >= Action::DamageHi1 && f.action_pre <= Action::DamageFlyRoll;
  }
  static inline bool isOffscreen(const SlippiFrame &f) {
    return f.flags_5 & 0x80;
  }
  static inline bool isDead(const SlippiFrame &f) {
    return (f.flags_5 & 0x10) || f.action_pre < Action::Sleep;
  }
  static inline JoystickRegion getJoystickRegion(float x, float y, float neut) {
    if (x >= neut && y >= neut) {
      return JoystickRegion::NE;
    }
    else if (x >= neut && y <= -neut) {
      return JoystickRegion::SE;
    }
    else if (x <= -neut && y <= -neut) {
      return JoystickRegion::SW;
    }
    else if (x <= -neut && y >= neut) {
      return JoystickRegion::NW;
    }
    else if (y >= neut) {
      return JoystickRegion::N;
    }
    else if (x >= neut) {
      return JoystickRegion::E;
    }
    else if (y <= -neut) {
      return JoystickRegion::S;
    }
    else if (x <= -neut) {
      return JoystickRegion::W;
    }

    return JoystickRegion::DZ;
  }
  static inline unsigned checkStickMovement(float x1, float y1, float x2, float y2, float neut=0.2875) {
    // logic copied from slippi-js
    // https://github.com/project-slippi/slippi-js/blob/master/src/common/stats/inputs.ts#L97

    auto region1 = getJoystickRegion(x1, y1, neut);
    auto region2 = getJoystickRegion(x2, y2, neut);

    return region1 != JoystickRegion::DZ && region1 != region2 ? 1 : 0;
  }
  static inline unsigned checkTriggerMovement(float trigCurrent, float trigLast, float threshold = 0.3) {
    // logic copied from slippi-js
    // https://github.com/project-slippi/slippi-js/blob/master/src/common/stats/inputs.ts#L110

    return trigLast < threshold && trigCurrent >= threshold ? 1 : 0;
  }
  static inline std::string frameAsTimer(unsigned fnum, unsigned startmins) {
    if (startmins == 0) {
      return "no timer";
    }
    int startframes = 3600*startmins; //Total number of frames in the match
    int elapsed    = fnum+LOAD_FRAME; //Number of frames elapsed since timer started
    if (elapsed < 0) {
      return std::to_string(startmins) + ":00:00";
    }
    int remaining = startframes-elapsed;
    int lmins     = remaining/3600;
    int lsecs     = (remaining%3600)/60;
    int lframes   = remaining%60;
    return std::to_string(lmins) + ":"
      + (lsecs   < 10 ? "0" : "") + std::to_string(lsecs) + ":"
      + (lframes < 6  ? "0" : "") + std::to_string(int(100*(float)lframes/60.0f));
  }
public:
  Analyzer(int debug_level);
  ~Analyzer();
  Analysis* analyze(const SlippiReplay &s);
};

}

#endif /* ANALYZER_H_ */
