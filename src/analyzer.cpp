#include "analyzer.h"

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
  DOUT1("  Players found on ports " << a->ap[0].port << " and " << a->ap[1].port);
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
  a->original_file     = s.original_file;
  a->slippi_version    = s.slippi_version;
  a->parser_version    = s.parser_version;
  a->analyzer_version  = SLIPPC_VERSION;
  a->parse_errors      = s.errors;
  a->game_time         = s.start_time;
  a->game_length       = s.frame_count;
  a->timer             = s.timer;
  a->end_type          = s.end_type;
  a->lras_player       = s.lras;
  a->ap[0].tag_player  = s.player[a->ap[0].port].tag;
  a->ap[1].tag_player  = s.player[a->ap[1].port].tag;
  a->ap[0].tag_css     = s.player[a->ap[0].port].tag_css;
  a->ap[1].tag_css     = s.player[a->ap[1].port].tag_css;
  a->ap[0].tag_code    = s.player[a->ap[0].port].tag_code;
  a->ap[1].tag_code    = s.player[a->ap[1].port].tag_code;

  a->ap[0].start_stocks  = s.player[a->ap[0].port].start_stocks;
  a->ap[1].start_stocks  = s.player[a->ap[1].port].start_stocks;
  a->ap[0].color         = s.player[a->ap[0].port].color;
  a->ap[1].color         = s.player[a->ap[1].port].color;
  a->ap[0].team_id       = s.player[a->ap[0].port].team_id;
  a->ap[1].team_id       = s.player[a->ap[1].port].team_id;
  a->ap[0].player_type   = s.player[a->ap[0].port].player_type;
  a->ap[1].player_type   = s.player[a->ap[1].port].player_type;
  a->ap[0].cpu_level     = s.player[a->ap[0].port].cpu_level;
  a->ap[1].cpu_level     = s.player[a->ap[1].port].cpu_level;

  a->ap[0].end_stocks  = s.player[a->ap[0].port].end_stocks;
  a->ap[1].end_stocks  = s.player[a->ap[1].port].end_stocks;
  a->ap[0].end_pct     = s.player[a->ap[0].port].frame[s.frame_count-1].percent_pre;
  a->ap[1].end_pct     = s.player[a->ap[1].port].frame[s.frame_count-1].percent_pre;
  a->ap[0].char_id     = s.player[a->ap[0].port].ext_char_id;
  a->ap[1].char_id     = s.player[a->ap[1].port].ext_char_id;
  a->stage_id          = s.stage;
  a->ap[0].char_name   = CharExt::name[a->ap[0].char_id];
  a->ap[1].char_name   = CharExt::name[a->ap[1].char_id];
  a->stage_name        = Stage::name[s.stage];
  a->winner_port       = s.winner_id;
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

void Analyzer::countButtons(const SlippiReplay &s, Analysis *a) const {
  for (unsigned pi = 0; pi < 2; ++pi) {
    SlippiPlayer p        = s.player[a->ap[pi].port];
    uint16_t last_buttons = 0;
    float    last_ax      = 0;
    float    last_ay      = 0;
    float    last_cx      = 0;
    float    last_cy      = 0;
    for(unsigned f = (-LOAD_FRAME); f < s.frame_count; ++f) {
      // Add buttons pressed this frame to button count
      uint16_t cur_buttons    = p.frame[f].buttons;
      uint16_t new_buttons    = cur_buttons&(cur_buttons^last_buttons);
      new_buttons            &= 0x0F70;  //Mask out unused bits
      a->ap[pi].button_count += countBits(new_buttons);
      last_buttons            = cur_buttons;

      // Check analog stick for movement
      float    cur_ax = p.frame[f].joy_x;
      float    cur_ay = p.frame[f].joy_y;
      a->ap[pi].astick_count += checkStickMovement(cur_ax,cur_ay,last_ax,last_ay);
      last_ax = cur_ax;
      last_ay = cur_ay;

      // Check C stick for movement
      float    cur_cx = p.frame[f].c_x;
      float    cur_cy = p.frame[f].c_y;
      a->ap[pi].cstick_count += checkStickMovement(cur_cx,cur_cy,last_cx,last_cy);
      last_cx = cur_cx;
      last_cy = cur_cy;
    }
  }
}

