#ifndef COMPRESSOR_H_
#define COMPRESSOR_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

#include "util.h"
#include "replay.h"
#include "analyzer.h"

// Replay File (.slp) Spec: https://github.com/project-slippi/project-slippi/wiki/Replay-File-Spec

const std::string COMPRESSOR_VERSION = "0.0.1";

namespace slip {

class Compressor {
private:
  int             _debug;                    //Current debug level
  SlippiReplay    _replay;                   //Internal struct for replay being parsed
  uint16_t        _payload_sizes[256] = {0}; //Size of payload for each event
  std::string     _slippi_version;           //String representation of the Slippi version of the replay
  uint8_t         _slippi_maj = 0;           //Major version number of replay being parsed
  uint8_t         _slippi_min = 0;           //Minor version number of replay being parsed
  uint8_t         _slippi_rev = 0;           //Revision number of replay being parsed
  uint8_t         _slippi_enc = 0;           //Encryption status of replay being parsed
  int32_t         _max_frames = 0;           //Maximum number of frames that there will be in the replay file

  uint32_t        _rng; //Current RNG seed we're working with
  char            _last_pre_frame[8][256] = {0};    //Last pre-frames for each player
  char            _last_post_frame[8][256] = {0};   //Last post-frames for each player
  char            _x_pre_frame[8][256] = {0};    //Delta for pre-frames
  char            _x_post_frame[8][256] = {0};   //Delta for post-frames
  int32_t         laststartframe = -123;         //Last frame used in frame start event

  char*           _rb = nullptr; //Read buffer
  char*           _wb = nullptr; //Write buffer
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
  bool            _parseFrameStart();
  bool            _parseBookend();
  bool            _parseGameEnd();
  bool            _parseMetadata();
  void            _cleanup(); //Cleanup replay data
public:
  Compressor(int debug_level);               //Instantiate the parser (possibly in debug mode)
  ~Compressor();                             //Destroy the parser
  bool load(const char* replayfilename); //Load a replay file
  Analysis* analyze();                   //Analyze the loaded replay file
  std::string asJson(bool delta);        //Convert the parsed replay structure to a JSON
  void save(const char* outfilename); //Save a comprssed replay file

  //https://www.reddit.com/r/SSBM/comments/71gn1d/the_basics_of_rng_in_melee/
  inline int32_t rollRNG(int32_t seed) {
    int64_t bigseed = seed;
    return ((bigseed * 214013) + 2531011) % 4294967296;
  }

  inline int32_t rollRNGNew(int32_t seed) {
    return (seed + 65536) % 4294967296;
  }
};

}

#endif /* COMPRESSOR_H_ */
