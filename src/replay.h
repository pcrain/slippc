#ifndef REPLAY_H_
#define REPLAY_H_

#include <iostream>
#include <fstream>

#include "enums.h"
#include "util.h"

// Replay File (.slp) Spec: https://github.com/project-slippi/project-slippi/wiki/Replay-File-Spec

const unsigned MAX_ITEMS = 1024; //Max number of items to output

namespace slip {

struct SlippiFrame {
  //Parser stuff
  bool     alive         = false;  //For checking if this frame was actually set

  //Pre-frame stuff
  int32_t  frame         = 0;
  uint8_t  player        = 0;
  bool     follower      = false;
  uint32_t seed          = 0;
  uint16_t action_pre    = 0;
  float    pos_x_pre     = 0.0f;
  float    pos_y_pre     = 0.0f;
  float    face_dir_pre  = 0.0f;
  float    joy_x         = 0.0f;
  float    joy_y         = 0.0f;
  float    c_x           = 0.0f;
  float    c_y           = 0.0f;
  float    trigger       = 0.0f;
  uint32_t buttons       = 0;
  float    phys_l        = 0.0f;
  float    phys_r        = 0.0f;
  uint8_t  ucf_x         = 0;
  float    percent_pre   = 0.0f;

  //Post-frame stuff
  uint8_t  char_id       = 0;
  uint16_t action_post   = 0;
  float    pos_x_post    = 0.0f;
  float    pos_y_post    = 0.0f;
  float    face_dir_post = 0.0f;
  float    percent_post  = 0.0f;
  float    shield        = 0.0f;
  uint8_t  hit_with      = 0;
  uint8_t  combo         = 0;
  uint8_t  hurt_by       = 0;
  uint8_t  stocks        = 0;
  float    action_fc     = 0.0f;
  uint8_t  flags_1       = 0;
  uint8_t  flags_2       = 0;
  uint8_t  flags_3       = 0;
  uint8_t  flags_4       = 0;
  uint8_t  flags_5       = 0;
  float    hitstun       = 0.0f;
  bool     airborne      = false;
  uint16_t ground_id     = 0;
  uint8_t  jumps         = 0;
  uint8_t  l_cancel      = 0;
  uint8_t  hurtbox       = 0;
  float    self_air_x    = 0;
  float    self_air_y    = 0;
  float    attack_x      = 0;
  float    attack_y      = 0;
  float    self_grd_x    = 0;
  float    hitlag        = 0;
};

struct SlippiItemFrame {
  int32_t  frame         = 0;
  uint8_t  state         = 0;
  float    face_dir      = 0;
  float    xvel          = 0;
  float    yvel          = 0;
  float    xpos          = 0;
  float    ypos          = 0;
  uint16_t damage        = 0;
  float    expire        = 0;
  uint8_t  flags_1       = 0;
  uint8_t  flags_2       = 0;
  uint8_t  flags_3       = 0;
  uint8_t  flags_4       = 0;
  uint8_t  owner         = 0;
};

struct SlippiItem {
  uint16_t         type       = 0;
  uint32_t         spawn_id   = MAX_ITEMS+1;
  uint32_t         num_frames = 0;
  SlippiItemFrame* frame      = nullptr;
};

struct SlippiPlayer {
  uint8_t      ext_char_id  = 0;
  uint8_t      player_type  = 3;  //empty
  uint8_t      start_stocks = 0;
  uint8_t      end_stocks   = 0;
  uint8_t      color        = 0;
  uint8_t      team_id      = 0;
  uint8_t      cpu_level    = 0;
  uint32_t     dash_back    = 0;
  uint32_t     shield_drop  = 0;
  std::string  tag          = "";
  std::string  tag_code     = "";
  std::string  tag_css      = "";
  SlippiFrame* frame        = nullptr;
};

struct SlippiReplay {
  uint32_t        slippi_version_raw  = 0;
  std::string     slippi_version      = "";
  std::string     parser_version      = "";
  std::string     game_start_raw      = "";
  std::string     metadata            = "";
  std::string     played_on           = "";
  std::string     start_time          = "";
  int8_t          winner_id           = -1;
  bool            teams               = false;
  uint16_t        stage               = 0;
  uint32_t        seed                = 0;
  bool            pal                 = false;
  bool            frozen              = false;
  uint8_t         end_type            = 0;
  int8_t          lras                = -1;
  int32_t         first_frame         = LOAD_FRAME;
  int32_t         last_frame          = 0;
  uint32_t        frame_count         = 0;
  uint8_t         timer               = 0;
  int8_t          items_on            = 0;
  bool            sudden_death        = false;
  SlippiPlayer    player[8]           = {0};
  SlippiItem      item[MAX_ITEMS]     = {0};

  void setFrames(int32_t max_frames);
  void cleanup();
  std::string replayAsJson(bool delta);
};


}

#endif /* REPLAY_H_ */
