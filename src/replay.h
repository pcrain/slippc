#ifndef REPLAY_H_
#define REPLAY_H_

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <curses.h>
#include <thread>

#include <cstdarg>
#include <vector>
#include <string>

#include "util.h"

// Info
//   https://docs.google.com/spreadsheets/d/1JX2w-r2fuvWuNgGb6D3Cs4wHQKLFegZe2jhbBuIhCG8/edit#gid=20
//   https://docs.google.com/spreadsheets/d/1JX2w-r2fuvWuNgGb6D3Cs4wHQKLFegZe2jhbBuIhCG8/edit#gid=13

#define BYTE8(b1,b2,b3,b4,b5,b6,b7,b8) (*((uint64_t*)(char[]){b1,b2,b3,b4,b5,b6,b7,b8}))
#define BYTE4(b1,b2,b3,b4)             (*((uint32_t*)(char[]){b1,b2,b3,b4}))
#define BYTE2(b1,b2)                   (*((uint16_t*)(char[]){b1,b2}))

namespace slip {

enum Event {
  EV_PAYLOADS = 0x35,
  GAME_START  = 0x36,
  PRE_FRAME   = 0x37,
  POST_FRAME  = 0x38,
  GAME_END    = 0x39,
  __LAST      = 0xFF
};

const uint64_t SLP_HEADER = BYTE8(0x7b,0x55,0x03,0x72,0x61,0x77,0x5b,0x24); // {U.raw[$

struct SlippiFrame {
  //Parser stuff
  bool     alive;  //For checking if this frame was actually set

  //Pre-frame stuff
  int32_t  frame;
  uint8_t  player;
  bool     follower;
  uint32_t seed;
  uint16_t action_pre;
  float    pos_x_pre;
  float    pos_y_pre;
  float    face_dir_pre;
  float    joy_x;
  float    joy_y;
  float    c_x;
  float    c_y;
  float    trigger;
  uint32_t buttons;
  float    phys_l;
  float    phys_r;
  uint8_t  ucf_x;
  float    percent_pre;

  //Post-frame stuff
  uint8_t  char_id;
  uint16_t action_post;
  float    pos_x_post;
  float    pos_y_post;
  float    face_dir_post;
  float    percent_post;
  float    shield;
  uint8_t  hit_with;
  uint8_t  combo;
  uint8_t  hurt_by;
  uint8_t  stocks;
  float    action_fc;
  uint8_t  flags_1;
  uint8_t  flags_2;
  uint8_t  flags_3;
  uint8_t  flags_4;
  uint8_t  flags_5;
  uint32_t hitstun;
  bool     airborne;
  uint16_t ground_id;
  uint8_t  jumps;
  uint8_t  l_cancel;
};

struct SlippiPlayer {
  uint8_t      ext_char_id;
  uint8_t      player_type;
  uint8_t      start_stocks;
  uint8_t      color;
  uint8_t      team_id;
  uint32_t     dash_back;
  uint32_t     shield_drop;
  std::string  tag;
  SlippiFrame* frame;
};

struct SlippiReplay {
  std::string     slippi_version;
  std::string     game_start_raw;
  std::string     metadata;
  bool            teams;
  uint16_t        stage;
  uint32_t        seed;
  bool            pal;
  bool            frozen;
  uint8_t         end_type;
  int8_t          lras;
  int32_t         frame_count;
  SlippiPlayer    player[8];
};

void setFrames(SlippiReplay &s, int32_t nframes);
void cleanup(SlippiReplay &s);
std::string replayAsJson(SlippiReplay &s,bool delta);
void summarize(SlippiReplay &s);

}

#endif /* REPLAY_H_ */