void Analyzer::countTechs(const SlippiReplay &s, Analysis *a) const {
  for (unsigned pi = 0; pi < 2; ++pi) {
    SlippiPlayer p             = s.player[a->ap[pi].port];
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
          if (p.frame[f].action_pre <= Action::DownSpotD) {
            ++techs_missed;
          } else if (p.frame[f].action_pre <= Action::PassiveStandB) {
            ++techs_hit;
          } else if (p.frame[f].action_pre == Action::PassiveWallJump) {
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
    int airdodges  = 0;
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

    // it's possible the logic above outputs a negative airdodge count -> use a minimum as workaround:
    a->ap[pi].airdodges  = airdodges > 0 ? airdodges : 0;
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
  unsigned pLastGrounded               = 0;  //Last frame we touched solid ground'
  bool     oHitThisFrame               = false;
  bool     pHitThisFrame               = false;
  bool     oHitLastFrame               = false;
  bool     pHitLastFrame               = false;

  //All interactions analyzed from perspective of p (lower port player)
  for (unsigned f = (PLAYABLE_FRAME-LOAD_FRAME); f < s.frame_count; ++f) {
    //Important variables for each frame
    SlippiFrame of     = o->frame[f];
    SlippiFrame pf     = p->frame[f];
    oHitLastFrame = oHitThisFrame;
    oHitThisFrame = false;
    bool oInHitstun    = isInHitstun(of);
      if (oInHitstun)    {
        oLastInHitsun = f;
        if (of.percent_post > o->frame[f-1].percent_post) {
          oHitThisFrame = true; //Check if opponent was hit this frame by looking at % last frame
        }
      }
    pHitLastFrame = pHitThisFrame;
    pHitThisFrame = false;
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
    } else if (oShielding) {  //If our opponent is in shield, we are pressureing
      cur_dynamic = Dynamic::PRESSURING;
    } else if (pShielding) {  //If we are in shield, we are pressured
      cur_dynamic = Dynamic::PRESSURED;
    } else if (oTeching) {  //If opponent is teching, we are techchasing
      cur_dynamic = Dynamic::TECHCHASING;
    } else if (pTeching) {  //If we are teching, we are escaping a techchase
      cur_dynamic = Dynamic::ESCAPING;
    } else if (oInHitstun && pInHitstun) {  //If we're both in hitstun and neither of us are offstage, it's a trade
      // cur_dynamic = Dynamic::TRADING;
      cur_dynamic = Dynamic::POKING; //Update 2019-11-05: trading is no longer a used dynamic; now just poking
    } else {  //Everything else depends on the cur_dynamic
      switch (cur_dynamic) {
        case Dynamic::PRESSURING:
          if (oHitLastFrame && not oIsGrabbed) { //If opponent was actually hit last frame by a non pummel, we're punishing
            cur_dynamic = Dynamic::PUNISHING;
          } else if (oThrown) {  //If the opponent is being thrown, this is a techchase opportunity
            cur_dynamic = Dynamic::TECHCHASING;
          } else if ((not oInHitstun) && (not oShielding) && (not oOnLedge) && (not oAirborne)) {  //If the opponent touches the ground w/o shielding, we're back to neutral
            cur_dynamic = neut_dyn;
          }
          break;
        case Dynamic::PRESSURED:
          if (pHitLastFrame && not pIsGrabbed) { //If we were actually hit last frame by a non pummel, we're being punishied
            cur_dynamic = Dynamic::PUNISHED;
          } else if (pThrown) {  //If we are being thrown, opponent has a techchase opportunity
            cur_dynamic = Dynamic::ESCAPING;
          } else if ((not pInHitstun) && (not pShielding) && (not pOnLedge) && (not pAirborne)) {  //If we touch the ground w/o shielding, we're back to neutral
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
          if (oHitLastFrame && (not oTeching) && oAirborne) {
            cur_dynamic = Dynamic::PUNISHING; //If we started comboing an opponent after the techchase, it's a punish
          } else if ((not oInHitstun) && (not oIsGrabbed) && (not oThrown) && (not oTeching) && (not oShielding)) {
            cur_dynamic = oAirborne ? Dynamic::SHARKING : neut_dyn;  //If the opponent escaped our tech chase, back to neutral it is
          }
          break;
        case Dynamic::ESCAPING:
          if (pHitLastFrame && (not pTeching) && pAirborne) {
            cur_dynamic = Dynamic::PUNISHED; //If we started comboing an opponent after the techchase, it's a punish
          } else if ((not pInHitstun) && (not pIsGrabbed) && (not pThrown) && (not pTeching) && (not pShielding)) {
            cur_dynamic = pAirborne ? Dynamic::GROUNDING : neut_dyn;  //If we escaped our opponent's tech chase, back to neutral it is
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
          if (oHitLastFrame) {
            cur_dynamic = Dynamic::PUNISHING;  //If we hit our opponent LAST frame, we're comboing again
          } else if (pInHitstun) { //If our opponent hits us while sharking, we're back to poking
            cur_dynamic = Dynamic::POKING;
          } else if (not oAirborne) {  //If our opponent has outright landed, we're back to neutral
            cur_dynamic = neut_dyn;
          }
          break;
        case Dynamic::GROUNDING:
          if (pHitLastFrame) {
            cur_dynamic = Dynamic::PUNISHED;  //If our opponent hit us LAST frame, we're being comboed again
          } else if (oInHitstun) { //If we hit our opponent while grounding, we're back to poking
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
    DOUT3("    " << f << " (" << frameAsTimer(f,s.timer) << ") P1 "
      << Dynamic::name[cur_dynamic]);
  }

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
  unsigned last_dyn      = a->dynamics[FIRST_FRAME];
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

    //Check if we counter-attacked (switched from defense to offense or vice versa)
    if (cur_dyn <= Dynamic::DEFENSIVE && last_dyn >= Dynamic::OFFENSIVE) {
      ++a->ap[1].counters;  //If we went from offense to defense, we were countered
    } else if (cur_dyn >= Dynamic::OFFENSIVE && last_dyn <= Dynamic::DEFENSIVE) {
      ++a->ap[0].counters;  //If we went from defense to offense, we countered
    }

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
    if (pPunishEnd && pa > 0) {
      if (pPunishes[pn].num_moves > 0) {
        pPunishes[pn].end_frame    = f;
        pPunishes[pn].end_pct      = of.percent_pre;
        pPunishes[pn].last_move_id = pf.hit_with;
        if(pPunishes[pn].num_moves > pAttacks[pa-1].hit_id) {
          a->ap[0].neutral_wins += 1;
        } else {
          a->ap[0].pokes += 1;
        }
        ++pn;
      }
    }
    if (oPunishEnd && oa > 0) {
      if (oPunishes[on].num_moves > 0) {
        oPunishes[on].end_frame    = f;
        oPunishes[on].end_pct      = pf.percent_pre;
        oPunishes[on].last_move_id = of.hit_with;
        if(oPunishes[on].num_moves > oAttacks[oa-1].hit_id) {
          a->ap[1].neutral_wins += 1;
        } else {
          a->ap[1].pokes += 1;
        }
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
        ++(a->ap[0].move_counts[pAttacks[pa].move_id]);
      }
      pAttacks[pa].frame      = f;
      pAttacks[pa].damage     = o_damage_taken;
      a->ap[0].dyn_damage[cur_dyn] += o_damage_taken; //Dynamic is normal for player
      pAttacks[pa].opening    = cur_dyn;  //Dynamic is normal for player
      pAttacks[pa].kill_dir   = Dir::NEUT;
      ++pa;
      //If this is the start of a combo
      if (pPunishes[pn].num_moves == 0) {
        pPunishes[pn].start_frame = f;
        pPunishes[pn].start_pct   = o->frame[f-1].percent_pre;
        pPunishes[pn].stocks      = o->frame[f-1].stocks;
        pPunishes[pn].kill_dir    = Dir::NEUT;
      }
      a->ap[0].damage_dealt      += o_damage_taken;
      pPunishes[pn].end_frame     = f;
      pPunishes[pn].end_pct       = of.percent_pre;
      pPunishes[pn].last_move_id  = pf.hit_with;
      pPunishes[pn].num_moves    += 1;
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
        ++(a->ap[1].move_counts[oAttacks[oa].move_id]);
      }
      oAttacks[oa].frame      = f;
      oAttacks[oa].damage     = p_damage_taken;
      //Dynamic needs to be flipped around for opponent
      int oDynamic = (cur_dyn > Dynamic::OFFENSIVE || cur_dyn < Dynamic::DEFENSIVE) ? Dynamic::__LAST-cur_dyn : cur_dyn;
      a->ap[1].dyn_damage[oDynamic] += p_damage_taken; //Dynamic needs to be flipped around for opponent
      oAttacks[oa].opening    =  oDynamic;
      oAttacks[oa].kill_dir   = Dir::NEUT;
      ++oa;
      //If this is the start of a combo
      if (oPunishes[on].num_moves == 0) {
        oPunishes[on].start_frame = f;
        oPunishes[on].start_pct   = p->frame[f-1].percent_pre;
        oPunishes[on].stocks      = p->frame[f-1].stocks;
        oPunishes[on].kill_dir    = Dir::NEUT;
      }
      a->ap[1].damage_dealt      += p_damage_taken;
      oPunishes[on].end_frame     = f;
      oPunishes[on].end_pct       = pf.percent_pre;
      oPunishes[on].last_move_id  = of.hit_with;
      oPunishes[on].num_moves    += 1;
    }

    last_dyn = cur_dyn;  //Update the last dynamic
  }
}

void Analyzer::analyzeCancels(const SlippiReplay &s, Analysis *a) const {
  for(unsigned pi = 0; pi < 2; ++pi) {
    const SlippiPlayer *p = &(s.player[a->ap[pi].port]);
    Attack *attacks       = a->ap[pi].attacks;
    for(unsigned i = 0; attacks[i].frame > 0; ++i) {
      unsigned f             = attacks[i].frame;
      if (attacks[i].move_id >= Move::NAIR && attacks[i].move_id <= Move::DAIR) {
        unsigned last_frame = attacks[i].frame+60;
        if (last_frame > s.frame_count) {
          last_frame = s.frame_count;
        }
        for(unsigned f = attacks[i].frame; f < last_frame; ++f) {
          //Check if we had the opportunity to edge cancel
          if (didEdgeCancelAerial(p->frame[f])) {
            attacks[i].cancel_type = Cancel::EDGE;
            break;
          }
          //Check if we had the opportunity to teeter cancel
          if (didTeeterCancelAerial(p->frame[f])) {
            attacks[i].cancel_type = Cancel::TEETER;
            break;
          }
          //Check if we had the opportunity to autocancel
          if (didAutoCancelAerial(p->frame[f])) {
            attacks[i].cancel_type = Cancel::AUTO;
            break;
          }
          //Check if we had the opportunity to L cancel
          if (p->frame[f].l_cancel > 0) {
            if (p->frame[f].l_cancel == 1) {
              attacks[i].cancel_type = Cancel::L;
            }
            break;
          }
        }
      }
    }
  }
}

void Analyzer::analyzeShield(const SlippiReplay &s, Analysis *a) const {
  for(unsigned pi = 0; pi < 2; ++pi) {
    const SlippiPlayer *p = &(s.player[a->ap[pi].port]);
    for(unsigned f = (-LOAD_FRAME); f < s.frame_count; ++f) {
      float shield_damage = (p->frame[f].shield - p->frame[f-1].shield);
      if (shield_damage > 0) {
        a->ap[pi].shield_time   += 1;
        a->ap[pi].shield_damage += shield_damage;
        if(p->frame[f].shield < a->ap[pi].shield_lowest) {
          a->ap[pi].shield_lowest = p->frame[f].shield;
        }
      }
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
    a->ap[pi].state_changes          = countTransitions(s,a,pi,didActionStateChange);

    a->ap[pi].ledge_grabs            = countTransitions(s,a,pi,isOnLedge);
    a->ap[pi].rolls                  = countTransitions(s,a,pi,isRolling);
    a->ap[pi].spotdodges             = countTransitions(s,a,pi,isSpotdodging);
    a->ap[pi].powershields           = countTransitions(s,a,pi,didPowerShield);
    a->ap[pi].grabs                  = countTransitions(s,a,pi,isGrabbing);
    a->ap[pi].taunts                 = countTransitions(s,a,pi,isTaunting);
    a->ap[pi].meteor_cancels         = countTransitions(s,a,pi,didMeteorCancel);
    a->ap[pi].hits_blocked           = countTransitions(s,a,pi,isInShieldstun);
    a->ap[pi].edge_cancel_aerials    = countTransitions(s,a,pi,didEdgeCancelAerial);
    a->ap[pi].edge_cancel_specials   = countTransitions(s,a,pi,didEdgeCancelSpecial);
    a->ap[pi].teeter_cancel_aerials  = countTransitions(s,a,pi,didTeeterCancelAerial);
    a->ap[pi].teeter_cancel_specials = countTransitions(s,a,pi,didTeeterCancelSpecial);
    a->ap[pi].no_impact_lands        = countTransitions(s,a,pi,didNoImpactLand);
    a->ap[pi].shield_drops           = countTransitions(s,a,pi,didShieldDrop);
    a->ap[pi].pivots                 = countTransitions(s,a,pi,didPivot);
    a->ap[pi].short_hops             = countTransitions(s,a,pi,didShortHop);
    a->ap[pi].full_hops              = countTransitions(s,a,pi,didHop)-a->ap[pi].short_hops;

    a->ap[pi].shield_breaks          = countTransitions(s,a,1-pi,isShieldBroken);
    a->ap[pi].grab_escapes           = countTransitions(s,a,1-pi,isReleasing);
    a->ap[pi].shield_stabs           = countTransitions(s,a,1-pi,wasShieldStabbed);
    a->ap[pi].stage_spikes           = countTransitions(s,a,1-pi,wasStageSpiked);
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
      << " " << Action::name[pf.action_post]);
    SlippiFrame of = o->frame[f];
    DOUT2("    " << f << " (" << frameAsTimer(f,s.timer) << ") P2 "
      << Action::name[of.action_pre] << " "
      << " -> "
      << " " << Action::name[of.action_post]);
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

void Analyzer::countLedgedashes(const SlippiReplay &s, Analysis *a) const {
  for (unsigned pi = 0; pi < 2; ++pi) {
    const SlippiPlayer *p  = &(s.player[a->ap[pi].port]);
    unsigned galint        = 0;
    bool     offledge      = false;
    for (unsigned f = FIRST_FRAME; f < s.frame_count; ++f) {
      if (galint > 0) {
        --galint;
        if (not offledge) {
          offledge = didReleaseLedge(*p,f);
          continue;
        }
        SlippiFrame pf = p->frame[f];
        bool landed    = isLanding(p->frame[f-1]) && (not isLanding(pf)) && (not isAirborne(pf));
        if ((didNoImpactLand(pf) || landed) && (not isInHitlag(pf)) && (not isInHitstun(pf))) {
          if (a->ap[pi].max_galint < galint) {
            a->ap[pi].max_galint        = galint;
          }
          a->ap[pi].galint_ledgedashes += 1;
          a->ap[pi].mean_galint        += galint;
          f                            += galint;
          galint                        = 0;
          offledge                      = false;
        }
      }
      if(didCliffCatchEnd(*p,f)) {
        galint        = 30;
        if (didReleaseLedge(*p,f)) {
          offledge = true;
        }
      }
    }
    if (a->ap[pi].galint_ledgedashes > 0) {
      a->ap[pi].mean_galint /= a->ap[pi].galint_ledgedashes;
    }
  }
}

void Analyzer::countMoves(const SlippiReplay &s, Analysis *a) const {
  for (unsigned pi = 0; pi < 2; ++pi) {
    const SlippiPlayer *p  = &(s.player[a->ap[pi].port]);
    const unsigned pid = p->ext_char_id;
    bool was_throw   = false;
    bool was_normal  = false;
    bool was_special = false;
    bool was_misc    = false;
    bool was_grab    = false;
    bool was_pummel  = false;
    for (unsigned f = FIRST_FRAME; f < s.frame_count; ++f) {
      SlippiFrame pf = p->frame[f];
      if (isThrowing(pf)) {
        if (!(was_throw)) {
          ++(a->ap[pi].used_throws);
        }
        was_throw = true;
      } else if (isUsingNormalMove(pf)) {
        if (!(was_normal)) {
          ++(a->ap[pi].used_norm_moves);
        }
        was_normal = true;
      } else if (isUsingSpecialMove(pf,pid)) {
        if (!(was_special)) {
          ++(a->ap[pi].used_spec_moves);
        }
        was_special = true;
      } else if (isUsingMiscMove(pf)) {
        if (!(was_misc)) {
          ++(a->ap[pi].used_misc_moves);
        }
        was_misc = true;
      } else if (isUsingGrab(pf)) {
        if (!(was_grab)) {
          ++(a->ap[pi].used_grabs);
        }
        was_grab = true;
      } else if (isUsingPummel(pf)) {
        if (!(was_pummel)) {
          ++(a->ap[pi].used_pummels);
        }
        was_pummel = true;
      } else {
        was_throw   = false;
        was_normal  = false;
        was_special = false;
        was_misc    = false;
        was_grab    = false;
        was_pummel  = false;
      }
    }
    a->ap[pi].total_moves_used = a->ap[pi].used_throws + a->ap[pi].used_norm_moves
      + a->ap[pi].used_spec_moves + a->ap[pi].used_misc_moves + a->ap[pi].used_grabs + a->ap[pi].used_pummels;
    // std::cout << "Player " << pi+1 << " attempted: " << std::endl;
    // std::cout << "  " <<  a->ap[pi].used_throws << " used_throws" << std::endl;
    // std::cout << "  " <<  a->ap[pi].used_norm_moves << " used_norm_moves" << std::endl;
    // std::cout << "  " <<  a->ap[pi].used_spec_moves << " used_spec_moves" << std::endl;
    // std::cout << "  " <<  a->ap[pi].used_misc_moves << " used_misc_moves" << std::endl;
    // std::cout << "  " <<  a->ap[pi].used_grabs << " used_grabs" << std::endl;
    // std::cout << "  " <<  a->ap[pi].used_pummels << " used_pummels" << std::endl;
    // std::cout << "  " <<  a->ap[pi].total_moves_used << " used_total_moves" << std::endl;
  }
}

void Analyzer::countActionability(const SlippiReplay &s, Analysis *a) const {
  for (unsigned pi = 0; pi < 2; ++pi) {
    const SlippiPlayer *p  = &(s.player[a->ap[pi].port]);
    bool     was_in_hitstun     = false;
    unsigned hitstun_times      = 0; //Number of times we enter hitstun
    unsigned hitstun_act        = 0; //Total number of frames we take to act out of hitstun
    unsigned hitstun_act_cur    = 0; //Current number of frames we take to act out of hitstun
    bool     was_in_shieldstun  = false;
    unsigned shieldstun_times   = 0; //Number of times we enter shieldstun
    unsigned shieldstun_act     = 0; //Total number of frames we take to act out of shieldstun
    unsigned shieldstun_act_cur = 0; //Current number of frames we take to act out of shieldstun
    bool     was_in_wait        = false;
    unsigned wait_times         = 0; //Number of times we enter wait
    unsigned wait_act           = 0; //Total number of frames we take to act out of wait
    unsigned wait_act_cur       = 0; //Current number of frames we take to act out of wait
    for (unsigned f = FIRST_FRAME; f < s.frame_count; ++f) {
      SlippiFrame pf = p->frame[f];

      // Count the number of frames we take to act out of hitstun
      if (isInHitstun(pf)) {
        if (! was_in_hitstun) {
          was_in_hitstun = true;
          ++hitstun_times;
          hitstun_act_cur = 0;
        }
      } else if (was_in_hitstun) {
        if (
         inTumble(pf)            ||
         isInDamageAnimation(pf) ||
         isInAnyWait(pf)) {
          ++hitstun_act_cur;
          ++hitstun_act;
        } else {
          if ((MAX_WAIT < hitstun_act_cur) || wasStageSpiked(pf) || inMissedTechState(pf)) {
            --hitstun_times;  //We're not trying to act out of stun
          } else {
            // std::cout << "hitstun act " << hitstun_act_cur << std::endl;
            hitstun_act += hitstun_act_cur;
          }
          was_in_hitstun = false;
        }
      }

      // Count the number of frames we take to act out of shieldstun
      unsigned max_shield_wait = MAX_WAIT;
      if (isInShieldstun(pf)) {
        if (! was_in_shieldstun) {
          was_in_shieldstun = true;
          ++shieldstun_times;
          shieldstun_act_cur = 0;
        } else if (shieldstun_act_cur > 0) {
          //We might be waiting in shield due to shield pressure,
          //  so break the MAX_WAIT limit in these cases
          max_shield_wait = MAX_WAIT + shieldstun_act_cur;
        }
      } else if (was_in_shieldstun) {
        if (isInShield(pf)) {
          ++shieldstun_act_cur;
        } else {
          if ((max_shield_wait < shieldstun_act_cur) ) {
            --shieldstun_times;  //We're not trying to act out of stun
          } else {
            // std::cout << "shieldstun act " << shieldstun_act_cur << std::endl;
            shieldstun_act += shieldstun_act_cur;
          }
          was_in_shieldstun = false;
        }
      }

      // Count the number of frames we take to act out of shieldstun
      if (isInAnyWait(pf)) {
        if (! was_in_wait) {
          was_in_wait = true;
          ++wait_times;
          wait_act_cur = 0;
        } else {
          ++wait_act_cur;
        }
      } else if (was_in_wait) {
        if ((MAX_WAIT < wait_act_cur) ) {
          --wait_times;  //We're not trying to act out of wait
        } else {
          // std::cout << "wait act " << wait_act_cur << std::endl;
          wait_act += wait_act_cur;
        }
        was_in_wait = false;
      }

    }

    a->ap[pi].hitstun_times         = hitstun_times;
    a->ap[pi].hitstun_act_frames    = hitstun_act;
    a->ap[pi].shieldstun_times      = shieldstun_times;
    a->ap[pi].shieldstun_act_frames = shieldstun_act;
    a->ap[pi].wait_times            = wait_times;
    a->ap[pi].wait_act_frames       = wait_act;
  }
}

void Analyzer::computeTrivialInfo(const SlippiReplay &s, Analysis *a) const {
  for (unsigned pi = 0; pi < 2; ++pi) {
    // Get damage per opening
    a->ap[pi].total_openings       = a->ap[pi].neutral_wins + a->ap[pi].pokes;
    if (a->ap[pi].total_openings > 0) {
      a->ap[pi].mean_opening_percent = a->ap[pi].damage_dealt / a->ap[pi].total_openings;
    }

    // Get openings per kill and percent per kill
    unsigned o_end_stocks = a->ap[1-pi].end_stocks;
    unsigned o_stocks_lost = a->ap[1-pi].start_stocks - o_end_stocks;
    if (o_stocks_lost > 0) {
      a->ap[pi].mean_kill_openings = float(a->ap[pi].total_openings) / o_stocks_lost;
      float damage_dealt = a->ap[pi].damage_dealt;
      if (o_end_stocks > 0) {
        //Need to not include stocks we didn't take in our computation
        damage_dealt -= a->ap[1-pi].end_pct;
      }
      a->ap[pi].mean_kill_percent    = damage_dealt / o_stocks_lost;
      a->ap[1-pi].mean_death_percent = a->ap[pi].mean_kill_percent;
    }

    // Get actions per minute
    unsigned total_actions =
      a->ap[pi].button_count + a->ap[pi].cstick_count + a->ap[pi].astick_count;
    a->ap[pi].apm        = total_actions * (3600.0f / a->game_length);

    // Get action states per minute
    a->ap[pi].aspm       = a->ap[pi].state_changes * (3600.0f / a->game_length);

    // Get total number of moves landed and move accuracy
    a->ap[pi].total_moves_landed = 0;
    for(unsigned i = 0; i < Move::__LAST; ++i) {
      a->ap[pi].total_moves_landed += a->ap[pi].move_counts[i];
    }
    if (a->ap[pi].total_moves_used > 0) {
      a->ap[pi].move_accuracy = float(a->ap[pi].total_moves_landed) / float(a->ap[pi].total_moves_used);
    }

    // Get average actionability
    a->ap[pi].actionability = 0;
    if (a->ap[pi].shieldstun_times > 0) {
      a->ap[pi].actionability += float(a->ap[pi].shieldstun_act_frames) / a->ap[pi].shieldstun_times;
    }
    if (a->ap[pi].hitstun_times > 0) {
      a->ap[pi].actionability += float(a->ap[pi].hitstun_act_frames) / a->ap[pi].hitstun_times;
    }
    if (a->ap[pi].wait_times > 0) {
      a->ap[pi].actionability += float(a->ap[pi].wait_act_frames) / a->ap[pi].wait_times;
    }
    a->ap[pi].actionability /= 3.0f;

    // Get number of times we won neutral relative to time spent in neutral / defense
    unsigned neut_frames =
      a->ap[pi].dyn_counts[Dynamic::TRADING]     +
      a->ap[pi].dyn_counts[Dynamic::POKING]      +
      a->ap[pi].dyn_counts[Dynamic::NEUTRAL]     +
      a->ap[pi].dyn_counts[Dynamic::POSITIONING] +
      a->ap[pi].dyn_counts[Dynamic::FOOTSIES]    +
      a->ap[pi].dyn_counts[Dynamic::RECOVERING]  +
      a->ap[pi].dyn_counts[Dynamic::ESCAPING]    +
      a->ap[pi].dyn_counts[Dynamic::PUNISHED]    +
      a->ap[pi].dyn_counts[Dynamic::GROUNDING]   +
      a->ap[pi].dyn_counts[Dynamic::PRESSURED]   +
      a->ap[pi].dyn_counts[Dynamic::DEFENSIVE]
      ;
    if (neut_frames > 0) {
      a->ap[pi].neutral_wins_per_min = a->ap[pi].total_openings / (float(neut_frames) / 3600.0f);
    }
  }
}

Analysis* Analyzer::analyze(const SlippiReplay &s) {
  DOUT1("  Analyzing replay");

  Analysis *a = new Analysis(s.frame_count);  //Structure for holding the analysis so far

  //Verify this is a 1 v 1 match; can't analyze otherwise
  if (not get1v1Ports(s,a)) {
    FAIL("    Not a two player match; refusing to analyze further");
    a->success = false;
    return a;
  }

  DOUT1("    Analyzing basic game info");
  getBasicGameInfo(s,a);

  //Interaction-level stats
  DOUT1("    Analyzing player interactions");
  analyzeInteractions(s,a);
  DOUT1("    Summarizing player interactions");
  summarizeInteractions(s,a);
  DOUT1("    Analyzing players' punishes");
  analyzePunishes(s,a);
  DOUT1("    Analyzing players' cancelling techniques");
  analyzeCancels(s,a);
  DOUT1("    Analyzing players' shielding");
  analyzeShield(s,a);

  //Player-level stats
  DOUT1("    Computing statistics based on animations");
  countBasicAnimations(s,a);
  DOUT1("    Computing air / ground time for each player");
  computeAirtime(s,a);
  DOUT1("    Counting l cancels");
  countLCancels(s,a);
  DOUT1("    Counting techs");
  countTechs(s,a);
  DOUT1("    Counting dashdances");
  countDashdances(s,a);
  DOUT1("    Counting airdodges, wavelands, and wavedashes");
  countAirdodgesAndWavelands(s,a);
  DOUT1("    Counting phantom hits");
  countPhantoms(s,a);
  DOUT1("    Counting button pushes");
  countButtons(s,a);
  DOUT1("    Counting ledgedashes");
  countLedgedashes(s,a);
  DOUT1("    Counting act out of wait / stun");
  countActionability(s,a);
  DOUT1("    Counting number of moves used");
  countMoves(s,a);

  DOUT1("    Computing trivial match info");
  computeTrivialInfo(s,a);

  //Debug stuff
  if (_debug >= 2) {
    DOUT2("    Showing action states");
    showActionStates(s,a);
  }

  a->success = true;
  DOUT1("  Successfully analyzed replay!");
  return a;
}

}
