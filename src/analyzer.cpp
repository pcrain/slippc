#include "analyzer.h"

namespace slip {

Analyzer::Analyzer(std::ostream* dout) {
  _dout = dout;
  //Nothing to do yet
}

Analyzer::~Analyzer() {
  //Nothing to do yet
}

bool Analyzer::get1v1Ports(const SlippiReplay &s, Analysis *a) const {
  unsigned num_players = 0;
  for(uint8_t i = 0 ; i < 4; ++i) {
    if (s.player[i].player_type != 3) {
      if (num_players == 2) {
        return false;
      }
      a->ap[num_players++].port = i;
    }
    if (i == 3 && num_players != 2) {
      return false;
    }
  }
  (*_dout)
    << "Players found on ports "
    << a->ap[0].port << " and "
    << a->ap[1].port << std::endl
    ;
  return true;
}

void Analyzer::computeAirtime(const SlippiReplay &s, Analysis *a) const {
    for (unsigned pi = 0; pi < 2; ++pi) {
      SlippiPlayer p = s.player[a->ap[pi].port];
      unsigned airframes = 0;
      for (unsigned f = START_FRAMES; f < s.frame_count; ++f) {
        airframes += p.frame[f].airborne;
      }

      a->ap[pi].air_frames = airframes;
      // float aircount = 100*((float)airframes / s.last_frame);
      // std::cout << "  Airborne for " << aircount  << "% of match" << std::endl;
    }
}

void Analyzer::getBasicGameInfo(const SlippiReplay &s, Analysis* a) const {
  a->slippi_version    = s.slippi_version;
  a->parser_version    = s.parser_version;
  a->analyzer_version  = ANALYZER_VERSION;
  a->game_time         = s.start_time;
  a->game_length       = s.frame_count;
  a->ap[0].tag_player  = s.player[a->ap[0].port].tag;
  a->ap[1].tag_player  = s.player[a->ap[1].port].tag;
  a->ap[0].tag_css     = s.player[a->ap[0].port].tag_css;
  a->ap[1].tag_css     = s.player[a->ap[1].port].tag_css;
  a->ap[0].end_stocks  = s.player[a->ap[0].port].frame[s.frame_count-1].stocks;
  a->ap[1].end_stocks  = s.player[a->ap[1].port].frame[s.frame_count-1].stocks;
  a->ap[0].end_pct     = s.player[a->ap[0].port].frame[s.frame_count-1].percent_pre;
  a->ap[1].end_pct     = s.player[a->ap[1].port].frame[s.frame_count-1].percent_pre;
  a->ap[0].char_id     = s.player[a->ap[0].port].ext_char_id;
  a->ap[1].char_id     = s.player[a->ap[1].port].ext_char_id;
  a->ap[0].char_name   = CharInt::name[a->ap[0].char_id];
  a->ap[1].char_name   = CharInt::name[a->ap[1].char_id];
  a->stage_id          = s.stage;
  a->stage_name        = Stage::name[s.stage];

  // std::cout
  //   << a->ap[0].char_name
  //   << " ("
  //   << std::to_string(a->ap[0].end_stocks)
  //   << ") vs "
  //   << a->ap[1].char_name
  //   << " ("
  //   << std::to_string(a->ap[1].end_stocks)
  //   << ") on "
  //   << a->stage_name
  //   << std::endl;
}

// void Analyzer::printCombo(unsigned cur_combo, const uint8_t (&combo_moves)[CB_SIZE], const unsigned (&combo_frames)[CB_SIZE]) const {
//   bool allsame = true;
//   for(unsigned i = 1; i < cur_combo; ++i) {
//     if(combo_moves[i] != combo_moves[i-1]) {
//       allsame = false;
//       break;
//     }
//   }
//   if (!allsame) {
//     std::cout << "    Hit with " << cur_combo << " hit combo" << std::endl;
//     for(unsigned i = 0; i < cur_combo; ++i) {
//       std::cout << "      "
//         << Move::name[combo_moves[i]] << " on frame "
//         << combo_frames[i] << "(" << frameAsTimer(combo_frames[i]) << ")" << std::endl;
//     }
//   }
// }

// void Analyzer::findAllCombos(const SlippiReplay &s, const uint8_t (&ports)[2], uint8_t i) const {
//   uint8_t p = ports[i], o = ports[1-i]; //Port indices for player / opponent
//   SlippiFrame *pf, *of;  //Frame data for player / opponent
//   std::cout << "  Showing all combos" << std::endl;
//   bool     new_combo             = true;  //Whether we've officially started a new combo
//   unsigned combo                 = 0;     //Internal combo count for the current frame
//   unsigned last_combo            = 0;     //Internal combo count for last frame
//   unsigned cur_combo             = 0;     //Current semi-official combo cound
//   unsigned combo_frames[CB_SIZE] = {0};
//   uint8_t  combo_moves[CB_SIZE]  = {0};

//   for(unsigned f = START_FRAMES+PLAYABLE_FRAME; f <= s.frame_count; ++f) {
//     last_combo = combo;
//     pf         = &(s.player[p].frame[f]);
//     of         = &(s.player[o].frame[f]);
//     if (isInHitstun(*of) || isInHitlag(*of)) {
//       combo     = pf->combo;
//       new_combo = (last_combo == 0);
//       if (new_combo) {
//         if (cur_combo > 2) {
//           printCombo(cur_combo,combo_moves,combo_frames);
//         }
//         cur_combo = 0;
//       }
//       if (combo == cur_combo+1) {
//         combo_frames[cur_combo] = f;
//         combo_moves[cur_combo]  = pf->hit_with;
//         ++cur_combo;
//       }
//     } else if (isDead(*of)) {
//       if (cur_combo > 2) {
//         printCombo(cur_combo,combo_moves,combo_frames);
//       }
//       cur_combo = 0;
//     } else {
//       combo = 0;
//     }
//     // std::cout << combo;
//   }
//   if (cur_combo > 2) {
//     printCombo(cur_combo,combo_moves,combo_frames);
//   }
// }

void Analyzer::summarizeInteractions(const SlippiReplay &s, Analysis *a) const {
  // std::cout << "  Summarizing player interactions" << std::endl;
  for (unsigned f = START_FRAMES+PLAYABLE_FRAME; f < s.frame_count; ++f) {
    unsigned d = a->dynamics[f];
    ++(a->ap[0].dyn_counts[d]); //Increase the counter for dynamics across all frames
    if (d > Dynamic::DEFENSIVE && d < Dynamic::OFFENSIVE) {
      //Index is the same for the 2nd player for neutral dynamics
      ++(a->ap[1].dyn_counts[d]);
    } else {
      //Index is inverted for the 2nd player for non-neutral dynamics
      ++(a->ap[1].dyn_counts[Dynamic::__LAST - d]);
    }
  }
  // std::cout << std::fixed; //Show floats in fixed representation
  // for (unsigned p = 0; p < 2; ++p) {
  //   // std::cout << "    From the perspective of port " << int(a->ap[p].port+1) << " (" << a->ap[p].char_name << "):" << std::endl;
  //   for (unsigned i = 0; i < Dynamic::__LAST; ++i) {
  //     std::string n   = std::to_string(a->ap[p].dyn_counts[i]);
  //     std::string pct = std::to_string((100*float(a->ap[p].dyn_counts[i])/s.frame_count)).substr(0,5);
  //     std::cout << "      " << SPACE[6-n.length()] << n << " frames (" << pct
  //       << "% of game) spent in " << Dynamic::name[i] << std::endl;
  //   }
  // }
}

void Analyzer::countLCancels(const SlippiReplay &s, Analysis *a) const {
  for (unsigned pi = 0; pi < 2; ++pi) {
    SlippiPlayer p = s.player[a->ap[pi].port];
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

    a->ap[pi].l_cancels_hit    = cancels_hit;
    a->ap[pi].l_cancels_missed = cancels_miss;
    float cancels_pct = 100*((float)cancels_hit / (cancels_hit+cancels_miss));
    // std::cout << "  Hit " << cancels_hit << "/" << cancels_hit+cancels_miss << " l cancels (" << cancels_pct << "%)" << std::endl;
  }
}

void Analyzer::countTechs(const SlippiReplay &s, Analysis *a) const {
  for (unsigned pi = 0; pi < 2; ++pi) {
    SlippiPlayer p = s.player[a->ap[pi].port];
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

    a->ap[pi].techs         = techs_hit;
    a->ap[pi].walltechs     = walltechs_hit;
    a->ap[pi].walljumps     = walljumps_hit;
    a->ap[pi].walltechjumps = walljumptechs_hit;
    a->ap[pi].missed_techs  = techs_missed;

    // std::cout << "  Hit "    << techs_hit         << " grounded techs"  << std::endl;
    // std::cout << "  Hit "    << walltechs_hit     << " wall techs"      << std::endl;
    // std::cout << "  Hit "    << walljumps_hit     << " wall jumps"      << std::endl;
    // std::cout << "  Hit "    << walljumptechs_hit << " wall jump techs" << std::endl;
    // std::cout << "  Missed " << techs_missed      << " techs"           << std::endl;
  }
}

void Analyzer::countDashdances(const SlippiReplay &s, Analysis *a) const {
  for (unsigned pi = 0; pi < 2; ++pi) {
    SlippiPlayer p = s.player[a->ap[pi].port];
    unsigned dashdances = 0;
    for(unsigned f = START_FRAMES; f < s.frame_count; ++f) {
      if (isDashdancing(p,f)) {
        ++dashdances;
      }
    }

    a->ap[pi].dashdances = dashdances;
    // std::cout << "  Dashdanced " << dashdances  << " times" << std::endl;
  }
}

void Analyzer::countLedgegrabs(const SlippiReplay &s, Analysis *a) const {
  for (unsigned pi = 0; pi < 2; ++pi) {
    SlippiPlayer p = s.player[a->ap[pi].port];
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
    a->ap[pi].ledge_grabs = ledgegrabs;
    // std::cout << "  Grabbed the ledge " << ledgegrabs  << " times" << std::endl;
  }
}

//Airdodges counted separately when looking out wavedashes
void Analyzer::countDodges(const SlippiReplay &s, Analysis *a) const {
  for (unsigned pi = 0; pi < 2; ++pi) {
    SlippiPlayer p = s.player[a->ap[pi].port];
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

    a->ap[pi].rolls      = rolls;
    a->ap[pi].spotdodges = dodges;
    // std::cout << "  Rolled " << rolls  << " times" << std::endl;
    // std::cout << "  Spotdodged " << dodges  << " times" << std::endl;
  }
}

//Code adapted from Fizzi's
void Analyzer::countAirdodgesAndWavelands(const SlippiReplay &s, Analysis *a) const {
  for (unsigned pi = 0; pi < 2; ++pi) {
    SlippiPlayer p = s.player[a->ap[pi].port];
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

    a->ap[pi].airdodges  = airdodges;
    a->ap[pi].wavelands  = wavelands;
    a->ap[pi].wavedashes = wavedashes;
    // std::cout << "  Airdodged "  << airdodges   << " times" << std::endl;
    // std::cout << "  Wavelanded " << wavelands   << " times" << std::endl;
    // std::cout << "  Wavedashed " << wavedashes  << " times" << std::endl;
  }
}

void Analyzer::analyzeInteractions(const SlippiReplay &s, Analysis *a) const {
  // std::cout << "  Analyzing player interactions" << std::endl;
  const SlippiPlayer *p                = &(s.player[a->ap[0].port]);
  const SlippiPlayer *o                = &(s.player[a->ap[1].port]);

  unsigned cur_dynamic                 = Dynamic::POSITIONING; // Current dynamic in effect

  unsigned oLastInHitsun               = 0;  //Last frame opponent was stuck in hitstun
  unsigned pLastInHitsun               = 0;  //Last frame we were stuck in hitstun
  // unsigned oLastHitsunFC               = 0;  //Last count for our opponent's' hitstun
  // unsigned pLastHitsunFC               = 0;  //Last count for our hitstun
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
        // std::cout << int(uint8_t( ((char*)(void*)&(of.hitstun))[2] )) << std::endl;
        // if (of.hitstun > oLastHitsunFC) {  //TODO: hitstun frames left is unreliable! Weird numbers; ask Fizzi
        if (of.percent_post > of.percent_pre) {
          oHitThisFrame = true; //Check if opponent was hit this frame by looking at hitstun frames left
        }
        // oLastHitsunFC = of.hitstun;
      }
    bool pHitThisFrame = false;
    bool pInHitstun    = isInHitstun(pf);
      if (pInHitstun)    {
        pLastInHitsun = f;
        if (pf.percent_post > pf.percent_pre) {
          pHitThisFrame = true; //Check if we were hit this frame by looking at hitstun frames left
        }
        // pLastHitsunFC = of.hitstun;
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

    //Determine whether neutral would be considered footsies or positioning
    // unsigned neut_dyn = ( oPoked || pPoked ) ? Dynamic::POKING
    //   : ((playerDistance(pf,of) > FOOTSIE_THRES) ? Dynamic::POSITIONING : Dynamic::FOOTSIES);
    unsigned neut_dyn = ((playerDistance(pf,of) > FOOTSIE_THRES) ? Dynamic::POSITIONING : Dynamic::FOOTSIES);

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
          } else if (oBeingSharked) {
            cur_dynamic = Dynamic::SHARKING; //If we'd otherwise be considered as punishing, we're sharking
          } else if (pBeingSharked) {
            cur_dynamic = Dynamic::GROUNDING; //If we'd otherwise be considered as being punished, we're being sharked
          } else {
            cur_dynamic = ( oPoked || pPoked ) ? Dynamic::POKING : neut_dyn;  //We're in neutral if nobody has been hit recently
          }
          break;
        case Dynamic::POSITIONING:
        case Dynamic::FOOTSIES:
        case Dynamic::TRADING:
          if (oBeingSharked) {
            cur_dynamic = Dynamic::SHARKING; //If we are sharking, update accordingly
          } else if (pBeingSharked) {
            cur_dynamic = Dynamic::GROUNDING; //If we are being sharked, update accordingly
          } else {
            cur_dynamic = ( oPoked || pPoked ) ? Dynamic::POKING : neut_dyn;  //If we were trading before, we're in neutral now that we're no longer trading
          }
          break;
        default:
          break;
      }
    }

    //Aggregate results
    a->dynamics[f] = cur_dynamic;  //Set the dynamic for this frame to the current dynamic computed
  }

}

