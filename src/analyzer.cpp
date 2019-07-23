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
  for(unsigned i = 0; i < 2 ; ++i) {
    std::cout << "Player " << (i+1) << " (" << chars[i] << "):" << std::endl;
    computeAirtime(s,ports[i]);
    computeMaxCombo(s,ports[i]);
    findAllCombos(s,ports,i);
  }

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
    if (isInHitstun(of) || isInHitlag(of)) {
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
    } else if (isDead(of)) {
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

}
