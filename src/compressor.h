#ifndef COMPRESSOR_H_
#define COMPRESSOR_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <map>

#include "util.h"
#include "enums.h"

// Replay File (.slp) Spec: https://github.com/project-slippi/project-slippi/wiki/Replay-File-Spec

const std::string COMPRESSOR_VERSION = "0.0.1";
const uint32_t RAW_RNG_MASK          = 0x40000000;  //2nd bit of unsigned int

namespace slip {

class Compressor {
private:
  int             _debug;                    //Current debug level
  uint16_t        _payload_sizes[256] = {0}; //Size of payload for each event
  uint8_t         _slippi_maj         =  0;  //Major version number of replay being parsed
  uint8_t         _slippi_min         =  0;  //Minor version number of replay being parsed
  uint8_t         _slippi_rev         =  0;  //Revision number of replay being parsed
  uint8_t         _is_encoded         =  0;  //Encryption status of replay being parsed
  int32_t         _max_frames         =  0;  //Maximum number of frames that there will be in the replay file

  //Variables needed for mapping floats to ints and vice versa
  std::map<unsigned,unsigned> float_to_int;  //Map of floats to ints
  std::map<unsigned,unsigned> int_to_float;  //Reverse map of ints back to floats
  unsigned            num_floats = 1;        //Start at 1 since 0's used for exact predictions

  uint32_t        _rng; //Current RNG seed we're working with
  char            _x_pre_frame[8][256]    = {0};  //Delta for pre-frames
  char            _x_pre_frame_2[8][256]  = {0};  //Delta for 2 pre-frames ago
  char            _x_post_frame[8][256]   = {0};  //Delta for post-frames
  char            _x_post_frame_2[8][256] = {0};  //Delta for 2 post-frames ago
  char            _x_item[16][256]        = {0};  //Delta for item updates
  char            _x_item_2[16][256]      = {0};  //Delta for item updates 2 frames ago
  int32_t         laststartframe          = -123; //Last frame used in frame start event

  char*           _rb = nullptr; //Read buffer
  char*           _wb = nullptr; //Write buffer
  unsigned        _bp;           //Current position in buffer
  uint32_t        _length_raw;   //Remaining length of raw payload
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
public:
  Compressor(int debug_level);               //Instantiate the parser (possibly in debug mode)
  ~Compressor();                             //Destroy the parser
  bool load(const char* replayfilename); //Load a replay file
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
  //  to an index build on-the-fly
  inline void buildFloatMap(unsigned off) {
    unsigned enc_damage = readBE4U(&_rb[_bp+off]);
    unsigned magic = 0xF0000000;  //Flip first 4 bits
    if (_is_encoded) {                  //Decode
      if ((enc_damage & magic) != magic) {
        //If it's our first encounter with a float, add it to our maps
        float_to_int[enc_damage] = num_floats;
        int_to_float[num_floats] = enc_damage;
        ++num_floats;
      } else {
        //Decode our int back to a float
        unsigned as_unsigned = int_to_float[enc_damage & (magic ^ 0xFFFFFFFF)];
        writeBE4U(as_unsigned, &_wb[_bp+off]);
      }
    } else {                            //Encode
      if (float_to_int.find(enc_damage) == float_to_int.end()) {
        //If it's our first encounter with a float, add it to our maps
        float_to_int[enc_damage] = num_floats;
        int_to_float[num_floats] = enc_damage;
        ++num_floats;
      } else {
        //Set the 2nd and 3rd bit, since they will never be set simultaneously in real floats
        writeBE4U(float_to_int[enc_damage] | magic, &_wb[_bp+off]);
      }
    }
  }

