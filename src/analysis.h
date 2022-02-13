
#ifndef ANALYSIS_H_
#define ANALYSIS_H_

#include <iostream>
#include <fstream>
#include <unistd.h>  //usleep
#include <math.h>    //sqrt

#include "enums.h"
#include "util.h"

const unsigned MAX_ATTACKS   = 65535;    //Maximum number of attacks per player per game (increase later if needed)
const unsigned MAX_PUNISHES  = 65535;    //Maximum number of punishes per player per game (increase later if needed)

namespace slip {

//Struct for storing information about each attack landed
struct Attack {
  uint8_t  move_id      = 0;  //ID of the move that hit
  float    anim_frame   = 0;  //Animation frame when the move hit
  uint8_t  punish_id    = 0;  //Index of the punish this move is a part of (one punish per opening)
  uint8_t  hit_id       = 0;  //Index of the hit number for the current move (applies only to multihits)
  unsigned frame        = 0;  //Frame the move connected (0 == internal frame -123)
  float    damage       = 0;  //Damage dealt by the move
  uint8_t  opening      = 0;  //Opening type for first hit of punish (TODO: make an enum for this)
  uint8_t  kill_dir     = 0;  //Direction of kill, as specified in Dir enum (-1 = didn't kill)
  uint8_t  cancel_type  = 0;  //How the move was canceled (as applicable)
};

//Struct for storing basic punish infomations
struct Punish {
  uint8_t  num_moves    = 0;  //Number of moves (hitboxes connected) in the current punish
  unsigned start_frame  = 0;  //First frame of punish (0 == internal frame -123)
  unsigned end_frame    = 0;  //Last frame of punish (0 == internal frame -123)
  float    start_pct    = 0;  //Opponent's damage on first frame of punish
  float    end_pct      = 0;  //Opponent's damage on last frame of punish
  int      stocks       = 0;  //Opponent's stocks on first frame of punish
  uint8_t  last_move_id = 0;  //ID of the last move that hit
  uint8_t  opening      = 0;  //Opening type for first hit of punish (TODO: make an enum for this)
  uint8_t  kill_dir     = 0;  //Direction of kill, as specified in Dir enum (-1 = didn't kill)
};

//Struct for holding analysis data for a particular player within a game
struct AnalysisPlayer {
  unsigned     port                   =  0;  //0-indexed port number of the player (0-3 are valid)
  std::string  tag_player             =  ""; //Tag of the player as recorded in the game metadata
  std::string  tag_css                =  ""; //Tag of the player as chosen on the char. select screen
  std::string  tag_code               =  ""; //Code for the player on Slippi Online
  unsigned     char_id                =  0;  //External ID of the selected character
  std::string  char_name              =  ""; //Name of the selected character
  unsigned     player_type            =  0;  //Type of player (0=human, 1=CPU, 2=demo, 3=none)
  unsigned     cpu_level              =  0;  //Our level if we're a CPU
  unsigned     start_stocks           =  0;  //Number of stocks we started with
  unsigned     color                  =  0;  //Costume color
  unsigned     team_id                =  0;  //ID of the team we're on
  unsigned     end_stocks             =  0;  //Stock count on the last frame
  unsigned     end_pct                =  0;  //Damage on the last frame

