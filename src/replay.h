#ifndef REPLAY_H_
#define REPLAY_H_

#include <iostream>
#include <fstream>

#include "enums.h"
#include "util.h"

// Replay File (.slp) Spec: https://github.com/project-slippi/project-slippi/wiki/Replay-File-Spec

#define BYTE8(b1,b2,b3,b4,b5,b6,b7,b8) (*((uint64_t*)(char[]){b1,b2,b3,b4,b5,b6,b7,b8}))
#define BYTE4(b1,b2,b3,b4)             (*((uint32_t*)(char[]){b1,b2,b3,b4}))
#define BYTE2(b1,b2)                   (*((uint16_t*)(char[]){b1,b2}))

const uint64_t SLP_HEADER = BYTE8(0x7b,0x55,0x03,0x72,0x61,0x77,0x5b,0x24); // {U.raw[$

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
  uint32_t hitstun       = 0;
  bool     airborne      = false;
  uint16_t ground_id     = 0;
  uint8_t  jumps         = 0;
  uint8_t  l_cancel      = 0;
};

struct SlippiPlayer {
  uint8_t      ext_char_id  = 0;
  uint8_t      player_type  = 3;  //empty
  uint8_t      start_stocks = 0;
  uint8_t      color        = 0;
  uint8_t      team_id      = 0;
  uint32_t     dash_back    = 0;
  uint32_t     shield_drop  = 0;
  std::string  tag          = "";
  std::string  tag_css      = "";
  SlippiFrame* frame        =  nullptr;
};

struct SlippiReplay {
  std::string     slippi_version = "";
  std::string     parser_version = "";
  std::string     game_start_raw = "";
  std::string     metadata       = "";
  std::string     played_on      = "";
  std::string     start_time     = "";
  bool            teams          = false;
  uint16_t        stage          = 0;
  uint32_t        seed           = 0;
  bool            pal            = false;
  bool            frozen         = false;
  uint8_t         end_type       = 0;
  int8_t          lras           = 0;
  int32_t         first_frame    = LOAD_FRAME;
  int32_t         last_frame     = 0;
  uint32_t        frame_count    = 0;
  SlippiPlayer    player[8];

  void setFrames(int32_t max_frames);
  void cleanup();
  std::string replayAsJson(bool delta);
};


}

#endif /* REPLAY_H_ */
