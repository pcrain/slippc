#ifndef PARSER_H_
#define PARSER_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

#include "util.h"
#include "replay.h"
#include "analyzer.h"

#define BUFFERMAXSIZE 104878080 //10 MB

const std::string PARSER_VERSION = "0.0.2";

namespace slip {

class Parser {
private:
  bool            _debug;  //Whether we're operating in debug mode
  SlippiReplay    _replay;
  std::string     _slippi_version;
  uint16_t        _payload_sizes[256] = {0}; //Size of payload for each event

  std::ostream*   _dout;  //Debug output stream
  std::streambuf* _sbuf;  //Debug output stream buffer
  std::ofstream*  _dnull; //File pointer to /dev/null if needed

  char*           _rb; //Read buffer
  unsigned        _bp; //Current position in buffer
  uint32_t        _length_raw; //Remaining length of raw payload
  uint32_t        _length_raw_start; //Total length of raw payload
  bool            _parse(); //Internal main parsing funnction
  bool            _parseHeader();
  bool            _parseEventDescriptions();
  bool            _parseEvents();
  bool            _parseGameStart();
  bool            _parsePreFrame();
  bool            _parsePostFrame();
  bool            _parseGameEnd();
  bool            _parseMetadata();
public:
  Parser(bool debug); //Instantiate the parser
  ~Parser();                             //Destroy the parser
  bool load(const char* replayfilename); //Load a replay file
  Analysis* analyze();                   //Analyze the loaded replay file
  void save(const char* outfilename,bool delta); //Save a replay file
  void cleanup(); //Cleanup replay data

  //Estimate the maximum number of frames stored in the file
  //  -> Assumes only two people are alive for the whole match / one ice climber
  inline int32_t getMaxNumFrames() {
    //Get the base size for the file
    unsigned base_size = 0;
    base_size += 1+_payload_sizes[Event::EV_PAYLOADS];  //One meta event
    base_size += 1+_payload_sizes[Event::GAME_START]; //One start event
    base_size += 1+_payload_sizes[Event::GAME_END]; //One end event

    unsigned num_players = 2;

    unsigned frame_size = num_players*(
      _payload_sizes[Event::PRE_FRAME]+
      1+
      _payload_sizes[Event::POST_FRAME]+
      1
      );

    // std::cerr << "((" << _length_raw_start << "-" << base_size << ")/" << frame_size << ")-123" << std::endl;
    unsigned maxframes = ((_length_raw_start-base_size)/frame_size)-123;
    return maxframes+1;  //TODO: the above doesn't compute an exact number sometimes and we need the +1; figure out why
  }
};

}

#endif /* PARSER_H_ */
