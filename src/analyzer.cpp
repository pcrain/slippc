#include "analyzer.h"

namespace slip {

Analyzer::Analyzer() {
  //Nothing to do yet
}

Analyzer::~Analyzer() {
  //Nothing to do yet
}

void Analyzer::analyze(SlippiReplay &s, std::ostream* _dout) {
  (*_dout) << "Analyzing replay\n----------\n" << std::endl;

  // std::cout << "  Generic Data" << std::endl;
  unsigned num_players = 0;
  unsigned p[2];
  std::string chars[2];
  for(unsigned i = 0 ; i < 4; ++i) {
    if (s.player[i].player_type != 3) {
      if (num_players == 2) {
        std::cerr << "  Not a two player match; refusing to parse further" << std::endl;
        return;
      }
      p[num_players++] = i;
    }
  }
  (*_dout) << "Players found on ports " << p[0] << " and " << p[1] << std::endl;

  chars[0] = CharInt::name[s.player[p[0]].ext_char_id];
  chars[1] = CharInt::name[s.player[p[1]].ext_char_id];
  std::cout
    << chars[0]
    << " ("
    << std::to_string(s.player[p[0]].frame[s.frame_count-1].stocks)
    << ") vs "
    << chars[1]
    << " ("
    << std::to_string(s.player[p[1]].frame[s.frame_count-1].stocks)
    << ") on "
    << Stage::name[s.stage]
    << std::endl
    ;

  //Per player stats
  for(unsigned i = 0; i < 2 ; ++i) {
    std::cout << "Player " << (i+1) << " (" << chars[i] << "):" << std::endl;

    //Initialize some counters
    unsigned airframes = 0;
    unsigned max_combo = 0;

    //Loop over each frame
    for(unsigned f = START_FRAMES; f < s.frame_count; ++f) {
      airframes += s.player[p[i]].frame[f].airborne;
      uint8_t combo = s.player[p[i]].frame[f].combo;
      max_combo = (combo > max_combo) ? combo : max_combo;
    }

    //Compute statistics
    float aircount = 100*((float)airframes / s.last_frame);

    //Print results
    std::cout << "  Airborne for " << aircount  << "% of match" << std::endl;
    std::cout << "  Max combo of " << max_combo << " hits" << std::endl;
  }

  return;
}

}