  unsigned     airdodges              =  0;  //Number of airdodges performed
  unsigned     spotdodges             =  0;  //Number of spotdodges performed
  unsigned     rolls                  =  0;  //Number of rolls performed
  unsigned     dashdances             =  0;  //Number of dashdances performed
  unsigned     l_cancels_hit          =  0;  //Number of L-cancels performed
  unsigned     l_cancels_missed       =  0;  //Number of L-cancels failed
  unsigned     techs                  =  0;  //Number of ground techs performed
  unsigned     walltechs              =  0;  //Number of wall techs performed
  unsigned     walljumps              =  0;  //Number of wall jumps performed
  unsigned     walltechjumps          =  0;  //Number of wall tech jumps performed
  unsigned     missed_techs           =  0;  //Number of (ground) techs missed
  unsigned     ledge_grabs            =  0;  //Number of times we grabbed the ledge
  unsigned     air_frames             =  0;  //Number of frames spent airborne
  unsigned     wavedashes             =  0;  //Number of wavedashes performed
  unsigned     wavelands              =  0;  //Number of wavelands performed
  unsigned     neutral_wins           =  0;  //Number of times we won neutral
  unsigned     pokes                  =  0;  //Number of times we poked (single hits not leading to punishes)
  unsigned     counters               =  0;  //Number of counterattacks we performed
  unsigned     powershields           =  0;  //Number of times we successfully powershielded
  unsigned     shield_breaks          =  0;  //Number of times we broke our opponent's shield
  unsigned     grabs                  =  0;  //Number of grabs we landed
  unsigned     grab_escapes           =  0;  //Number of grabs we mashed out of
  unsigned     taunts                 =  0;  //Number of taunts we performed
  unsigned     meteor_cancels         =  0;  //Number of meteor cancels we performed
  float        damage_dealt           =  0;  //Total damage the player has dealth over the course of the game
  unsigned     hits_blocked           =  0;  //Number of hits blocked by shield
  unsigned     shield_stabs           =  0;  //Number of times we've shield stabbed our opponent
  unsigned     edge_cancel_aerials    =  0;  //Number of aerials edge cancelled
  unsigned     edge_cancel_specials   =  0;  //Number of special moves / air dodges / zairs edge cancelled
  unsigned     phantom_hits           =  0;  //Number of phantoms we hit our opponent with
  unsigned     shield_drops           =  0;  //Number of shield drops performed
  unsigned     no_impact_lands        =  0;  //Number of no impact lands (NILs) performed
  unsigned     pivots                 =  0;  //Number of pivots performed
  unsigned     reverse_edgeguards     =  0;  //Number of times we scored a KO while recovering
  unsigned     self_destructs         =  0;  //Number of times we died in neutral
  unsigned     stage_spikes           =  0;  //Number of times we KO'd our opponents with a stage spike
  unsigned     short_hops             =  0;  //Number of short hops we performed
  unsigned     full_hops              =  0;  //Number of full hops we performed
  unsigned     shield_time            =  0;  //Number of frames we spent in shield
  float        shield_damage          =  0;  //Cumulative shield damage taken over the course of the game
  float        shield_lowest          = 60;  //Lowest shield health we had at any point in the match
  unsigned     teeter_cancel_aerials  =  0;  //Number of aerials teeter cancelled
  unsigned     teeter_cancel_specials =  0;  //Number of special moves / air dodges / zairs teeter cancelled
  unsigned     total_openings         =  0;  //Total number of openings (pokes + neutral wins)
  float        mean_kill_openings     =  0;  //Average number of openings needed before getting a kill
  float        mean_kill_percent      =  0;  //Average damage done before taking a stock
  float        mean_opening_percent   =  0;  //Average damage done during each opening / punish
  unsigned     galint_ledgedashes     =  0;  //Number of intangible ledgedashes performed
  float        mean_galint            =  0;  //Average GALINT frames after a ledgedash
  unsigned     shieldstun_times       =  0;  //Number of times we entered shieldstun
  unsigned     shieldstun_act_frames  =  0;  //Total frames we spent acting out of shieldstun
  unsigned     hitstun_times          =  0;  //Number of times we entered hitstun
  unsigned     hitstun_act_frames     =  0;  //Total frames we spent acting out of hitstun
  unsigned     wait_times             =  0;  //Number of times we entered wait
  unsigned     wait_act_frames        =  0;  //Total frames we spent acting out of wait