void Analyzer::analyzeMoves(const SlippiReplay &s, Analysis *a) const {
  // std::cout << "  Summarizing moves" << std::endl;
  // std::cout << "    From the perspective of port " << int(a->ap[0].port+1) << " (" << CharInt::name[s.player[a->ap[0].port].ext_char_id] << "):" << std::endl;

  const SlippiPlayer *p         = &(s.player[a->ap[0].port]);
  const SlippiPlayer *o         = &(s.player[a->ap[1].port]);

  unsigned oLastHitstunStart    = 0;
  unsigned pLastHitstunStart    = 0;
  unsigned oLastHitstunEnd      = 0;
  unsigned pLastHitstunEnd      = 0;

  int      lastpoke             = 0;  //0 = no last poke, 1 = our last poke, -1 = opponent's last poke
  unsigned pokes                = 0;
  unsigned poked                = 0;
  unsigned neutral_wins         = 0;
  unsigned neutral_losses       = 0;
  // unsigned neutral_conversions  = 0;
  // unsigned neutral_converted_on = 0;
  unsigned counters             = 0;
  unsigned countered_on         = 0;

  unsigned last_dyn             = a->dynamics[FIRST_FRAME];
  unsigned cur_dyn              = a->dynamics[FIRST_FRAME];
  for (unsigned f = FIRST_FRAME; f < s.frame_count; ++f) {
    cur_dyn        = a->dynamics[f];

    if (cur_dyn == Dynamic::POKING) {
      if (last_dyn != Dynamic::POKING) {
        if (isInHitstun(o->frame[f])) {
          // std::cout << "Poked at " << frameAsTimer(f) << std::endl;
          ++pokes;
          lastpoke = 1;
        } else if (isInHitstun(p->frame[f])) {
          // std::cout << "Got poked at " << frameAsTimer(f) << std::endl;
          ++poked;
          lastpoke = -1;
        }
      }
      last_dyn = cur_dyn;
      continue;
    }
    if (cur_dyn <= Dynamic::DEFENSIVE) {
      if (last_dyn >= Dynamic::OFFENSIVE) {
        ++countered_on;
      } else if (last_dyn > Dynamic::DEFENSIVE) {
        if (lastpoke == -1 && (cur_dyn == Dynamic::ESCAPING || cur_dyn == Dynamic::PUNISHED)) {
          // std::cout << "  " << Dynamic::name[cur_dyn] << " Got unpoked at " << frameAsTimer(f) << std::endl;
          --poked;
        }
        ++neutral_losses;
      }
    } else if (cur_dyn >= Dynamic::OFFENSIVE) {
      if (last_dyn <= Dynamic::DEFENSIVE) {
        ++counters;  //If we went from defense to offense, we countered
      } else if (last_dyn < Dynamic::OFFENSIVE) {
        if (lastpoke == 1 && (cur_dyn == Dynamic::TECHCHASING || cur_dyn == Dynamic::PUNISHING)) {
          // std::cout << "  " << Dynamic::name[cur_dyn] << " Unpoked at " << frameAsTimer(f) << std::endl;
          --pokes;
        }
        ++neutral_wins;
      }
    } else {
      //Back in neutral
    }
    lastpoke = 0;

    last_dyn = cur_dyn;
  }

  a->ap[0].counters     = counters;
  a->ap[0].neutral_wins = neutral_wins;
  a->ap[0].pokes        = pokes;
  a->ap[1].pokes        = poked;
  a->ap[1].neutral_wins = neutral_losses;
  a->ap[1].counters     = countered_on;

  // std::cout << "      Countered "     << a->ap[0].counters     << " times" << std::endl;
  // std::cout << "      Won neutral "   << a->ap[0].neutral_wins << " times" << std::endl;
  // std::cout << "      Poked "         << a->ap[0].pokes        << " times" << std::endl;
  // std::cout << "      Got poked "     << a->ap[1].pokes        << " times" << std::endl;
  // std::cout << "      Lost neutral "  << a->ap[1].neutral_wins << " times" << std::endl;
  // std::cout << "      Was countered " << a->ap[1].counters     << " times" << std::endl;
}

