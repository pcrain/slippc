#ifndef COMPRESSOR_H_
#define COMPRESSOR_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <map>

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
  uint8_t         _is_encoded = 0;           //Encryption status of replay being parsed
  int32_t         _max_frames = 0;           //Maximum number of frames that there will be in the replay file

  // std::map<float,int> float_to_int;
  // std::map<int,float> int_to_float;
  std::map<unsigned,unsigned> float_to_int;
  std::map<unsigned,unsigned> int_to_float;
  unsigned            num_floats = 0;

  uint32_t        _rng; //Current RNG seed we're working with
  char            _last_pre_frame[8][256] = {0};    //Last pre-frames for each player
  char            _last_post_frame[8][256] = {0};   //Last post-frames for each player
  char            _x_pre_frame[8][256] = {0};    //Delta for pre-frames
  char            _x_post_frame[8][256] = {0};   //Delta for post-frames
  char            _x_item[16][256] = {0};         //Delta for item updates
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
  bool            _parseItemUpdate();
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

  //Create a mapping of floats we've already seen by treating them as unsigned integer indices
  inline void buildFloatMap(unsigned off) {
    unsigned enc_damage = readBE4U(&_rb[_bp+off]);
    if (_is_encoded) {                  //Decode
      if ((enc_damage & 0x60000000) != 0x60000000) {
        //If it's our first encounter with a float, add it to our maps
        float_to_int[enc_damage] = num_floats;
        int_to_float[num_floats] = enc_damage;
        // std::cout << "Adding " << enc_damage << " as " << num_floats << std::endl;
        ++num_floats;
      } else {
        //Decode our int back to a float
        // std::cout << "Converting from " << int_to_float[enc_damage & 0x9FFFFFFF] << " to " << (enc_damage & 0x9FFFFFFF) << std::endl;
        unsigned as_unsigned = int_to_float[enc_damage & 0x9FFFFFFF];
        writeBE4U(as_unsigned, &_wb[_bp+off]);
      }
    } else {                            //Encode
      if (float_to_int.find(enc_damage) == float_to_int.end()) {
        //If it's our first encounter with a float, add it to our maps
        float_to_int[enc_damage] = num_floats;
        int_to_float[num_floats] = enc_damage;
        // std::cout << "Adding " << enc_damage << " as " << num_floats << std::endl;
        ++num_floats;
      } else {
        //Set the 2nd and 3rd bit, since they will never be set simultaneously in real floats
        // std::cout << "Converting from " << enc_damage << " to " << float_to_int[enc_damage] << std::endl;
        writeBE4U(float_to_int[enc_damage] | 0x60000000, &_wb[_bp+off]);
      }
    }
  }
};

}

#endif /* COMPRESSOR_H_ */
