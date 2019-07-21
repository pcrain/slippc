#ifndef PARSER_H_
#define PARSER_H_

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <curses.h>
#include <thread>

#include <cstdarg>
#include <vector>
#include <string>
#include <regex>

// #include <json/json.h>     //Json

#include "util.h"
#include "replay.h"

#define BUFFERMAXSIZE 104878080 //10 MB

namespace slip {

class Parser {
private:
  SlippiReplay _replay;
  std::string  _slippi_version;
  uint16_t     _payload_sizes[256] = {0}; //Size of payload for each event
  // Json::Value  _jout;

  char*        _rb; //Read buffer
  unsigned     _bp; //Current position in buffer
  uint32_t     _length_raw; //Remaining length of raw payload
  uint32_t     _length_raw_start; //Total length of raw payload
  bool         _parse(); //Internal main parsing funnction
  bool         _parseHeader();
  bool         _parseEventDescriptions();
  bool         _parseEvents();
  bool         _parseGameStart();
  bool         _parsePreFrame();
  bool         _parsePostFrame();
  bool         _parseGameEnd();
  bool         _parseMetadata();
public:
  Parser();                              //Instantiate the parser
  Parser(const char* replayfilename);    //Instantiate the parser and load a replay file
  ~Parser();                             //Destroy the parser
  bool load(const char* replayfilename); //Load a replay file
  void summary();                        //Print a summary of the loaded replay file
  void save(const char* outfilename,bool delta); //Save a replay file

  //Estimate the maximum number of frames stored in the file
  //  -> Assumes only two people are alive for the whole match / one ice climber
  inline int32_t getMaxNumFrames() {
    //Get the base size for the file
    unsigned base_size = 0;
    base_size += 1+_payload_sizes[Event::EV_PAYLOADS];  //One meta event
    base_size += 1+_payload_sizes[Event::GAME_START]; //One start event
    base_size += 1+_payload_sizes[Event::GAME_END]; //One end event

    // unsigned num_players = 0;
    // for(unsigned p = 0 ; p < 4; ++p) {
    //   if (_replay.player[p].player_type != 3) {
    //     ++num_players;  //Ignoring icies
    //     // if (_replay.player[p].ext_char_id == 0x0E) {
    //     //   ++num_players; //Ice climbers count twice
    //     // }
    //   }
    // }
    unsigned num_players = 2;

    unsigned frame_size = num_players*(
      _payload_sizes[Event::PRE_FRAME]+
      1+
      _payload_sizes[Event::POST_FRAME]+
      1
      );

    unsigned nframes = ((_length_raw_start-base_size)/frame_size)-123;
    return nframes;
  }
};

}

#endif /* PARSER_H_ */
