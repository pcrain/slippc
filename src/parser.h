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

#include <json/json.h>     //Json

#include "util.h"
#include "replay.h"

#define BUFFERMAXSIZE 104878080 //10 MB

namespace slip {

class Parser {
private:
  std::string _slippi_version;
  uint16_t _payload_sizes[256] = {0}; //Size of payload for each event
  Json::Value _jout;

  char*       _rb; //Read buffer
  unsigned    _bp; //Current position in buffer
  uint32_t    _length_raw; //(Remaining) length of raw payload
  bool        _parse(); //Internal main parsing funnction
  bool        _parseHeader();
  bool        _parseEventDescriptions();
  bool        _parseEvents();
  bool        _parseGameStart();
  bool        _parsePreFrame();
  bool        _parsePostFrame();
  bool        _parseGameEnd();
  bool        _parseMetadata();
public:
  Parser();                              //Instantiate the parser
  Parser(const char* replayfilename);    //Instantiate the parser and load a replay file
  ~Parser();                             //Destroy the parser
  void load(const char* replayfilename); //Load a replay file
  void summary();                        //Print a summary of the loaded replay file
  void save(const char* outfilename); //Load a replay file
};

}

#endif /* PARSER_H_ */
