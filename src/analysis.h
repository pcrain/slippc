
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
  unsigned num_moves    = 0;  //Number of moves in the current punish
  unsigned start_frame  = 0;  //First frame of punish (0 == internal frame -123)
  unsigned end_frame    = 0;  //Last frame of punish (0 == internal frame -123)
  float    start_pct    = 0;  //Opponent's damage on first frame of punish
  float    end_pct      = 0;  //Opponent's damage on last frame of punish
  uint8_t  last_move_id = 0;  //ID of the last move that hit
  int      opening      = 0;  //Opening type for first hit of punish (TODO: make an enum for this)
  int      kill_dir     = 0;  //Direction of kill, as specified in Dir enum (-1 = didn't kill)
};

//Struct for holding analysis data for a particular player within a game
struct AnalysisPlayer {
  unsigned     port             = 0;  //0-indexed port number of the player (0-3 are valid)
  std::string  tag_player       = ""; //Tag of the player as recorded in the game metadata
  std::string  tag_css          = ""; //Tag of the player as chosen on the char. select screen
  unsigned     char_id          = 0;  //External ID of the selected character
  std::string  char_name        = ""; //Name of the selected character
  unsigned     airdodges        = 0;  //Number of airdodges performed
  unsigned     spotdodges       = 0;  //Number of spotdodges performed
  unsigned     rolls            = 0;  //Number of rolls performed
  unsigned     dashdances       = 0;  //Number of dashdances performed
  unsigned     l_cancels_hit    = 0;  //Number of L-cancels performed
  unsigned     l_cancels_missed = 0;  //Number of L-cancels failed
  unsigned     techs            = 0;  //Number of ground techs performed
  unsigned     walltechs        = 0;  //Number of wall techs performed
  unsigned     walljumps        = 0;  //Number of wall jumps performed
  unsigned     walltechjumps    = 0;  //Number of wall tech jumps performed
  unsigned     missed_techs     = 0;  //Number of (ground) techs missed
  unsigned     ledge_grabs      = 0;  //Number of times we grabbed the ledge
  unsigned     air_frames       = 0;  //Number of frames spent airborne
  unsigned     end_stocks       = 0;  //Stock count on the last frame
  unsigned     end_pct          = 0;  //Damage on the last frame
  unsigned     wavedashes       = 0;  //Number of wavedashes performed
  unsigned     wavelands        = 0;  //Number of wavelands performed
  unsigned     neutral_wins     = 0;  //Number of times we won neutral
  unsigned     pokes            = 0;  //Number of times we poked (single hits not leading to punishes)
  unsigned     counters         = 0;  //Number of counterattacks we performed
  unsigned*    dyn_counts;            //Frame counts for player interaction dynamics
  Punish*      punishes;              //List of all punishes we performed throughout the game

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
  bool           success           = false;  //Whether we succeeded analyzing a replay
  std::string     game_time        = "";     //When the game was played, from replay metadata
  std::string     slippi_version   = "";     //Version of Slippi the replay was recorded with
  std::string     parser_version   = "";     //Version of parser the replay was parsed with
  std::string     analyzer_version = "";     //Version of this analyzer we are using
  unsigned        stage_id         = 0;      //Internal ID of the stage
  std::string     stage_name       = "";     //Readable name of the stage
  int             winner_port      = 0;      //Port index of the winning player (-1 == no winner)
  unsigned        game_length      = 0;      //Length of the game in frames (0 == internal frame -123)
  AnalysisPlayer* ap;                        //Analysis of individual players in the game
  unsigned*      dynamics;                   //Interaction dynamics on a per-frame basis

  Analysis() {
    ap = new AnalysisPlayer[2];
  }
  ~Analysis() {
    delete ap;
    delete dynamics;
  }

  std::string asJson();                      //Convert the analysis structure to a JSON
  void save(const char* outfilename);        //Write the analysis out to a JSON file
};

}

#endif /* ANALYSIS_H_ */