  //Using the previous two pre-frame events as reference, predict a floating point value, and store
  //  an otherwise-impossible float if our prediction was correct
  inline void predictAccelPre(unsigned p, unsigned off) {
    union { float f; uint32_t u; } float_true, float_pred, float_temp;
    unsigned magic = 0x12345678;  //Flip all bits

    float_true.f = readBE4F(&_rb[_bp+off]);
    float_pred.f = 2*readBE4F(&_x_pre_frame[p][off]) - readBE4F(&_x_pre_frame_2[p][off]);
    float_temp.u = float_pred.u ^ float_true.u;
    if (_is_encoded) {
      if (float_true.u == magic) {  //If our prediction was exactly accurate
        writeBE4F(float_pred.f,&_wb[_bp+off]);  //Write our predicted float
      }
    } else {
      if (float_temp.u == 0) {  //If our prediction was exactly accurate
        writeBE4U(magic,&_wb[_bp+off]);  //Write an impossible float
      }
    }
    memcpy(&_x_pre_frame_2[p][off], &_x_pre_frame[p][off],4);
    memcpy(&_x_pre_frame[p][off],_is_encoded ? &_wb[_bp+off] : &_rb[_bp+off],4);
  }

  //Using the previous two post-frame events as reference, predict a floating point value, and store
  //  an otherwise-impossible float if our prediction was correct
  inline void predictAccelPost(unsigned p, unsigned off) {
    union { float f; uint32_t u; } float_true, float_pred, float_temp;
    unsigned magic = 0x01000000;  //Flip 8th bit

    float_true.f = readBE4F(&_rb[_bp+off]);
    float_pred.f = 2*readBE4F(&_x_post_frame[p][off]) - readBE4F(&_x_post_frame_2[p][off]);
    float_temp.u = float_pred.u ^ float_true.u;
    if (_is_encoded) {
      if (float_true.u == magic) {  //If our prediction was exactly accurate
        writeBE4F(float_pred.f,&_wb[_bp+off]);  //Write our predicted float
      }
    } else {
      if (float_temp.u == 0) {  //If our prediction was exactly accurate
        writeBE4U(magic,&_wb[_bp+off]);  //Write an impossible float
      }
    }
    memcpy(&_x_post_frame_2[p][off], &_x_post_frame[p][off],4);
    memcpy(&_x_post_frame[p][off],_is_encoded ? &_wb[_bp+off] : &_rb[_bp+off],4);
  }

  //Using the previous two item events with slot id as reference, predict a floating point value, and store
  //  an otherwise-impossible float if our prediction was correct
  inline void predictAccelItem(unsigned p, unsigned off) {
    union { float f; uint32_t u; } float_true, float_pred, float_temp;
    unsigned magic = 0x01000000;  //Flip 8th bit

    float_true.f = readBE4F(&_rb[_bp+off]);
    float_pred.f = 2*readBE4F(&_x_item[p][off]) - readBE4F(&_x_item_2[p][off]);
    float_temp.u = float_pred.u ^ float_true.u;
    if (_is_encoded) {
      if (float_true.u == magic) {  //If our prediction was exactly accurate
        writeBE4F(float_pred.f,&_wb[_bp+off]);  //Write our predicted float
      }
    } else {
      if (float_temp.u == 0) {  //If our prediction was exactly accurate
        writeBE4U(magic,&_wb[_bp+off]);  //Write an impossible float
      }
    }
    memcpy(&_x_item_2[p][off], &_x_item[p][off],4);
    memcpy(&_x_item[p][off],_is_encoded ? &_wb[_bp+off] : &_rb[_bp+off],4);
  }

  //Remove the RAW_RNG_MASK bit from a frame, and return whether that bit was flipped
  inline bool unmaskFrame(int32_t &frame) {
    //If first and second bits of frame are different, rng is stored raw
    uint8_t bit1 = (frame >> 31) & 0x01;
    uint8_t bit2 = (frame >> 30) & 0x01;
    uint8_t rng_is_raw = bit1 ^ bit2;
    if (rng_is_raw > 0) {
      //Flip second bit back to what it should be
      // std::cout << "is raw" << std::endl;
      frame ^= RAW_RNG_MASK;
    }

    return rng_is_raw > 0;
  }

