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
  SlippiPlayer *p = &(s.player[ports[0]]);
  SlippiPlayer *o = &(s.player[ports[1]]);

  unsigned all_dynamics[s.frame_count] = {0};                  // Dynamic active during each frame
  unsigned dyn_counts[Dynamic::__LAST] = {0};                  // Number of total frames each dynamic has been observed
  unsigned cur_dynamic                 = Dynamic::POSITIONING; // Current dynamic in effect
  // unsigned cur_dynamic_start           = 0;                    // Frame current dynamic started to be in effect
  // unsigned cur_dynamic_frames          = 0;                    // # of frames current dynamic has been in effect
  // unsigned last_dynamic                = Dynamic::NEUTRAL;     // Last dynamic that was in effect
  // unsigned last_dynamic_start          = 0;                    // Frame last dynamic started to be in effect
  // unsigned last_dynamic_frames         = 0;                    // # of frames last dynamic was in effect


  //All interactions analyzed from perspective of p (lower port player)
  for (unsigned f = START_FRAMES+PLAYABLE_FRAME; f < s.frame_count; ++f) {
    // std::cout << "Analyzing frame " << int(f)-START_FRAMES << std::endl;

    //Until proven otherwise, we have the same dynamic as before
    unsigned frame_dynamic = cur_dynamic;

    //Make some decisions about the current frame
    bool oHitOffStage = isOffStage(s,o->frame[f]) && isInHitstun(o->frame[f]);  //Check if opponent is in hitstun offstage
    bool pHitOffStage = isOffStage(s,p->frame[f]) && isInHitstun(p->frame[f]);  //Check if we are in hitstun offstage
    bool oAirborne    = isAirborne(o->frame[f]);
    bool pAirborne    = isAirborne(p->frame[f]);
    bool oOnLedge     = isOnLedge(o->frame[f]);
    bool pOnLedge     = isOnLedge(p->frame[f]);

    //[Define offstage as: beyond the stage's ledges OR below the stage's base (i.e., y < 0)]
    //Check for edgeguard situations - you are edgeguarding if ALL of the following conditions hold true:
    //  Your opponent has experienced hitstun while offstage since the last time they touched the ground
    //  Since the above occurred...
    //    ...your opponent has not touched the ground and been actionable (e.g., end of Sheik's recovery not actionable)
    //    ...your opponent has not grabbed ledge
    if (oHitOffStage) {
      frame_dynamic = Dynamic::EDGEGUARDING;
    } else if (pHitOffStage) {
      frame_dynamic = Dynamic::RECOVERING;
    } else {
      switch (cur_dynamic) {
        case Dynamic::PRESSURING:
          if ((not oOnLedge) && (not oAirborne)) {  //If the opponent touches the ground, we're back to neutral
            frame_dynamic = Dynamic::NEUTRAL;
          }
          break;
        case Dynamic::PRESSURED:
          if ((not pOnLedge) && (not pAirborne)) {  //If we touch the ground, we're back to neutral
            frame_dynamic = Dynamic::NEUTRAL;
          }
          break;
        case Dynamic::EDGEGUARDING:
          if (oOnLedge) {  //If we're edgeguarding, and the opponent grabs ledge, they've recovered; we are now just pressuring
            frame_dynamic = Dynamic::PRESSURING;
          } else if (not oAirborne) {  //If they outright hit the ground, we're back to neutral
            frame_dynamic = Dynamic::NEUTRAL;
          }
          break;
        case Dynamic::RECOVERING:
          if (pOnLedge) {  //If we're being edgeguarded, and we grab ledge, we've recovered; we are now just being pressured
            frame_dynamic = Dynamic::PRESSURED;
          } else if (not pAirborne) {  //If we outright hit the ground, we're back to neutral
            frame_dynamic = Dynamic::NEUTRAL;
          }
          // frame_dynamic = Dynamic::NEUTRAL;
          break;
        default:
          break;
      }
    }

    if (frame_dynamic != cur_dynamic) {
      // last_dynamic        = cur_dynamic;
      // last_dynamic_start  = cur_dynamic_start;
      // last_dynamic_frames = cur_dynamic_frames;
      cur_dynamic         = frame_dynamic;
      // cur_dynamic_start   = f;
      // cur_dynamic_frames  = 0;
    }

    //Aggregate results
    all_dynamics[f] = cur_dynamic;  //Set the dynamic for this frame to the current dynamic computed
    ++dyn_counts[cur_dynamic];      //Increase the counter for dynamics across all frames
    // usleep(1000000);                //Sleep for a bit to make debugging easier
  }

  std::cout << "    From the perspective of port " << int(ports[0]+1) << " (" << CharInt::name[s.player[ports[0]].ext_char_id] << "):" << std::endl;
  for (unsigned i = 0; i < Dynamic::__LAST; ++i) {
    std::cout << "      " << dyn_counts[i] << " frames spent in " << Dynamic::name[i] << std::endl;
  }

}

}
