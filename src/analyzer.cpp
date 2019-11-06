#include "analyzer.h"

//Debug output convenience macros
#define DOUT1(s) if (_debug >= 1) { std::cout << s; }
#define DOUT2(s) if (_debug >= 2) { std::cout << s; }

namespace slip {

Analyzer::Analyzer(int debug_level) {
  _debug = debug_level;
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
  DOUT1("Players found on ports " << a->ap[0].port << " and " << a->ap[1].port << std::endl);
  return true;
}

void Analyzer::computeAirtime(const SlippiReplay &s, Analysis *a) const {
    for (unsigned pi = 0; pi < 2; ++pi) {
      SlippiPlayer p = s.player[a->ap[pi].port];
      unsigned airframes = 0;
      for (unsigned f = (-LOAD_FRAME); f < s.frame_count; ++f) {
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

  a->ap[0].start_stocks  = s.player[a->ap[0].port].start_stocks;
  a->ap[1].start_stocks  = s.player[a->ap[1].port].start_stocks;
  a->ap[0].color         = s.player[a->ap[0].port].color;
  a->ap[1].color         = s.player[a->ap[1].port].color;
  a->ap[0].team_id       = s.player[a->ap[0].port].team_id;
  a->ap[1].team_id       = s.player[a->ap[1].port].team_id;
  a->ap[0].player_type   = s.player[a->ap[0].port].player_type;
  a->ap[1].player_type   = s.player[a->ap[1].port].player_type;

  a->ap[0].end_stocks  = s.player[a->ap[0].port].frame[s.frame_count-1].stocks;
  a->ap[1].end_stocks  = s.player[a->ap[1].port].frame[s.frame_count-1].stocks;
  a->ap[0].end_pct     = s.player[a->ap[0].port].frame[s.frame_count-1].percent_pre;
  a->ap[1].end_pct     = s.player[a->ap[1].port].frame[s.frame_count-1].percent_pre;
  a->ap[0].char_id     = s.player[a->ap[0].port].ext_char_id;
  a->ap[1].char_id     = s.player[a->ap[1].port].ext_char_id;
  a->stage_id          = s.stage;
  a->ap[0].char_name   = CharExt::name[a->ap[0].char_id];
  a->ap[1].char_name   = CharExt::name[a->ap[1].char_id];
  a->stage_name        = Stage::name[s.stage];
}

void Analyzer::summarizeInteractions(const SlippiReplay &s, Analysis *a) const {
  // std::cout << "  Summarizing player interactions" << std::endl;
  for (unsigned f = (PLAYABLE_FRAME-LOAD_FRAME); f < s.frame_count; ++f) {
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
    for(unsigned f = (-LOAD_FRAME); f < s.frame_count; ++f) {
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

    for(unsigned f = (-LOAD_FRAME); f < s.frame_count; ++f) {
      if (inTechState(p.frame[f])) {
        if (not teching) {
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
    for(unsigned f = (-LOAD_FRAME); f < s.frame_count; ++f) {
      if (isDashdancing(p,f)) {
        ++dashdances;
      }
    }

    a->ap[pi].dashdances = dashdances;
    // std::cout << "  Dashdanced " << dashdances  << " times" << std::endl;
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
    for(unsigned f = (-LOAD_FRAME); f < s.frame_count; ++f) {
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
  unsigned oLastGrounded               = 0;  //Last frame opponent touched solid ground
  unsigned pLastGrounded               = 0;  //Last frame we touched solid ground

  //All interactions analyzed from perspective of p (lower port player)
  for (unsigned f = (PLAYABLE_FRAME-LOAD_FRAME); f < s.frame_count; ++f) {
    //Important variables for each frame
    SlippiFrame of     = o->frame[f];
    SlippiFrame pf     = p->frame[f];
    bool oHitThisFrame = false;
    bool oInHitstun    = isInHitstun(of);
      if (oInHitstun)    {
        oLastInHitsun = f;
        if (of.percent_post > o->frame[f-1].percent_post) {
          oHitThisFrame = true; //Check if opponent was hit this frame by looking at % last frame
        }
      }
    bool pHitThisFrame = false;
    bool pInHitstun    = isInHitstun(pf);
      if (pInHitstun)    {
        pLastInHitsun = f;
        if (pf.percent_post > p->frame[f-1].percent_post) {
          pHitThisFrame = true; //Check if we were hit this frame by looking at % last frame
        }
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
    } else if (oHitOffStage && cur_dynamic != Dynamic::RECOVERING) {  //If the opponent is offstage and in hitstun, they're being [possibly reverse] edgeguarded
      cur_dynamic = Dynamic::EDGEGUARDING;
    } else if (pHitOffStage && cur_dynamic != Dynamic::EDGEGUARDING) {  //If we're offstage and in hitstun, we're being [possibly reverse] edgeguarded
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
    } else if (oTeching) {  //If we are teching, we are pressured
      cur_dynamic = Dynamic::TECHCHASING;
    } else if (pTeching) {  //If we are teching, we are pressured
      cur_dynamic = Dynamic::ESCAPING;
    } else if (oInHitstun && pInHitstun) {  //If we're both in hitstun and neither of us are offstage, it's a trade
      // cur_dynamic = Dynamic::TRADING;
      cur_dynamic = Dynamic::POKING; //Update 2019-11-05: trading is no longer a used dynamic; now just poking
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
          if (oOnLedge || (not oAirborne && not oPoked)) {  //If they make it back, we're back to neutral
            cur_dynamic = neut_dyn;
          }
          break;
        case Dynamic::RECOVERING:
          if (pOnLedge || (not pAirborne && not pPoked)) {  //If we make it back, we're back to neutral
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
          } else if ((not oAirborne) && (not oPoked)) {  //If our opponent has outright landed, we're back to neutral
            cur_dynamic = neut_dyn;
          }
          break;
        case Dynamic::PUNISHED:
          if (pBeingSharked) {  //If we survive punishes long enough, but are still airborne, we're being sharked
            cur_dynamic = Dynamic::GROUNDING;
          } else if ((not pAirborne) && (not pPoked)) {  //If we have outright landed, we're back to neutral
            cur_dynamic = neut_dyn;
          }
          break;
        case Dynamic::SHARKING:
          if (pInHitstun) { //If our opponent hits us while sharking, we're back to poking
            cur_dynamic = Dynamic::POKING;
          } else if (not oAirborne) {  //If our opponent has outright landed, we're back to neutral
            cur_dynamic = neut_dyn;
          }
          break;
        case Dynamic::GROUNDING:
          if (oInHitstun) { //If we hit our opponent while grounding, we're back to poking
            cur_dynamic = Dynamic::POKING;
          } else if (not pAirborne) {  //If we have outright landed, we're back to neutral
            cur_dynamic = neut_dyn;
          }
          break;
        case Dynamic::POKING:
          if (oPoked && oHitThisFrame) {
            cur_dynamic = Dynamic::PUNISHING; //If we have poked our opponent recently and hit them again, we're punishing
          } else if (pPoked && pHitThisFrame) {
            cur_dynamic = Dynamic::PUNISHED; //If we have been poked recently and got hit again, we're being punished
          } else if (oBeingSharked && not pPoked) {
            cur_dynamic = Dynamic::SHARKING; //If we'd otherwise be considered as punishing, we're sharking
          } else if (pBeingSharked && not oPoked) {
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
    DOUT1("    " << f << " (" << frameAsTimer(f,s.timer) << ") P1 "
      << Dynamic::name[cur_dynamic] << " "
      << std::endl);
  }

}

void Analyzer::analyzeMoves(const SlippiReplay &s, Analysis *a) const {
  // std::cout << "  Summarizing moves" << std::endl;
  // std::cout << "    From the perspective of port " << int(a->ap[0].port+1) << " (" << CharInt::name[s.player[a->ap[0].port].ext_char_id] << "):" << std::endl;

  const SlippiPlayer *p         = &(s.player[a->ap[0].port]);
  const SlippiPlayer *o         = &(s.player[a->ap[1].port]);


  int      lastpoke             = 0;  //0 = no last poke, 1 = our last poke, -1 = opponent's last poke
  unsigned pokes                = 0;
  unsigned poked                = 0;
  unsigned neutral_wins         = 0;
  unsigned neutral_losses       = 0;
  unsigned counters             = 0;
  unsigned countered_on         = 0;

  unsigned last_dyn             = a->dynamics[FIRST_FRAME];
  unsigned cur_dyn              = a->dynamics[FIRST_FRAME];
  for (unsigned f = FIRST_FRAME; f < s.frame_count; ++f) {
    cur_dyn        = a->dynamics[f];
    if (cur_dyn == Dynamic::POKING) {
      if (last_dyn != Dynamic::POKING) {
        if (isInHitstun(o->frame[f])) {
          ++pokes;
          lastpoke = 1;
        } else if (isInHitstun(p->frame[f])) {
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
          --poked;
        }
        ++neutral_losses;
        DOUT1("    " << f << " (" << frameAsTimer(f,s.timer) << ") P2 "
          << " won neutral"
          << std::endl);
      }
    } else if (cur_dyn >= Dynamic::OFFENSIVE) {
      if (last_dyn <= Dynamic::DEFENSIVE) {
        ++counters;  //If we went from defense to offense, we countered
      } else if (last_dyn < Dynamic::OFFENSIVE) {
        if (lastpoke == 1 && (cur_dyn == Dynamic::TECHCHASING || cur_dyn == Dynamic::PUNISHING)) {
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
}

void Analyzer::analyzePunishes(const SlippiReplay &s, Analysis *a) const {
  const SlippiPlayer *p  = &(s.player[a->ap[0].port]);
  const SlippiPlayer *o  = &(s.player[a->ap[1].port]);
  unsigned pa            = 0; //Running tally of player attacks
  unsigned oa            = 0; //Running tally of opponent attacks
  unsigned pn            = 0; //Running tally of player punishes
  unsigned on            = 0; //Running tally of opponent punishes
  unsigned oLastInHitsun = 0;
  unsigned pLastInHitsun = 0;
  unsigned cur_dyn       = a->dynamics[FIRST_FRAME];
  Punish *pPunishes      = a->ap[0].punishes;
  Punish *oPunishes      = a->ap[1].punishes;
  Attack *pAttacks       = a->ap[0].attacks;
  Attack *oAttacks       = a->ap[1].attacks;

  for (unsigned f = FIRST_FRAME; f < s.frame_count; ++f) {
    SlippiFrame pf = p->frame[f];
    SlippiFrame of = o->frame[f];
    cur_dyn        = a->dynamics[f];

    oLastInHitsun = isInHitstun(o->frame[f]) ? f : oLastInHitsun;
    pLastInHitsun = isInHitstun(p->frame[f]) ? f : pLastInHitsun;
    bool oPoked    = ((f - oLastInHitsun) < POKE_THRES);  //Check if opponent has been poked recently
    bool pPoked    = ((f - pLastInHitsun) < POKE_THRES);  //Check if we have been poked recently

    //Check if either player lost a stock
    if (pf.stocks < p->frame[f-1].stocks) {
      unsigned ddir = deathDirection(*p,f);
      if (oa > 0) {
        oAttacks[oa-1].kill_dir = ddir;
      }
      if (oPunishes[on].num_moves > 0) {
        oPunishes[on].kill_dir = ddir; //TODO: filter out self-destructs somehow
      } else if (on > 0) {
        oPunishes[on-1].kill_dir = ddir;
      }
      if (cur_dyn == Dynamic::EDGEGUARDING) {
        ++(a->ap[1].reverse_edgeguards);
      } else if (cur_dyn != Dynamic::RECOVERING && ddir != Dir::UP) {
        ++(a->ap[0].self_destructs);
      }
    }
    if (of.stocks < o->frame[f-1].stocks) {
      unsigned ddir = deathDirection(*o,f);
      if (pa > 0) {
        pAttacks[pa-1].kill_dir = ddir;
      }
      if (pPunishes[pn].num_moves > 0) {
        pPunishes[pn].kill_dir = ddir;
      } else if (pn > 0) {
        pPunishes[pn-1].kill_dir = ddir;
      }
      if (cur_dyn == Dynamic::RECOVERING) {
        ++(a->ap[0].reverse_edgeguards);
      } else if (cur_dyn != Dynamic::EDGEGUARDING && ddir != Dir::UP) {
        ++(a->ap[1].self_destructs);
      }
    }

    //Check if either player has dropped a punish off an opening
    bool pPunishEnd = false;
    bool oPunishEnd = false;
    if (f == s.frame_count-1) {
      pPunishEnd = true;
      oPunishEnd = true;
    } else if (cur_dyn != Dynamic::POKING) {
      if (cur_dyn > Dynamic::DEFENSIVE && not pPoked) {
        //If the opponent had a punish going and they are no longer on offense, end their punish
        oPunishEnd = true;
      }
      if (cur_dyn < Dynamic::OFFENSIVE && not oPoked) {
        //If we had a punish going and we are no longer on offense, end our punish
        pPunishEnd = true;
      }
    }

    //Update punish counter for ended punishes
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
    float o_damage_taken = of.percent_pre - o->frame[f-1].percent_pre;
    if (o_damage_taken > 0) {
      //Check for bubble damage
      pAttacks[pa].move_id    = (isOffscreen(of) && o_damage_taken == 1) ? Move::BUBBLE : pf.hit_with;
      //Last frame we actually took damage; frame before that gives us the animation frame our move hit
      pAttacks[pa].anim_frame = p->frame[f-2].action_fc;
      pAttacks[pa].punish_id  = pn;
      if (  //Check if this is a consecutive hit from a multihit move
       pa > 0 &&  //If this isn't our first move
       pAttacks[pa].move_id == pAttacks[pa-1].move_id &&  //If the last move we hit with had the same id
       pAttacks[pa].anim_frame > pAttacks[pa-1].anim_frame && //If the action frame counter is higher
       //If the gap between animation and game frames is less than max hitlag for the move
       //Formula: https://www.ssbwiki.com/Freeze_frame
       (f - pAttacks[pa].anim_frame <= (pAttacks[pa-1].frame - pAttacks[pa-1].anim_frame + (pAttacks[pa-1].damage/3+3)))) {
        pAttacks[pa].hit_id  = pAttacks[pa-1].hit_id+1;
      } else {
        pAttacks[pa].hit_id  = 1;
      }
      pAttacks[pa].frame      = f;
      pAttacks[pa].damage     = o_damage_taken;
      pAttacks[pa].opening    = cur_dyn;  //Dynamic is normal for player
      pAttacks[pa].kill_dir   = Dir::NEUT;
      ++pa;
      //If this is the start of a combo
      if (pPunishes[pn].num_moves == 0) {
        pPunishes[pn].start_frame = f;
        pPunishes[pn].start_pct   = o->frame[f-1].percent_pre;
        pPunishes[pn].kill_dir    = Dir::NEUT;
      }
      a->ap[0].damage_dealt      += o_damage_taken;
      pPunishes[pn].end_frame     = f;
      pPunishes[pn].end_pct       = of.percent_pre;
      pPunishes[pn].last_move_id  = pf.hit_with;
      pPunishes[pn].num_moves    += 1;
      ++(a->ap[0].move_counts[pf.hit_with]);
    }

    //If we just took damage
    float p_damage_taken = pf.percent_pre - p->frame[f-1].percent_pre;
    if (p_damage_taken > 0) {
      //Check for bubble damage
      oAttacks[oa].move_id    = (isOffscreen(pf) && p_damage_taken == 1)  ? Move::BUBBLE : of.hit_with;
      //Last frame we actually took damage; frame before that gives us the animation frame our move hit
      oAttacks[oa].anim_frame = o->frame[f-2].action_fc;
      oAttacks[oa].punish_id  = on;
      if (  //Check if this is a consecutive hit from a multihit move
       oa > 0 &&  //If this isn't our first move
       oAttacks[oa].move_id == oAttacks[oa-1].move_id &&  //If the last move we hit with had the same id
       oAttacks[oa].anim_frame > oAttacks[oa-1].anim_frame && //If the action frame counter is higher
       //If the gap between animation and game frames is less than max hitlag for the move
       (f - oAttacks[oa].anim_frame <= (oAttacks[oa-1].frame - oAttacks[oa-1].anim_frame + (oAttacks[oa-1].damage/3+3)))) {
        oAttacks[oa].hit_id  = oAttacks[oa-1].hit_id+1;
      } else {
        oAttacks[oa].hit_id  = 1;
      }
      oAttacks[oa].frame      = f;
      oAttacks[oa].damage     = p_damage_taken;
      oAttacks[oa].opening    = //Dynamic needs to be flipped around for opponent
        (cur_dyn > Dynamic::OFFENSIVE || cur_dyn < Dynamic::DEFENSIVE) ? Dynamic::__LAST-cur_dyn : cur_dyn;
      oAttacks[oa].kill_dir   = Dir::NEUT;
      ++oa;
      //If this is the start of a combo
      if (oPunishes[on].num_moves == 0) {
        oPunishes[on].start_frame = f;
        oPunishes[on].start_pct   = p->frame[f-1].percent_pre;
        oPunishes[on].kill_dir    = Dir::NEUT;
      }
      a->ap[1].damage_dealt      += p_damage_taken;
      oPunishes[on].end_frame     = f;
      oPunishes[on].end_pct       = pf.percent_pre;
      oPunishes[on].last_move_id  = of.hit_with;
      oPunishes[on].num_moves    += 1;
      ++(a->ap[1].move_counts[of.hit_with]);
    }
  }
}

unsigned Analyzer::countTransitions(const SlippiReplay &s, Analysis *a, unsigned pnum, bool (*cb)(const SlippiFrame&)) const {
  SlippiPlayer p = s.player[a->ap[pnum].port];
  unsigned counter = 0;
  bool active = false;
  for(unsigned f = (-LOAD_FRAME); f < s.frame_count; ++f) {
    if (cb(p.frame[f])) {
      if(not active) {
        ++counter;
        active = true;
      }
    } else {
      active = false;
    }
  }
  return counter;
}

unsigned Analyzer::countTransitions(const SlippiReplay &s, Analysis *a, unsigned pnum, bool (*cb)(const SlippiPlayer &, const unsigned)) const {
  SlippiPlayer p = s.player[a->ap[pnum].port];
  unsigned counter = 0;
  bool active = false;
  for(unsigned f = (-LOAD_FRAME); f < s.frame_count; ++f) {
    if (cb(p,f)) {
      if(not active) {
        ++counter;
        active = true;
      }
    } else {
      active = false;
    }
  }
  return counter;
}

void Analyzer::countBasicAnimations(const SlippiReplay &s, Analysis *a) const {
  for (unsigned pi = 0; pi < 2; ++pi) {
    a->ap[pi].ledge_grabs          = countTransitions(s,a,pi,isOnLedge);
    a->ap[pi].rolls                = countTransitions(s,a,pi,isRolling);
    a->ap[pi].spotdodges           = countTransitions(s,a,pi,isSpotdodging);
    a->ap[pi].powershields         = countTransitions(s,a,pi,isPowershielding);
    a->ap[pi].grabs                = countTransitions(s,a,pi,isGrabbing);
    a->ap[pi].taunts               = countTransitions(s,a,pi,isTaunting);
    a->ap[pi].meteor_cancels       = countTransitions(s,a,pi,didMeteorCancel);
    a->ap[pi].hits_blocked         = countTransitions(s,a,pi,isInShieldstun);
    a->ap[pi].edge_cancel_aerials  = countTransitions(s,a,pi,didEdgeCancelAerial);
    a->ap[pi].edge_cancel_specials = countTransitions(s,a,pi,didEdgeCancelSpecial);
    a->ap[pi].no_impact_lands      = countTransitions(s,a,pi,didNoImpactLand);
    a->ap[pi].shield_drops         = countTransitions(s,a,pi,didShieldDrop);
    a->ap[pi].pivots               = countTransitions(s,a,pi,didPivot);
    a->ap[pi].shield_breaks        = countTransitions(s,a,1-pi,isShieldBroken);
    a->ap[pi].grab_escapes         = countTransitions(s,a,1-pi,isReleasing);
    a->ap[pi].shield_stabs         = countTransitions(s,a,1-pi,wasShieldStabbed);
    a->ap[pi].stage_spikes         = countTransitions(s,a,1-pi,wasStageSpiked);
  }
}

void Analyzer::showActionStates(const SlippiReplay &s, Analysis *a) const {
  const SlippiPlayer *p         = &(s.player[a->ap[0].port]);
  const SlippiPlayer *o         = &(s.player[a->ap[1].port]);
  for (unsigned f = FIRST_FRAME; f < s.frame_count; ++f) {
    SlippiFrame pf = p->frame[f];
    DOUT2("    " << f << " (" << frameAsTimer(f,s.timer) << ") P1 "
      << Action::name[pf.action_pre] << " "
      << " -> "
      << " " << Action::name[pf.action_post]
      << std::endl);
    SlippiFrame of = o->frame[f];
    DOUT2("    " << f << " (" << frameAsTimer(f,s.timer) << ") P2 "
      << Action::name[of.action_pre] << " "
      << " -> "
      << " " << Action::name[of.action_post]
      << std::endl);
  }
}

void Analyzer::countPhantoms(const SlippiReplay &s, Analysis *a) const {
  const SlippiPlayer *p         = &(s.player[a->ap[0].port]);
  const SlippiPlayer *o         = &(s.player[a->ap[1].port]);
  for (unsigned f = FIRST_FRAME; f < s.frame_count; ++f) {
    if (wasHitByPhantom(*p,*o,f)) {
      ++(a->ap[1].phantom_hits);
    }
    if (wasHitByPhantom(*o,*p,f)) {
      ++(a->ap[0].phantom_hits);
    }
  }
}

Analysis* Analyzer::analyze(const SlippiReplay &s) {
  DOUT1("Analyzing replay" << std::endl);

  Analysis *a = new Analysis(s.frame_count);  //Structure for holding the analysis so far

  //Verify this is a 1 v 1 match; can't analyze otherwise
  if (not get1v1Ports(s,a)) {
    std::cerr << "  Not a two player match; refusing to analyze further" << std::endl;
    a->success = false;
    return a;
  }

  DOUT1("  Analyzing basic game info" << std::endl);
  getBasicGameInfo(s,a);

  //Interaction-level stats
  DOUT1("  Analyzing player interactions" << std::endl);
  analyzeInteractions(s,a);
  DOUT1("  Summarizing player interactions" << std::endl);
  summarizeInteractions(s,a);
  DOUT1("  Analyzing players' moves" << std::endl);
  analyzeMoves(s,a);
  DOUT1("  Analyzing players' punishes" << std::endl);
  analyzePunishes(s,a);

  //Player-level stats
  DOUT1("  Computing statistics based on animations" << std::endl);
  countBasicAnimations(s,a);
  DOUT1("  Computing air / ground time for each player" << std::endl);
  computeAirtime(s,a);
  DOUT1("  Counting l cancels" << std::endl);
  countLCancels(s,a);
  DOUT1("  Counting techs" << std::endl);
  countTechs(s,a);
  DOUT1("  Counting dashdances" << std::endl);
  countDashdances(s,a);
  DOUT1("  Counting airdodges, wavelands, and wavedashes" << std::endl);
  countAirdodgesAndWavelands(s,a);
  DOUT1("  Counting phantom hits" << std::endl);
  countPhantoms(s,a);

  //Debug stuff
  if (_debug >= 2) {
    DOUT2("  Showing action states" << std::endl);
    showActionStates(s,a);
  }

  a->success = true;
  DOUT1("Successfully analyzed replay!" << std::endl);
  return a;
}

}