  //Predict the next frame given the last frame and delta encode the difference
  inline void predictFrame(int32_t frame, unsigned off, bool setFrame = true) {
    //Remove RNG bit from frame
    bool rng_is_raw  = unmaskFrame(frame);
    //Flip 2nd bit in _wb output if raw RNG flag is set
    uint32_t bitmask = rng_is_raw ? RAW_RNG_MASK : 0x00000000;

    if (_is_encoded) {                  //Decode
      int32_t frame_delta_pred = frame + (laststartframe + 1);
      writeBE4S(frame_delta_pred ^ bitmask,&_wb[_bp+off]);
      if (setFrame) {
        laststartframe = frame_delta_pred;
      }
    } else {                            //Encode
      int32_t frame_delta_pred = frame - (laststartframe + 1);
      writeBE4S(frame_delta_pred,&_wb[_bp+off]);
      if (setFrame) {
        laststartframe = frame;
      }
    }
  }

  //Predict the RNG by reading a full (not delta-encoded) frame and
  //  counting the number of rolls it takes to arrive at the correct seed
  inline void predictRNG(unsigned frameoff, unsigned rngoff) {
    const uint32_t max_rolls = 256;

    //Read true frame from write buffer
    int32_t frame;
    if (_is_encoded) {  //Read from write buffer
      frame = readBE4S(&_wb[_bp+frameoff]);
    } else {            //Read from read buffer
      frame = readBE4S(&_rb[_bp+frameoff]);
    }
    bool rng_is_raw = unmaskFrame(frame);

    //Get RNG seed and record number of rolls it takes to get to that point
    if ((100 *_slippi_maj + _slippi_min) >= 307) {  //New RNG
      if (_is_encoded) {  //Decode
        if (rng_is_raw == 0) { //Roll RNG a few times until we get to the desired value
          unsigned rolls = readBE4U(&_rb[_bp+rngoff]);
          for(unsigned i = 0; i < rolls; ++i) {
            _rng = rollRNGNew(_rng);
          }
          writeBE4U(_rng,&_wb[_bp+rngoff]);
        } else {
          writeBE4U(frame,&_wb[_bp+frameoff]);
        }
      } else {  //Encode
        unsigned rolls     = 0;
        unsigned target    = readBE4U(&_rb[_bp+rngoff]);
        for(uint32_t start_rng = _rng; (start_rng != target) && (rolls < max_rolls); ++rolls) {
          start_rng = rollRNGNew(start_rng);
        }
        // std::cout << "rolls: " << rolls << std::endl;
        if (rolls < max_rolls) {  //If we can encode RNG as < 256 rolls, do it
          writeBE4U(rolls,&_wb[_bp+rngoff]);
          _rng = target;
        } else {  //Store the raw RNG value
          //Load the predicted frame value
          int32_t predicted_frame = readBE4S(&_wb[_bp+frameoff]);
          //Flip second bit of cached frame number to signal raw rng
          writeBE4S(predicted_frame ^ RAW_RNG_MASK,&_wb[_bp+frameoff]);
        }
      }
    } else { //Old RNG
      if (_is_encoded) {
        unsigned rolls = readBE4U(&_rb[_bp+rngoff]);
        for(unsigned i = 0; i < rolls; ++i) {
          _rng = rollRNG(_rng);
        }
        writeBE4U(_rng,&_wb[_bp+rngoff]);
      } else { //Roll RNG until we hit the target, write the # of rolls
        unsigned rolls, seed = readBE4U(&_rb[_bp+rngoff]);
        for(rolls = 0;_rng != seed; ++rolls) {
          _rng = rollRNG(_rng);
        }
        writeBE4U(rolls,&_wb[_bp+rngoff]);
      }
    }
  }

  //XOR bits in the current read buffer with bits from another buff
  inline void xorEncodeRange(unsigned start, unsigned stop, char* buffer) {
    for(unsigned i = start; i < stop; ++i) {
      _wb[_bp+i] = _rb[_bp+i] ^ buffer[i];
      buffer[i]  = _is_encoded ? _wb[_bp+i] : _rb[_bp+i];
    }
  }

};

}

#endif /* COMPRESSOR_H_ */
