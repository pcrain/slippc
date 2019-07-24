#include "analyzer.h"

#define PORT_UNUSED 99

namespace slip {

Analyzer::Analyzer(std::ostream* _dout) {
  this->_dout = _dout;
  //Nothing to do yet
}

Analyzer::~Analyzer() {
  //Nothing to do yet
}

bool Analyzer::is1v1(SlippiReplay &s, uint8_t (&ports)[2]) {
  unsigned num_players = 0;
  for(uint8_t i = 0 ; i < 4; ++i) {
    if (s.player[i].player_type != 3) {
      if (num_players == 2) {
        return false;
      }
      ports[num_players++] = i;
    }
    if (i == 3 && ports[1] == 99) {
      return false;
    }
  }
  (*_dout)
    << "Players found on ports "
    << ports[0] << " and "
    << ports[1] << std::endl
    ;
  return true;
}

void Analyzer::computeAirtime(SlippiReplay &s, uint8_t p) {
    unsigned airframes = 0;
    for(unsigned f = START_FRAMES; f < s.frame_count; ++f) {
      airframes += s.player[p].frame[f].airborne;
    }

    float aircount = 100*((float)airframes / s.last_frame);
    std::cout << "  Airborne for " << aircount  << "% of match" << std::endl;
}

void Analyzer::showGameHeader(SlippiReplay &s, uint8_t (&ports)[2]) const {
  std::cout
    << CharInt::name[s.player[ports[0]].ext_char_id]
    << " ("
    << std::to_string(s.player[ports[0]].frame[s.frame_count-1].stocks)
    << ") vs "
    << CharInt::name[s.player[ports[1]].ext_char_id]
    << " ("
    << std::to_string(s.player[ports[1]].frame[s.frame_count-1].stocks)
    << ") on "
    << Stage::name[s.stage]
    << std::endl
    ;
}

void Analyzer::analyze(SlippiReplay &s) {
  (*_dout) << "Analyzing replay\n----------\n" << std::endl;

  //Verify this is a 1 v 1 match; can't analyze otherwise
  uint8_t ports[2] = {PORT_UNUSED};  //Port indices of p1 / p2
  if (not is1v1(s,ports)) {
    std::cerr << "  Not a two player match; refusing to analyze further" << std::endl;
    return;
  }

  showGameHeader(s,ports);

  std::string chars[2] = { //Character names
    CharInt::name[s.player[ports[0]].ext_char_id],
    CharInt::name[s.player[ports[1]].ext_char_id]
  };

  //Per player stats
  // for(unsigned i = 0; i < 2 ; ++i) {
  //   std::cout << "Player " << (i+1) << " (" << chars[i] << "):" << std::endl;
  //   computeAirtime(s,ports[i]);
  //   computeMaxCombo(s,ports[i]);
  //   findAllCombos(s,ports,i);
  // }

  //Both-player stats
  analyzeInteractions(s,ports);

  return;
}

void Analyzer::computeMaxCombo(SlippiReplay &s, uint8_t p) {
    unsigned max_combo = 0;
    for(unsigned f = START_FRAMES+PLAYABLE_FRAME; f < s.frame_count; ++f) {
      uint8_t combo = s.player[p].frame[f].combo;
      max_combo = (combo > max_combo) ? combo : max_combo;
    }

    std::cout << "  Max combo of " << max_combo << " hits" << std::endl;
}

void Analyzer::printCombo(unsigned cur_combo, uint8_t (&combo_moves)[CB_SIZE], unsigned (&combo_frames)[CB_SIZE]) {
  bool allsame = true;
  for(unsigned i = 1; i < cur_combo; ++i) {
    if(combo_moves[i] != combo_moves[i-1]) {
      allsame = false;
      break;
    }
  }
  if (!allsame) {
    std::cout << "    Hit with " << cur_combo << " hit combo" << std::endl;
    for(unsigned i = 0; i < cur_combo; ++i) {
      std::cout << "      "
        << Move::name[combo_moves[i]] << " on frame "
        << combo_frames[i] << "(" << frameAsTimer(combo_frames[i]) << ")" << std::endl;
    }
  }
}

void Analyzer::findAllCombos(SlippiReplay &s, uint8_t (&ports)[2], uint8_t i) {
  uint8_t p = ports[i], o = ports[1-i]; //Port indices for player / opponent
  SlippiFrame *pf, *of;  //Frame data for player / opponent
  std::cout << "  Showing all combos" << std::endl;
  bool     new_combo             = true;  //Whether we've officially started a new combo
  unsigned combo                 = 0;     //Internal combo count for the current frame
  unsigned last_combo            = 0;     //Internal combo count for last frame
  unsigned cur_combo             = 0;     //Current semi-official combo cound
  unsigned combo_frames[CB_SIZE] = {0};
  uint8_t  combo_moves[CB_SIZE]  = {0};

  for(unsigned f = START_FRAMES+PLAYABLE_FRAME; f <= s.frame_count; ++f) {
    last_combo = combo;
    pf         = &(s.player[p].frame[f]);
    of         = &(s.player[o].frame[f]);
    if (isInHitstun(*of) || isInHitlag(*of)) {
      combo     = pf->combo;
      new_combo = (last_combo == 0);
      if (new_combo) {
        if (cur_combo > 2) {
          printCombo(cur_combo,combo_moves,combo_frames);
        }
        cur_combo = 0;
      }
      if (combo == cur_combo+1) {
        combo_frames[cur_combo] = f;
        combo_moves[cur_combo]  = pf->hit_with;
        ++cur_combo;
      }
    } else if (isDead(*of)) {
      if (cur_combo > 2) {
        printCombo(cur_combo,combo_moves,combo_frames);
      }
      cur_combo = 0;
    } else {
      combo = 0;
    }
    // std::cout << combo;
  }
  if (cur_combo > 2) {
    printCombo(cur_combo,combo_moves,combo_frames);
  }
}

void Analyzer::analyzeInteractions(SlippiReplay &s, uint8_t (&ports)[2]) {
  std::cout << "  Analyzing player interactions" << std::endl;
  SlippiPlayer *p                      = &(s.player[ports[0]]);
  SlippiPlayer *o                      = &(s.player[ports[1]]);

  const unsigned SHARK_THRES           = 15;    //Minimum frames to be out of hitstun before comboing becomes sharking
  const unsigned POKE_THRES            = 30;    //Frames since either player entered hitstun to consider neutral a poke
  const float    FOOTSIE_THRES         = 10.0f;  //Distance cutoff between FOOTSIES and POSITIONING dynamics

  unsigned all_dynamics[s.frame_count] = {0};                  // Dynamic active during each frame
  unsigned dyn_counts[Dynamic::__LAST] = {0};                  // Number of total frames each dynamic has been observed
  unsigned cur_dynamic                 = Dynamic::POSITIONING; // Current dynamic in effect

  unsigned oLastInHitsun               = 0;  //Last frame opponent was stuck in hitstun
  unsigned pLastInHitsun               = 0;  //Last frame we were stuck in hitstun
  unsigned oLastHitsunFC               = 0;  //Last count for our opponent's' hitstun
  unsigned pLastHitsunFC               = 0;  //Last count for our hitstun
  unsigned oLastGrounded               = 0;  //Last frame opponent touched solid ground
  unsigned pLastGrounded               = 0;  //Last frame we touched solid ground

  //All interactions analyzed from perspective of p (lower port player)
  for (unsigned f = START_FRAMES+PLAYABLE_FRAME; f < s.frame_count; ++f) {
    // std::cout << "Analyzing frame " << int(f)-START_FRAMES << std::endl;

    //Other variables
    SlippiFrame of     = o->frame[f];
    SlippiFrame pf     = p->frame[f];
    bool oHitThisFrame = false;
    bool oInHitstun    = isInHitstun(of);
      if (oInHitstun)    {
        oLastInHitsun = f;
        if (of.hitstun >= oLastHitsunFC) {
          oHitThisFrame = true; //Check if opponent was hit this frame by looking at hitstun frames left
        }
        oLastHitsunFC = of.hitstun;
      }
    bool pHitThisFrame = false;
    bool pInHitstun    = isInHitstun(pf);
      if (pInHitstun)    {
        pLastInHitsun = f;
        if (pf.hitstun >= pLastHitsunFC) {
          pHitThisFrame = true; //Check if we were hit this frame by looking at hitstun frames left
        }
        pLastHitsunFC = of.hitstun;
      }
    bool oAirborne     = isAirborne(of);
      if (not oAirborne) { oLastGrounded = f; }
    bool pAirborne     = isAirborne(pf);
      if (not pAirborne) { pLastGrounded = f; }
    bool oIsGrabbed    = isGrabbed(of);
    bool pIsGrabbed    = isGrabbed(pf);
    bool oOnLedge      = isOnLedge(of);
    bool pOnLedge      = isOnLedge(pf);
    bool oShielding    = isShielding(of);
    bool pShielding    = isShielding(pf);
    bool oInShieldstun = isInShieldstun(of);
    bool pInShieldstun = isInShieldstun(pf);
    bool oTeching      = inTechState(of);
    bool pTeching      = inTechState(pf);
    bool oThrown       = isThrown(of);
    bool pThrown       = isThrown(pf);
    bool oHitOffStage  = isOffStage(s,of) && oInHitstun;  //Check if opponent is in hitstun offstage
    bool pHitOffStage  = isOffStage(s,pf) && pInHitstun;  //Check if we are in hitstun offstage
    bool oPoked        = ((f - oLastInHitsun) < POKE_THRES);
    bool pPoked        = ((f - pLastInHitsun) < POKE_THRES);

    //If we're airborne and haven't touched the ground since last entering hitstun, we're being punished
    bool oBeingPunished = oAirborne && (oLastGrounded < oLastInHitsun);
    bool pBeingPunished = pAirborne && (pLastGrounded < pLastInHitsun);

    //If we also haven't been in hitstun for at least SHARK_THRES frames, we're being sharked
    bool oBeingSharked = oBeingPunished && ((f - oLastInHitsun) > SHARK_THRES);
    bool pBeingSharked = pBeingPunished && ((f - pLastInHitsun) > SHARK_THRES);

    //Until proven otherwise, we have the same dynamic as before
    unsigned frame_dynamic = cur_dynamic;

    //Determine whether neutral would be considered footsies, positioning, or poking
    unsigned neut_dyn = ( oPoked || pPoked ) ? Dynamic::POKING
      : ((playerDistance(pf,of) > FOOTSIE_THRES) ? Dynamic::POSITIONING : Dynamic::FOOTSIES);

    //[Define offstage as: beyond the stage's ledges OR below the stage's base (i.e., y < 0)]
    //Check for edgeguard situations - you are edgeguarding if ALL of the following conditions hold true:
    //  Your opponent has experienced hitstun while offstage since the last time they touched the ground
    //  Since the above occurred...
    //    ...your opponent has not touched the ground and been actionable (e.g., end of Sheik's recovery not actionable)
    //    ...your opponent has not grabbed ledge

    //First few checks are largely agnostic to cur_dynamic
    if (oHitOffStage) {  //If the opponent is offstage and in hitstun, they're being edgeguarded, period
      frame_dynamic = Dynamic::EDGEGUARDING;
    } else if (pHitOffStage) {  //If we're offstage and in hitstun, we're being edgeguarded, period
      frame_dynamic = Dynamic::RECOVERING;
    } else if (oIsGrabbed) {  //If we grab the opponent in neutral, it's pressure; on offense, it's a techchase
      if (cur_dynamic != Dynamic::PRESSURING) {  //Need this to prevent grabs from overwriting themselves
        frame_dynamic = (cur_dynamic >= Dynamic::OFFENSIVE) ? Dynamic::TECHCHASING : Dynamic::PRESSURING;
      }
    } else if (pIsGrabbed) {  //If we are grabbed by the opponent in neutral, we're pressured; on defense, we're techchased
      if (cur_dynamic != Dynamic::PRESSURED) {  //Need this to prevent grabs from overwriting themselves
        frame_dynamic = (cur_dynamic <= Dynamic::DEFENSIVE) ? Dynamic::ESCAPING : Dynamic::PRESSURED;
      }
    } else if (oInShieldstun) {  //If our opponent is in shieldstun, we are pressureing
      frame_dynamic = Dynamic::PRESSURING;
    } else if (pInShieldstun) {  //If we are in shieldstun, we are pressured
      frame_dynamic = Dynamic::PRESSURED;
    } else if (oTeching) {  //If we are in shieldstun, we are pressured
      frame_dynamic = Dynamic::TECHCHASING;
    } else if (pTeching) {  //If we are in shieldstun, we are pressured
      frame_dynamic = Dynamic::ESCAPING;
    } else if (oInHitstun && pInHitstun) {  //If we're both in hitstun and neither of us are offstage, it's a trade
      frame_dynamic = Dynamic::TRADING;
    } else {  //Everything else depends on the cur_dynamic
      switch (cur_dynamic) {
        case Dynamic::PRESSURING:
          // std::cout << stateName(of) << std::endl;
          if (oThrown) {  //If the opponent is being thrown, this is a techchase opportunity
            frame_dynamic = Dynamic::TECHCHASING;
          } else if ((not oShielding) && (not oOnLedge) && (not oAirborne)) {  //If the opponent touches the ground w/o shielding, we're back to neutral
            frame_dynamic = neut_dyn;
          }
          break;
        case Dynamic::PRESSURED:
          // std::cout << stateName(of) << std::endl;
          if (pThrown) {  //If we are being thrown, opponent has a techchase opportunity
            frame_dynamic = Dynamic::ESCAPING;
          } else if ((not pShielding) && (not pOnLedge) && (not pAirborne)) {  //If we touch the ground w/o shielding, we're back to neutral
            frame_dynamic = neut_dyn;
          }
          break;
        case Dynamic::EDGEGUARDING:
          if (oOnLedge || (not oAirborne)) {  //If they make it back, we're back to neutral
            frame_dynamic = neut_dyn;
          }
          break;
        case Dynamic::RECOVERING:
          if (pOnLedge || (not pAirborne)) {  //If we make it back, we're back to neutral
            frame_dynamic = neut_dyn;
          }
          // frame_dynamic = Dynamic::NEUTRAL;
          break;
        case Dynamic::TECHCHASING:
          if ((not oInHitstun) && (not oIsGrabbed) && (not oThrown) && (not oTeching) && (not oShielding)) {
            frame_dynamic = neut_dyn;  //If the opponent escaped our tech chase, back to neutral it is
          }
          break;
        case Dynamic::ESCAPING:
          if ((not pInHitstun) && (not pIsGrabbed) && (not pThrown) && (not pTeching) && (not pShielding)) {
            frame_dynamic = neut_dyn;  //If we escaped our opponent's tech chase, back to neutral it is
          }
          break;
        case Dynamic::PUNISHING:
          if (oBeingSharked) {  //If we let too much hitstun elapse, but our opponent is still airborne, we're sharking
            frame_dynamic = Dynamic::SHARKING;
          } else if (not oAirborne) {  //If our opponent has outright landed, we're back to neutral
            frame_dynamic = neut_dyn;
          }
          break;
        case Dynamic::PUNISHED:
          if (pBeingSharked) {  //If we survive punishes long enough, but are still airborne, we're being sharked
            frame_dynamic = Dynamic::GROUNDING;
          } else if (not pAirborne) {  //If we have outright landed, we're back to neutral
            frame_dynamic = neut_dyn;
          }
          break;
        case Dynamic::SHARKING:
          if (not oAirborne) {  //If our opponent has outright landed, we're back to neutral
            frame_dynamic = neut_dyn;
          }
          break;
        case Dynamic::GROUNDING:
          if (not pAirborne) {  //If we have outright landed, we're back to neutral
            frame_dynamic = neut_dyn;
          }
          break;
        case Dynamic::POKING:
          // if (oHitThisFrame || pHitThisFrame) {
          //   std::cout << "YAY";
          // }
          if (oPoked && oAirborne && oHitThisFrame) {
            frame_dynamic = Dynamic::PUNISHING; //If we have poked our opponent recently and hit them again, we're punishing
          } else if (pPoked && pAirborne && pHitThisFrame) {
            frame_dynamic = Dynamic::PUNISHED; //If we have been poked recently and got hit again, we're being punished
          } else {
            frame_dynamic = neut_dyn;  //If we were trading before, we're in neutral now that we're no longer trading
          }
          break;
        case Dynamic::POSITIONING:
        case Dynamic::FOOTSIES:
        case Dynamic::TRADING:
          frame_dynamic = neut_dyn;  //If we were trading before, we're in neutral now that we're no longer trading
          break;
        default:
          break;
      }
    }

    //Aggregate results
    cur_dynamic = frame_dynamic;
    all_dynamics[f] = cur_dynamic;  //Set the dynamic for this frame to the current dynamic computed
    ++dyn_counts[cur_dynamic];      //Increase the counter for dynamics across all frames
  }

  std::cout << "    From the perspective of port " << int(ports[0]+1) << " (" << CharInt::name[s.player[ports[0]].ext_char_id] << "):" << std::endl;
  std::cout << std::fixed; //Show floats in fixed representation
  for (unsigned i = 0; i < Dynamic::__LAST; ++i) {
    if (dyn_counts[i] == 0) {
      continue;
    }
    std::string n   = std::to_string(dyn_counts[i]);
    std::string pct = std::to_string((100*float(dyn_counts[i])/s.frame_count)).substr(0,5);
    std::cout << "      " << SPACE[6-n.length()] << n << " frames (" << pct
      << "% of game) spent in " << Dynamic::name[i] << std::endl;
  }

}

}
