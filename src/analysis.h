
#ifndef ANALYSIS_H_
#define ANALYSIS_H_

#include <iostream>
#include <fstream>
#include <unistd.h>  //usleep
#include <math.h>    //sqrt

#include "enums.h"
#include "util.h"

const unsigned MAX_PUNISHES  = 255;    //Maximum number of punishes per player per game (increase later if needed)

namespace slip {

//Struct for storing basic punish infomations
struct Punish {
  unsigned num_moves    = 0;
  unsigned start_frame  = 0;
  unsigned end_frame    = 0;
  float    start_pct    = 0;
  float    end_pct      = 0;
  uint8_t  last_move_id = 0;
  int      opening      = 0;  // TODO: make an enum for this
  int      kill_dir     = 0;  //-1 = no kill, other directions as specified in Dir enum
};

//Struct for holding analysis data for a particular player within a game
struct AnalysisPlayer {
  unsigned     port = 0;
  std::string  tag_player       = "";
  std::string  tag_css          = "";
  unsigned     char_id          = 0;
  std::string  char_name        = "";
  unsigned     airdodges        = 0;
  unsigned     spotdodges       = 0;
  unsigned     rolls            = 0;
  unsigned     dashdances       = 0;
  unsigned     l_cancels_hit    = 0;
  unsigned     l_cancels_missed = 0;
  unsigned     techs            = 0;
  unsigned     walltechs        = 0;
  unsigned     walljumps        = 0;
  unsigned     walltechjumps    = 0;
  unsigned     missed_techs     = 0;
  unsigned     ledge_grabs      = 0;
  unsigned     air_frames       = 0;
  unsigned     end_stocks       = 0;
  unsigned     end_pct          = 0;
  unsigned     wavedashes       = 0;
  unsigned     wavelands        = 0;
  unsigned     neutral_wins     = 0;
  unsigned     pokes            = 0;
  unsigned     counters         = 0;
  unsigned*    dyn_counts;  //Frame counts for player interaction dynamics
  Punish*      punishes;

  AnalysisPlayer() {
    dyn_counts = new unsigned[Dynamic::__LAST]{0};
    punishes   = new Punish[MAX_PUNISHES];
  }
  ~AnalysisPlayer() {
    delete dyn_counts;
    delete punishes;
  }
};

//Struct for holding all analysis data within a game
struct Analysis {
  bool           success           = false;
  std::string     game_time        = "";
  std::string     slippi_version   = "";
  std::string     parser_version   = "";
  std::string     analyzer_version = "";
  std::string     stage_name       = "";
  unsigned        stage_id         = 0;
  unsigned        winner_port      = 0;
  unsigned        game_length      = 0;
  AnalysisPlayer* ap;
  unsigned*      dynamics;  //Interaction dynamics on a per_frame basis

  Analysis() {
    ap = new AnalysisPlayer[2];
  }
  ~Analysis() {
    delete ap;
    delete dynamics;
  }

  std::string asJson();
  void save(const char* outfilename);
};

}

#endif /* ANALYSIS_H_ */
