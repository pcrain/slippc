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

struct SlippiReplay {

};

}

#endif /* REPLAY_H_ */
