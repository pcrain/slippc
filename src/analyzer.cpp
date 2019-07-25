#include "analyzer.h"

//Number representing a port is unused
#define PORT_UNUSED 99

namespace slip {

Analyzer::Analyzer(std::ostream* dout) {
  _dout = dout;
  //Nothing to do yet
}

Analyzer::~Analyzer() {
  //Nothing to do yet
}

bool Analyzer::get1v1Ports(const SlippiReplay &s, uint8_t (&ports)[2]) const {
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

void Analyzer::computeAirtime(const SlippiReplay &s, uint8_t port) const {
    SlippiPlayer p = s.player[port];
    unsigned airframes = 0;
    for(unsigned f = START_FRAMES; f < s.frame_count; ++f) {
      airframes += p.frame[f].airborne;
    }

    float aircount = 100*((float)airframes / s.last_frame);
    std::cout << "  Airborne for " << aircount  << "% of match" << std::endl;
}

void Analyzer::showGameHeader(const SlippiReplay &s, const uint8_t (&ports)[2]) const {
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

void Analyzer::computeMaxCombo(const SlippiReplay &s, uint8_t p) const {
    unsigned max_combo = 0;
    for(unsigned f = START_FRAMES+PLAYABLE_FRAME; f < s.frame_count; ++f) {
      uint8_t combo = s.player[p].frame[f].combo;
      max_combo = (combo > max_combo) ? combo : max_combo;
    }

    std::cout << "  Max combo of " << max_combo << " hits" << std::endl;
}

void Analyzer::printCombo(unsigned cur_combo, const uint8_t (&combo_moves)[CB_SIZE], const unsigned (&combo_frames)[CB_SIZE]) const {
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

void Analyzer::findAllCombos(const SlippiReplay &s, const uint8_t (&ports)[2], uint8_t i) const {
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

void Analyzer::printInteractions(const SlippiReplay &s, const uint8_t (&ports)[2], const unsigned *all_dynamics) const {
  std::cout << "  Summarizing player interactions" << std::endl;
  unsigned dyn_counts[Dynamic::__LAST] = {0}; // Number of total frames each dynamic has been observed
  for (unsigned f = START_FRAMES+PLAYABLE_FRAME; f < s.frame_count; ++f) {
    ++dyn_counts[all_dynamics[f]]; //Increase the counter for dynamics across all frames
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

void Analyzer::countLCancels(const SlippiReplay &s, uint8_t port) const {
    SlippiPlayer p        = s.player[port];
    unsigned cancels_hit  = 0;
    unsigned cancels_miss = 0;
    unsigned last_state   = 0;
    for(unsigned f = START_FRAMES; f < s.frame_count; ++f) {
      if (last_state == 0) {
        if (p.frame[f].l_cancel == 1) {
          cancels_hit += 1;
        } else if (p.frame[f].l_cancel == 2) {
          cancels_miss += 1;
        }
      }
      last_state = p.frame[f].l_cancel;
    }

    float cancels_pct = 100*((float)cancels_hit / (cancels_hit+cancels_miss));
    std::cout << "  Hit " << cancels_hit << "/" << cancels_hit+cancels_miss << " l cancels (" << cancels_pct << "%)" << std::endl;
}

void Analyzer::countTechs(const SlippiReplay &s, uint8_t port) const {
    SlippiPlayer p             = s.player[port];
    unsigned techs_hit         = 0;
    unsigned walltechs_hit     = 0;  //And ceiling techs
    unsigned walljumps_hit     = 0;
    unsigned walljumptechs_hit = 0;
    unsigned techs_missed      = 0;
    bool     teching           = false;

    for(unsigned f = START_FRAMES; f < s.frame_count; ++f) {
      if (inTechState(p.frame[f])) {
        if (not teching) {
          // std::cout << "wasin: " << stateName(p.frame[f-1]) << " -> " << stateName(p.frame[f]) << std::endl;
          teching = true;
          if (p.frame[f].action_pre <= 0x00C6) {
            ++techs_missed;
          } else if (p.frame[f].action_pre <= 0x00C9) {
            ++techs_hit;
          } else if (p.frame[f].action_pre == 0x00CB) {
            if (inDamagedState(p.frame[f-1]) || inTumble(p.frame[f-1])) {
              ++walljumptechs_hit;
            } else {
              ++walljumps_hit;
            }
          } else {
            ++walltechs_hit; //Or ceiling techs
          }
        }
      } else {
        teching = false;
      }
    }

    std::cout << "  Hit "    << techs_hit         << " grounded techs"  << std::endl;
    std::cout << "  Hit "    << walltechs_hit     << " wall techs"      << std::endl;
    std::cout << "  Hit "    << walljumps_hit     << " wall jumps"      << std::endl;
    std::cout << "  Hit "    << walljumptechs_hit << " wall jump techs" << std::endl;
    std::cout << "  Missed " << techs_missed      << " techs"           << std::endl;
}

void Analyzer::countDashdances(const SlippiReplay &s, uint8_t port) const {
    SlippiPlayer p = s.player[port];
    unsigned dashdances = 0;
    for(unsigned f = START_FRAMES; f < s.frame_count; ++f) {
      if (isDashdancing(p,f)) {
        ++dashdances;
      }
    }

    std::cout << "  Dashdanced " << dashdances  << " times" << std::endl;
}

void Analyzer::countLedgegrabs(const SlippiReplay &s, uint8_t port) const {
    SlippiPlayer p = s.player[port];
    unsigned ledgegrabs = 0;
    bool onledge = false;
    for(unsigned f = START_FRAMES; f < s.frame_count; ++f) {
      if (isOnLedge(p.frame[f])) {
        if(not onledge) {
          ++ledgegrabs;
          onledge = true;
        }
      } else {
        onledge = false;
      }
    }

    std::cout << "  Grabbed the ledge " << ledgegrabs  << " times" << std::endl;
}

//Airdodges counted separately when looking out wavedashes
void Analyzer::countDodges(const SlippiReplay &s, uint8_t port) const {
  SlippiPlayer p = s.player[port];
  unsigned rolls = 0;
  unsigned dodges = 0;
  bool dodging = false;
  for(unsigned f = START_FRAMES; f < s.frame_count; ++f) {
    if (isDodging(p.frame[f])) {
      if(not dodging) {
        if (isSpotdodging(p.frame[f])) {
          ++dodges;
        } else {
          ++rolls;
        }
        dodging = true;
      }
    } else {
      dodging = false;
    }
  }

  std::cout << "  Rolled " << rolls  << " times" << std::endl;
  std::cout << "  Spotdodged " << dodges  << " times" << std::endl;
}

//Code adapted from Fizzi's
void Analyzer::countAirdodgesAndWavelands(const SlippiReplay &s, uint8_t port) const {
  SlippiPlayer p      = s.player[port];
  unsigned airdodges  = 0;
  unsigned wavelands  = 0;
  unsigned wavedashes = 0;
  bool     airdodging = false;
  for(unsigned f = START_FRAMES; f < s.frame_count; ++f) {
    if (maybeWavelanding(p,f)) {  //Waveland detection by Fizzi
      //Look at the last 8 frames (including this one) re Fizzi
      bool foundAirdodge  = false;
      bool foundJumpsquat = false;
      bool foundOther     = false;
      for(unsigned i = 1; i < 8; ++i) {
        if (isAirdodging(p.frame[f-i])) {
          foundAirdodge = true;
        } else if (isInJumpsquat(p.frame[f-i])) {
          foundJumpsquat = true;
          break;
        } else {
          foundOther = true;
        }
      }
      if (foundAirdodge) {
        if ((not foundJumpsquat) && (not foundOther)) {
          continue; //If the airdodge is the only animation we found, proceed
        } else {
          --airdodges;  //Otherwise, subtract it from our airdodge count and keep checkings
        }
      }
      if (foundJumpsquat) {  //If we were in jumpsquat at all recently, we're wavedashing
        ++wavedashes;
      } else if (foundOther) {  //If we were in any other animation besides airdodge, it's a waveland
        ++wavelands;
      }  //Otherwise, nothing special happened
    } else if (isAirdodging(p.frame[f])) {
      if (not airdodging) {
        ++airdodges;
        airdodging = true;
      }
    } else {
      airdodging = false;
    }
  }

  std::cout << "  Airdodged "  << airdodges   << " times" << std::endl;
  std::cout << "  Wavelanded " << wavelands   << " times" << std::endl;
  std::cout << "  Wavedashed " << wavedashes  << " times" << std::endl;
}

void Analyzer::analyzeInteractions(const SlippiReplay &s, const uint8_t (&ports)[2], unsigned *all_dynamics) {
  std::cout << "  Analyzing player interactions" << std::endl;
  const SlippiPlayer *p                = &(s.player[ports[0]]);
  const SlippiPlayer *o                = &(s.player[ports[1]]);

  const unsigned SHARK_THRES           = 15;    //Minimum frames to be out of hitstun before comboing becomes sharking
  const unsigned POKE_THRES            = 30;    //Frames since either player entered hitstun to consider neutral a poke
  const float    FOOTSIE_THRES         = 10.0f;  //Distance cutoff between FOOTSIES and POSITIONING dynamics

  unsigned cur_dynamic                 = Dynamic::POSITIONING; // Current dynamic in effect

  unsigned oLastInHitsun               = 0;  //Last frame opponent was stuck in hitstun
  unsigned pLastInHitsun               = 0;  //Last frame we were stuck in hitstun
  unsigned oLastHitsunFC               = 0;  //Last count for our opponent's' hitstun
  unsigned pLastHitsunFC               = 0;  //Last count for our hitstun
  unsigned oLastGrounded               = 0;  //Last frame opponent touched solid ground
  unsigned pLastGrounded               = 0;  //Last frame we touched solid ground

  //All interactions analyzed from perspective of p (lower port player)
  for (unsigned f = START_FRAMES+PLAYABLE_FRAME; f < s.frame_count; ++f) {
    //Important variables for each frame
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
    bool oTeching      = inFloorTechState(of);
    bool pTeching      = inFloorTechState(pf);
    bool oThrown       = isThrown(of);
    bool pThrown       = isThrown(pf);
    bool oHitOffStage  = isOffStage(s,of) && oInHitstun;  //Check if opponent is in hitstun offstage
    bool pHitOffStage  = isOffStage(s,pf) && pInHitstun;  //Check if we are in hitstun offstage
    bool oPoked        = ((f - oLastInHitsun) < POKE_THRES);  //Check if opponent has been poked recently
    bool pPoked        = ((f - pLastInHitsun) < POKE_THRES);  //Check if we have been poked recently

    //If we're airborne and haven't touched the ground since last entering hitstun, we're being punished
    bool oBeingPunished = oAirborne && (oLastGrounded < oLastInHitsun);
    bool pBeingPunished = pAirborne && (pLastGrounded < pLastInHitsun);

    //If we also haven't been in hitstun for at least SHARK_THRES frames, we're being sharked
    bool oBeingSharked = oBeingPunished && ((f - oLastInHitsun) > SHARK_THRES);
    bool pBeingSharked = pBeingPunished && ((f - pLastInHitsun) > SHARK_THRES);

    //Determine whether neutral would be considered footsies, positioning, or poking
    unsigned neut_dyn = ( oPoked || pPoked ) ? Dynamic::POKING
      : ((playerDistance(pf,of) > FOOTSIE_THRES) ? Dynamic::POSITIONING : Dynamic::FOOTSIES);

    //First few checks are largely agnostic to cur_dynamic
    if (isDead(of) || isDead(pf)) {
      cur_dynamic = Dynamic::POSITIONING;
    } else if (oHitOffStage) {  //If the opponent is offstage and in hitstun, they're being edgeguarded, period
      cur_dynamic = Dynamic::EDGEGUARDING;
    } else if (pHitOffStage) {  //If we're offstage and in hitstun, we're being edgeguarded, period
      cur_dynamic = Dynamic::RECOVERING;
    } else if (oIsGrabbed) {  //If we grab the opponent in neutral, it's pressure; on offense, it's a techchase
      if (cur_dynamic != Dynamic::PRESSURING) {  //Need this to prevent grabs from overwriting themselves
        cur_dynamic = (cur_dynamic >= Dynamic::OFFENSIVE) ? Dynamic::TECHCHASING : Dynamic::PRESSURING;
      }
    } else if (pIsGrabbed) {  //If we are grabbed by the opponent in neutral, we're pressured; on defense, we're techchased
      if (cur_dynamic != Dynamic::PRESSURED) {  //Need this to prevent grabs from overwriting themselves
        cur_dynamic = (cur_dynamic <= Dynamic::DEFENSIVE) ? Dynamic::ESCAPING : Dynamic::PRESSURED;
      }
    } else if (oInShieldstun) {  //If our opponent is in shieldstun, we are pressureing
      cur_dynamic = Dynamic::PRESSURING;
    } else if (pInShieldstun) {  //If we are in shieldstun, we are pressured
      cur_dynamic = Dynamic::PRESSURED;
    } else if (oTeching) {  //If we are in shieldstun, we are pressured
      cur_dynamic = Dynamic::TECHCHASING;
    } else if (pTeching) {  //If we are in shieldstun, we are pressured
      cur_dynamic = Dynamic::ESCAPING;
    } else if (oInHitstun && pInHitstun) {  //If we're both in hitstun and neither of us are offstage, it's a trade
      cur_dynamic = Dynamic::TRADING;
    } else {  //Everything else depends on the cur_dynamic
      switch (cur_dynamic) {
        case Dynamic::PRESSURING:
          if (oThrown) {  //If the opponent is being thrown, this is a techchase opportunity
            cur_dynamic = Dynamic::TECHCHASING;
          } else if ((not oShielding) && (not oOnLedge) && (not oAirborne)) {  //If the opponent touches the ground w/o shielding, we're back to neutral
            cur_dynamic = neut_dyn;
          }
          break;
        case Dynamic::PRESSURED:
          if (pThrown) {  //If we are being thrown, opponent has a techchase opportunity
            cur_dynamic = Dynamic::ESCAPING;
          } else if ((not pShielding) && (not pOnLedge) && (not pAirborne)) {  //If we touch the ground w/o shielding, we're back to neutral
            cur_dynamic = neut_dyn;
          }
          break;
        case Dynamic::EDGEGUARDING:
          if (oOnLedge || (not oAirborne)) {  //If they make it back, we're back to neutral
            cur_dynamic = neut_dyn;
          }
          break;
        case Dynamic::RECOVERING:
          if (pOnLedge || (not pAirborne)) {  //If we make it back, we're back to neutral
            cur_dynamic = neut_dyn;
          }
          break;
        case Dynamic::TECHCHASING:
          if ((not oInHitstun) && (not oIsGrabbed) && (not oThrown) && (not oTeching) && (not oShielding)) {
            cur_dynamic = neut_dyn;  //If the opponent escaped our tech chase, back to neutral it is
          }
          break;
        case Dynamic::ESCAPING:
          if ((not pInHitstun) && (not pIsGrabbed) && (not pThrown) && (not pTeching) && (not pShielding)) {
            cur_dynamic = neut_dyn;  //If we escaped our opponent's tech chase, back to neutral it is
          }
          break;
        case Dynamic::PUNISHING:
          if (oBeingSharked) {  //If we let too much hitstun elapse, but our opponent is still airborne, we're sharking
            cur_dynamic = Dynamic::SHARKING;
          } else if (not oAirborne) {  //If our opponent has outright landed, we're back to neutral
            cur_dynamic = neut_dyn;
          }
          break;
        case Dynamic::PUNISHED:
          if (pBeingSharked) {  //If we survive punishes long enough, but are still airborne, we're being sharked
            cur_dynamic = Dynamic::GROUNDING;
          } else if (not pAirborne) {  //If we have outright landed, we're back to neutral
            cur_dynamic = neut_dyn;
          }
          break;
        case Dynamic::SHARKING:
          if (not oAirborne) {  //If our opponent has outright landed, we're back to neutral
            cur_dynamic = neut_dyn;
          }
          break;
        case Dynamic::GROUNDING:
          if (not pAirborne) {  //If we have outright landed, we're back to neutral
            cur_dynamic = neut_dyn;
          }
          break;
        case Dynamic::POKING:
          if (oPoked && oAirborne && oHitThisFrame) {
            cur_dynamic = Dynamic::PUNISHING; //If we have poked our opponent recently and hit them again, we're punishing
          } else if (pPoked && pAirborne && pHitThisFrame) {
            cur_dynamic = Dynamic::PUNISHED; //If we have been poked recently and got hit again, we're being punished
          } else if (oBeingPunished) {
            cur_dynamic = Dynamic::SHARKING; //If we'd otherwise be considered as punishing, we're sharking
          } else if (pBeingPunished) {
            cur_dynamic = Dynamic::GROUNDING; //If we'd otherwise be considered as being punished, we're being sharked
          } else {
            cur_dynamic = neut_dyn;  //We're in neutral if nobody has been hit recently
          }
          break;
        case Dynamic::POSITIONING:
        case Dynamic::FOOTSIES:
        case Dynamic::TRADING:
          if (oBeingSharked) {
            cur_dynamic = Dynamic::SHARKING; //If we are sharking, update accordingly
          } else if (pBeingSharked) {
            cur_dynamic = Dynamic::GROUNDING; //If we have are being sharked, update accordingly
          } else {
            cur_dynamic = neut_dyn;  //If we were trading before, we're in neutral now that we're no longer trading
          }
          break;
        default:
          break;
      }
    }

    //Aggregate results
    all_dynamics[f] = cur_dynamic;  //Set the dynamic for this frame to the current dynamic computed
  }

}

void Analyzer::analyzeMoves(const SlippiReplay &s, const uint8_t (&ports)[2], const unsigned *all_dynamics) const {
}

void Analyzer::analyze(const SlippiReplay &s) {
  (*_dout) << "Analyzing replay\n----------\n" << std::endl;

  //Verify this is a 1 v 1 match; can't analyze otherwise
  uint8_t ports[2] = {PORT_UNUSED};  //Port indices of p1 / p2
  if (not get1v1Ports(s,ports)) {
    std::cerr << "  Not a two player match; refusing to analyze further" << std::endl;
    return;
  }

  showGameHeader(s,ports);

  std::string chars[2] = { //Character names
    CharInt::name[s.player[ports[0]].ext_char_id],
    CharInt::name[s.player[ports[1]].ext_char_id]
  };

  //Interaction-level stats
  unsigned *all_dynamics = new unsigned[s.frame_count]{0}; // List of dynamics active at each frame
  analyzeInteractions(s,ports,all_dynamics);
  printInteractions(s,ports,all_dynamics);

  //Player-level stats
  for(unsigned i = 0; i < 2 ; ++i) {
    const SlippiPlayer *p = &(s.player[ports[i]]);
    std::cout << "Player " << (i+1) << " (" << chars[i] << ") stats:" << std::endl;
    computeAirtime(s,ports[i]);
    countLCancels(s,ports[i]);
    countTechs(s,ports[i]);
    countLedgegrabs(s,ports[i]);
    countDodges(s,ports[i]);
    countDashdances(s,ports[i]);
    countAirdodgesAndWavelands(s,ports[i]);
    // computeMaxCombo(s,ports[i]);
    // findAllCombos(s,ports,i);
  }

  delete all_dynamics;
  return;
}

}