  unsigned     max_galint             =  0;  //Maximum GALINT frames after a ledgedash
  unsigned     button_count           =  0;  //Button presses
  unsigned     cstick_count           =  0;  //C stick movements
  unsigned     astick_count           =  0;  //Action state changes
  float        apm                    =  0;  //Actions per minute (combines buttons and csticks)
  unsigned     state_changes          =  0;  //Analog stick movements
  float        aspm                   =  0;  //Action states per minute

  unsigned     used_throws            =  0;  //Number of throws we threw out
  unsigned     used_norm_moves        =  0;  //Number of normals we threw out
  unsigned     used_spec_moves        =  0;  //Number of specials we threw out
  unsigned     used_misc_moves        =  0;  //Number of misc. moves (getups, ledge attacks, etc.) we threw out
  unsigned     used_grabs             =  0;  //Number of grabs we threw out
  unsigned     used_pummels           =  0;  //Number of pummels we threw out
  unsigned     total_moves_used       =  0;  //Total number of moves we threw out
  unsigned     total_moves_landed     =  0;  //Total number of moves we landed
  float        move_accuracy          =  0;  //Computed as total_moves_used / total_moves_landed
  float        actionability          =  0;  //Mean actionability based on act out of stun and wait
  float        neutral_wins_per_min   =  0;  //Number of times we won neutral per minute spent in neutral
  float        mean_death_percent     =  0;  //Average damage received before losing a stock

  unsigned*    move_counts;                  //Counts for each move the player landed
  unsigned*    dyn_counts;                   //Frame counts for player interaction dynamics
  float*       dyn_damage;                   //Damage done during each player interaction dynamic
  Attack*      attacks;                      //List of all attacks we connected with throughout the game
  Punish*      punishes;                     //List of all punishes we performed throughout the game

  AnalysisPlayer() {
    move_counts = new unsigned[Move::__LAST]{0};
    dyn_counts  = new unsigned[Dynamic::__LAST]{0};
    dyn_damage  = new float[Dynamic::__LAST]{0};
    attacks     = new Attack[MAX_ATTACKS];
    punishes    = new Punish[MAX_PUNISHES];
  }
  ~AnalysisPlayer() {
    delete [] punishes;
    delete [] attacks;
    delete [] dyn_damage;
    delete [] dyn_counts;
    delete [] move_counts;
  }
};

//Struct for holding all analysis data within a game
struct Analysis {
  bool            success          = false;  //Whether we succeeded analyzing a replay
  unsigned        parse_errors     = 0;      //Number of errors that occurred during replay parsing
  std::string     game_time        = "";     //When the game was played, from replay metadata
  std::string     original_file    = "";     //Path to the original input file
  std::string     slippi_version   = "";     //Version of Slippi the replay was recorded with
  std::string     parser_version   = "";     //Version of parser the replay was parsed with
  std::string     analyzer_version = "";     //Version of this analyzer we are using
  unsigned        stage_id         = 0;      //Internal ID of the stage
  std::string     stage_name       = "";     //Readable name of the stage
  int             winner_port      = 0;      //Port index of the winning player (-1 == no winner)
  unsigned        game_length      = 0;      //Length of the game in frames (0 == internal frame -123)
  unsigned        timer            = 0;      //Game timer starting minutes
  AnalysisPlayer* ap;                        //Analysis of individual players in the game
  unsigned*       dynamics;                  //Interaction dynamics on a per-frame basis
  unsigned        end_type;                  //Game end type
  int             lras_player;               //Player port who LRAS-ed (-1 if none)

  Analysis(unsigned frame_count) {
    dynamics = new unsigned[frame_count]{0}; // List of dynamics active at each frame
    ap       = new AnalysisPlayer[2];        // The two players being analyzed
  }
  ~Analysis() {
    delete [] ap;
    delete [] dynamics;
  }

  std::string asJson();                      //Convert the analysis structure to a JSON
  void save(const char* outfilename);        //Write the analysis out to a JSON file
};

}

#endif /* ANALYSIS_H_ */
