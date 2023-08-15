#ifndef PARSER_H_
#define PARSER_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

#include "util.h"
#include "replay.h"
#include "analyzer.h"
#include "schema.h"
#include "compressor.h"

// Replay File (.slp) Spec: https://github.com/project-slippi/slippi-wiki/blob/master/SPEC.md

namespace slip {

class Parser {
private:
  int             _debug;                    //Current debug level
  SlippiReplay    _replay;                   //Internal struct for replay being parsed
  uint16_t        _payload_sizes[256] = {0}; //Size of payload for each event
  std::string     _slippi_version;           //String representation of the Slippi version of the replay
  uint8_t         _slippi_maj     = 0;       //Major version number of replay being parsed
  uint8_t         _slippi_min     = 0;       //Minor version number of replay being parsed
  uint8_t         _slippi_rev     = 0;       //Revision number of replay being parsed
  int32_t         _max_frames     = 0;       //Maximum number of frames that there will be in the replay file
  bool            _game_end_found = false;   //Whether we've found the game end event
  bool            _is_encoded     = false;   //Whether this file is encoded by the compressor

  char*           _rb = nullptr; //Read buffer
  unsigned        _bp; //Current position in buffer
  uint32_t        _length_raw; //Remaining length of raw payload
  uint32_t        _length_raw_start; //Total length of raw payload
  uint32_t        _file_size; //Total size of the replay file on disk
  bool            _parse(); //Internal main parsing funnction
  bool            _parseHeader();
  bool            _parseEventDescriptions();
  bool            _parseEvents();
  bool            _parseGameStart();
  bool            _parsePreFrame();
  bool            _parsePostFrame();
  bool            _parseGameEnd();
  bool            _parseItemUpdate();
  bool            _parseMetadata();
  void            _cleanup(); //Cleanup replay data
public:
  Parser(int debug_level);               //Instantiate the parser (possibly in debug mode)
  ~Parser();                             //Destroy the parser
  bool load(const char* replayfilename); //Load a replay file
  Analysis* analyze();                   //Analyze the loaded replay file
  std::string asJson(bool delta);        //Convert the parsed replay structure to a JSON
  void save(const char* outfilename,bool delta); //Save a replay file

  //Getter function for exposing read-only access to underlying replay
  inline const SlippiReplay* replay() const {
    return &_replay;
  };

  //Estimate the maximum number of frames stored in the file
  //  -> Assumes only two people are alive for the whole match / one ice climber
  inline int32_t getMaxNumFrames() {
    //Get the base size for the file
    unsigned base_size = 0;
    base_size += _payload_sizes[Event::EV_PAYLOADS];  //One meta event
    base_size += _payload_sizes[Event::GAME_START];   //One start event
    base_size += _payload_sizes[Event::GAME_END];     //One end event

    unsigned num_players = 2;

    unsigned frame_size = num_players*(
      _payload_sizes[Event::PRE_FRAME]+
      _payload_sizes[Event::POST_FRAME]
      );

    if (_length_raw_start < base_size) {
      return 0;  //There's a problem
    }

    // std::cerr << "((" << _length_raw_start << "-" << base_size << ")/" << frame_size << ")-123" << std::endl;
    int32_t maxframes = ((_length_raw_start-base_size)/frame_size)+LOAD_FRAME;
    return maxframes+1;  //TODO: the above doesn't compute an exact number sometimes and we need the +1; figure out why
  }
};

}

#endif /* PARSER_H_ */
