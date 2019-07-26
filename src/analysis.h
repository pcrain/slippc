
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
  unsigned start_frame;
  unsigned end_frame;
  float    start_pct;
  float    end_pct;
  unsigned num_moves;
  uint8_t  last_move_id;
  int      opening;   // TODO: make an enum for this
  int      kill_dir;  //-1 = no kill, other directions as specified in Dir enum
};

//Struct for holding analysis data for a particular player within a game
struct AnalysisPlayer {
  std::string  tag_css;
  std::string  tag_player;
  std::string  char_name;
  unsigned     char_id;
  unsigned     port;
  unsigned     airdodges;
  unsigned     spotdodges;
  unsigned     rolls;
  unsigned     dashdances;
  unsigned     l_cancels_hit;
  unsigned     l_cancels_missed;
  unsigned     techs;
  unsigned     walltechs;
  unsigned     walljumps;
  unsigned     walltechjumps;
  unsigned     missed_techs;
  unsigned     ledge_grabs;
  unsigned     air_frames;
  unsigned     end_stocks;
  unsigned     end_pct;
  unsigned     wavedashes;
  unsigned     wavelands;
  unsigned     neutral_wins;
  unsigned     pokes;
  unsigned     counters;
  unsigned     dyn_counts[Dynamic::__LAST];  //Frame counts for player interaction dynamics
  Punish*      punishes;

  AnalysisPlayer() {
    punishes = new Punish[MAX_PUNISHES];
  }
  ~AnalysisPlayer() {
    delete punishes;
  }
};

//Struct for holding all analysis data within a game
struct Analysis {
  bool           success;
  std::string    game_time;
  std::string    analyzer_version;
  std::string    stage_name;
  unsigned       stage_id;
  unsigned       winner_port;
  unsigned       total_frames;
  unsigned*      dynamics;  //Interaction dynamics on a per_frame basis
  AnalysisPlayer ap[2];

  ~Analysis() {
    delete dynamics;
  }

  std::string asJson();
};

}

#endif /* ANALYSIS_H_ */