void Analyzer::analyzePunishes(const SlippiReplay &s, Analysis *a) const {
  const SlippiPlayer *p = &(s.player[a->ap[0].port]);
  const SlippiPlayer *o = &(s.player[a->ap[1].port]);
  unsigned pn           = 0; //Running tally of player punishes
  unsigned on           = 0; //Running tally of opponent punishes
  unsigned cur_dyn      = a->dynamics[FIRST_FRAME];
  Punish *pPunishes     = a->ap[0].punishes;
  Punish *oPunishes     = a->ap[1].punishes;

  for (unsigned f = FIRST_FRAME; f < s.frame_count; ++f) {
    SlippiFrame of = o->frame[f];
    SlippiFrame pf = p->frame[f];
    cur_dyn        = a->dynamics[f];

    if (pf.stocks < p->frame[f-1].stocks) {
      if (oPunishes[on].num_moves > 0) {
        oPunishes[on].kill_dir = deathDirection(*p,f);
      } else if (on > 0) {
        oPunishes[on-1].kill_dir = deathDirection(*p,f);
      }
    }
    if (of.stocks < o->frame[f-1].stocks) {
      if (pPunishes[pn].num_moves > 0) {
        pPunishes[pn].kill_dir = deathDirection(*o,f);
      } else if (pn > 0) {
        pPunishes[pn-1].kill_dir = deathDirection(*o,f);
      }
    }

    bool pPunishEnd = false;
    bool oPunishEnd = false;
    if (f == s.frame_count-1) {
      pPunishEnd = true;
      oPunishEnd = true;
    } else if (cur_dyn != Dynamic::POKING) {
      if (cur_dyn > Dynamic::DEFENSIVE) {
        //If the opponent had a punish going and they are no longer on offense, end their punish
        oPunishEnd = true;
      }
      if (cur_dyn < Dynamic::OFFENSIVE) {
        //If we had a punish going and we are no longer on offense, end our punish
        pPunishEnd = true;
      }
    }

    if (pPunishEnd) {
      if (pPunishes[pn].num_moves > 0) {
        pPunishes[pn].end_frame    = f;
        pPunishes[pn].end_pct      = of.percent_pre;
        pPunishes[pn].last_move_id = pf.hit_with;
        ++pn;
      }
    }
    if (oPunishEnd) {
      if (oPunishes[on].num_moves > 0) {
        oPunishes[on].end_frame    = f;
        oPunishes[on].end_pct      = pf.percent_pre;
        oPunishes[on].last_move_id = of.hit_with;
        ++on;
      }
    }

    //If the opponent just took damage
    if (of.percent_pre > o->frame[f-1].percent_pre) {
      //If this is the start of a combo
      if (pPunishes[pn].num_moves == 0) {
        pPunishes[pn].start_frame = f;
        pPunishes[pn].start_pct   = o->frame[f-1].percent_pre;
        pPunishes[pn].kill_dir    = Dir::NEUT;
      }
      pPunishes[pn].end_frame     = f;
      pPunishes[pn].end_pct       = of.percent_pre;
      pPunishes[pn].last_move_id  = pf.hit_with;
      pPunishes[pn].num_moves    += 1;
    }

    //If we just took damage
    if (pf.percent_pre > p->frame[f-1].percent_pre) {
      //If this is the start of a combo
      if (oPunishes[on].num_moves == 0) {
        oPunishes[on].start_frame = f;
        oPunishes[on].start_pct   = p->frame[f-1].percent_pre;
        oPunishes[on].kill_dir    = Dir::NEUT;
      }
      oPunishes[on].end_frame     = f;
      oPunishes[on].end_pct       = pf.percent_pre;
      oPunishes[on].last_move_id  = of.hit_with;
      oPunishes[on].num_moves    += 1;
    }
  }

  // std::cout << "Showing punishes for player 1" << std::endl;
  // for(unsigned i = 0; pPunishes[i].num_moves > 0; ++i) {
  //   std::cout
  //     << "  "                   << pPunishes[i].num_moves
  //     << " moves from frames "  << frameAsTimer(pPunishes[i].start_frame)
  //     << " to "                 << frameAsTimer(pPunishes[i].end_frame)
  //     << " dealing "            << (pPunishes[i].end_pct - pPunishes[i].start_pct)
  //     << " damage "
  //     << std::endl;
  //     if (pPunishes[i].kill_dir >= 0) {
  //       std::cout
  //       << "    Killed with " << Move::name[pPunishes[i].last_move_id]
  //       << " at " << pPunishes[i].end_pct
  //       << "% off blastzone " << Dir::name[pPunishes[i].kill_dir]
  //       << std::endl;
  //     }
  // }

  // std::cout << "Showing punishes for player 2" << std::endl;
  // for(unsigned i = 0; oPunishes[i].num_moves > 0; ++i) {
  //   std::cout
  //     << "  "                   << oPunishes[i].num_moves
  //     << " moves from frames "  << frameAsTimer(oPunishes[i].start_frame)
  //     << " to "                 << frameAsTimer(oPunishes[i].end_frame)
  //     << " dealing "            << (oPunishes[i].end_pct - oPunishes[i].start_pct)
  //     << " damage "
  //     << std::endl;
  //     if (oPunishes[i].kill_dir >= 0) {
  //       std::cout
  //       << "    Killed with " << Move::name[oPunishes[i].last_move_id]
  //       << " at " << oPunishes[i].end_pct
  //       << "% off blastzone " << Dir::name[oPunishes[i].kill_dir]
  //       << std::endl;
  //     }
  // }
}

Analysis* Analyzer::analyze(const SlippiReplay &s) {
  (*_dout) << "Analyzing replay\n----------\n" << std::endl;

  Analysis *a            = new Analysis();  //Structure for holding the analysis so far
  a->dynamics            = new unsigned[s.frame_count]{0}; // List of dynamics active at each frame

  //Verify this is a 1 v 1 match; can't analyze otherwise
  if (not get1v1Ports(s,a)) {
    std::cerr << "  Not a two player match; refusing to analyze further" << std::endl;
    a->success = false;
    return a;
  }

  getBasicGameInfo(s,a);

  //Interaction-level stats
  analyzeInteractions(s,a);
  summarizeInteractions(s,a);
  analyzeMoves(s,a);
  analyzePunishes(s,a);

  //Player-level stats
  computeAirtime(s,a);
  countLCancels(s,a);
  countTechs(s,a);
  countLedgegrabs(s,a);
  countDodges(s,a);
  countDashdances(s,a);
  countAirdodgesAndWavelands(s,a);

  a->success = true;
  return a;
}

}
