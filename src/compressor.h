#ifndef COMPRESSOR_H_
#define COMPRESSOR_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <map>
#include <cmath>
#include <limits>
#include <cstdio>

#include "util.h"
#include "enums.h"

// Replay File (.slp) Spec: https://github.com/project-slippi/project-slippi/wiki/Replay-File-Spec

const std::string COMPRESSOR_VERSION = "0.0.1";
const uint32_t RAW_RNG_MASK          = 0x40000000;  //Second bit of unsigned int
const uint32_t MAGIC_FLOAT           = 0xFF000000;  //First 8 bits of float

const uint32_t FLOAT_RING_SIZE       = 256;

// Frame event column byte widths
const unsigned cw_start[40] = {1,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
const unsigned cw_pre[40]   = {1,4,1,1,4,2,4,4,4,4,4,4,4,4,4,2,4,4,1,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
const unsigned cw_item[40]  = {1,4,2,1,4,4,4,4,4,2,4,4,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
const unsigned cw_post[40]  = {1,4,1,1,1,2,4,4,4,4,4,1,1,1,1,4,1,1,1,1,1,4,1,2,1,1,1,4,4,4,4,4,0,0,0,0,0,0,0,0};
const unsigned cw_end[40]   = {1,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

// static std::map<uint16_t,uint16_t> ACTION_MAP; //Map of action states
// static std::map<uint16_t,uint16_t> ACTION_REV; //Reverse map of action states
// static uint32_t _mapped_c = 0; //Number of explicitly mapped values

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

  // uint32_t        _float_ring[256]{0};
  // uint32_t        _float_ring_pos;
  // std::map<unsigned,unsigned>  _as_counter;

  //Variables needed for mapping floats to ints and vice versa
  std::map<unsigned,unsigned> float_to_int;  //Map of floats to ints
  std::map<unsigned,unsigned> int_to_float;  //Reverse map of ints back to floats
  std::map<unsigned,unsigned> int_to_float_c;//Counter for reverse map of ints back to floats
  unsigned            num_floats = 0;        //Total number of floats in float map

  uint32_t        _preds = 0;
  uint32_t        _fails = 0;

  uint32_t        _rng; //Current RNG seed we're working with
  uint32_t        _rng_start; //Starting RNG seed
  char            _x_pre_frame[8][256]    = {0};  //Delta for pre-frames
  char            _x_pre_frame_2[8][256]  = {0};  //Delta for 2 pre-frames ago
  char            _x_post_frame[8][256]   = {0};  //Delta for post-frames
  char            _x_post_frame_2[8][256] = {0};  //Delta for 2 post-frames ago
  char            _x_post_frame_3[8][256] = {0};  //Delta for 3 post-frames ago
  char            _x_item[16][256]        = {0};  //Delta for item updates
  char            _x_item_2[16][256]      = {0};  //Delta for item updates 2 frames ago
  char            _x_item_3[16][256]      = {0};  //Delta for item updates 3 frames ago
  int32_t         laststartframe          = -123; //Last frame used in frame start event
  int32_t         lastshuffleframe        = -123; //Separate frame tracker for _unshuffleEvents()

  char*           _rb = nullptr; //Read buffer
  char*           _wb = nullptr; //Write buffer
  unsigned        _bp;           //Current position in buffer
  uint32_t        _length_raw;   //Remaining length of raw payload
  uint32_t        _length_raw_start; //Total length of raw payload
  uint32_t        _game_loop_start = 0; //First byte AFTER game start event
  uint32_t        _game_loop_end = 0; //First byte OF game end event
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
  bool            _shuffleEvents(bool unshuffle = false);
  bool            _unshuffleEvents();
public:
  Compressor(int debug_level);               //Instantiate the parser (possibly in debug mode)
  ~Compressor();                             //Destroy the parser
  bool load(const char* replayfilename);     //Load a replay file
  void save(const char* outfilename);        //Save a comprssed replay file

  //https://www.reddit.com/r/SSBM/comments/71gn1d/the_basics_of_rng_in_melee/
  inline int32_t rollRNGLegacy(int32_t seed) const {
    int64_t bigseed = seed;  //Cast from 32-bit to 64-bit int
    return ((bigseed * 214013) + 2531011) % 4294967296;
  }

  //Given the current frame's RNG value, just add 65536 to get the next frame's
  inline int32_t rollRNGRollback(int32_t seed) const {
    return (seed + 65536) % 4294967296;
  }

  //Per Fizzi and Nikki, RNG is just the starting RNG + 65536*<0-indexed frame>
  inline int32_t computeRNGRollback(int32_t framenum) const {
    return (((framenum+123) * 65536) + _rng_start) % 4294967296;
  }

  //Tried multiplying analog float values by 560 to get ints
  //  but did not improve compression
  inline void analogFloatToInt(unsigned off, float mult) const {
    union { float f; uint32_t u; int32_t i;} raw_val;

    const int magic = 0x40000000;
    raw_val.u       = readBE4U(&_rb[_bp+off]);

    if (_is_encoded) {                  //Decode
      if (raw_val.u & magic) {  //Check if it's actually encoded
        raw_val.u       ^= magic;
        float rst_float  = int32_t(raw_val.i-1.0f)/mult;
        // std::cout << "Restoring " << rst_float << std::endl;
        writeBE4F(rst_float, &_wb[_bp+off]);
      } else {
        // std::cout << "Restoring problem " << readBE4F(&_rb[_bp+off]) << std::endl;
      }
    } else {                            //Encode
      float raw_float = readBE4F(&_rb[_bp+off]);
      raw_val.u       = uint32_t(round((1.0f+raw_float)*mult));
      writeBE4U(raw_val.u, &_wb[_bp+off]);
      raw_val.u       = readBE4U(&_wb[_bp+off]);
      float rst_float = int32_t(raw_val.i-1.0f)/mult;
      if (raw_float != rst_float) {
        // std::cout << "PROBLEM: " << raw_float << " != " << rst_float << std::endl;
        writeBE4F(raw_float, &_wb[_bp+off]);  //Replace the old value
      } else {
        writeBE4U(raw_val.u | magic, &_wb[_bp+off]);
      }
    }
  }

  //Print the byte counts of the read and write buffers
  inline void printBuffers() const {
    std::map<char,int> wcounter, rcounter;
     for(unsigned i = 0; i < 256; ++i) {
      wcounter[i] = 0;
      rcounter[i] = 0;
    }
    for(unsigned i = 0; i < _file_size; ++i) {
      ++wcounter[_wb[i]];
      ++rcounter[_rb[i]];
    }
    for(unsigned i = 0; i < 256; ++i) {
      printf("Byte 0x%02x : %8u - %8u\n",i,rcounter[i],wcounter[i]);
    }
    // for(unsigned i = 0; i < num_floats; ++i) {
    //   std::cout << "Float " << i << ": "
    //     << int_to_float[i] << " -> " << int_to_float_c[i] << " times" << std::endl;
    // }
    // for(unsigned i = 0; i < Action::__LAST; ++i) {
    //   printf("Action %6u x%6u: %s\n",i,_as_counter[i],Action::name[i].c_str());
    // }
  }

  //Check the RAW_RNG_MASK bit from a frame, and return whether that bit was flipped
  inline bool checkRawRNG(int32_t &frame) const {
    //If first and second bits of frame are different, rng is stored raw
    uint8_t bit1 = (frame >> 31) & 0x01;
    uint8_t bit2 = (frame >> 30) & 0x01;
    uint8_t rng_is_raw = bit1 ^ bit2;
    return rng_is_raw > 0;
  }

  //Predict the next frame given a reference frame and delta encode the difference
  inline int32_t predictFrame(int32_t frame, int32_t ref_frame, char* write_addr = nullptr) const {
    //Remove RNG bit from frame
    bool rng_is_raw  = checkRawRNG(frame);
    if (rng_is_raw) {
      frame ^= RAW_RNG_MASK;
    }
    //Flip 2nd bit in _wb output if raw RNG flag is set
    uint32_t bitmask = rng_is_raw ? RAW_RNG_MASK : 0x00000000;

    int32_t frame_delta_pred;
    if (_is_encoded) { //Decode
      frame_delta_pred = frame + (ref_frame + 1);
      if (write_addr) {
        writeBE4S(frame_delta_pred ^ bitmask,write_addr);
      }
    } else {           //Encode
      frame_delta_pred = frame - (ref_frame + 1);
      if (write_addr) {
        writeBE4S(frame_delta_pred,write_addr);
      }
    }

    return frame_delta_pred;
  }

  //Version of predictFrame used for _unshuffleEvents
  inline int32_t decodeFrame(int32_t frame, int32_t ref_frame) const {
    return predictFrame(frame, ref_frame, nullptr);
  }

  //Create a mapping of floats we've already seen by treating them as unsigned integer indices
  //  to an index build on-the-fly
  inline void buildFloatMap(unsigned off) {
    uint32_t enc_float = readBE4U(&_rb[_bp+off]);
    if (_is_encoded) {                  //Decode
      if ((enc_float & MAGIC_FLOAT) != MAGIC_FLOAT) {
        //If it's our first encounter with a float, add it to our maps
        float_to_int[enc_float]  = num_floats;
        int_to_float[num_floats] = enc_float;
        ++num_floats;
      } else {
        //Decode our int back to a float
        unsigned as_unsigned = int_to_float[enc_float & (MAGIC_FLOAT ^ 0xFFFFFFFF)];
        writeBE4U(as_unsigned, &_wb[_bp+off]);
      }
    } else {                            //Encode
      if (float_to_int.find(enc_float) == float_to_int.end()) {
        //If it's our first encounter with a float, add it to our maps
        float_to_int[enc_float]    = num_floats;
        int_to_float[num_floats]   = enc_float;
        int_to_float_c[num_floats] = 1;
        ++num_floats;
      } else {
        //Set the 2nd and 3rd bit, since they will never be set simultaneously in real floats
        ++int_to_float_c[float_to_int[enc_float]];
        writeBE4U(float_to_int[enc_float] | MAGIC_FLOAT, &_wb[_bp+off]);
      }
    }
  }

  //General purpose function for predicting acceleration based
  //  on float data in three buffers, using a magic float value
  //  to flag whether such an encoded value is present
  inline void predictAccel(unsigned p, unsigned off, char* buff1, char* buff2, char* buff3) {
    union { float f; uint32_t u; } float_true, float_pred, float_temp;
    //If our prediction is accurate to at least the last 8 bits, it's close enough
    uint32_t MAXDIFF = 0xFF;

    float_true.f = readBE4F(&_rb[_bp+off]);
    float pos1   = readBE4F(&buff1[off]);
    float pos2   = readBE4F(&buff2[off]);
    float pos3   = readBE4F(&buff3[off]);
    float vel1   = pos1 - pos2;
    float vel2   = pos2 - pos3;
    float acc1   = vel1 - vel2;
    float_pred.f = pos1 + vel1 + acc1;
    float_temp.u = float_pred.u ^ float_true.u;

    if (_is_encoded) {
      uint32_t diff = float_true.u ^ MAGIC_FLOAT;
      if (diff <= MAXDIFF) {  //If our prediction was accurate to at least the last few bits
        float_pred.u ^= diff;
        writeBE4F(float_pred.f,&_wb[_bp+off]);  //Write our predicted float
      }
    } else {
      //If at most the last few bits are off, we succeeded
      if (float_temp.u <= MAXDIFF) {
        writeBE4U(MAGIC_FLOAT ^ float_temp.u,&_wb[_bp+off]);  //Write an impossible float
      }
    }

    memcpy(&buff3[off], &buff2[off],4);
    memcpy(&buff2[off], &buff1[off],4);
    memcpy(&buff1[off],_is_encoded ? &_wb[_bp+off] : &_rb[_bp+off],4);
  }

  //General purpose function for predicting velocity based
  //  on float data in two buffers, using a magic float value
  //  to flag whether such an encoded value is present
  inline void predictVeloc(unsigned p, unsigned off, char* buff1, char* buff2) {
    union { float f; uint32_t u; } float_true, float_pred, float_temp;

    float_true.f = readBE4F(&_rb[_bp+off]);
    float_pred.f = 2*readBE4F(&buff1[off]) - readBE4F(&buff2[off]);
    float_temp.u = float_pred.u ^ float_true.u;
    if (_is_encoded) {
      if (float_true.u == MAGIC_FLOAT) {  //If our prediction was exactly accurate
        writeBE4F(float_pred.f,&_wb[_bp+off]);  //Write our predicted float
      }
    } else {
      if (float_temp.u == 0) {  //If our prediction was exactly accurate
        writeBE4U(MAGIC_FLOAT,&_wb[_bp+off]);  //Write an impossible float
      }
    }
    memcpy(&buff2[off], &buff1[off],4);
    memcpy(&buff1[off],_is_encoded ? &_wb[_bp+off] : &_rb[_bp+off],4);
  }

  //Using the previous three post-frame events as reference, predict a floating point value, and store
  //  an otherwise-impossible float if our prediction was correct
  inline void predictAccelPost(unsigned p, unsigned off) {
    predictAccel(p,off,_x_post_frame[p],_x_post_frame_2[p],_x_post_frame_3[p]);
  }

  //Using the previous three item events as reference, predict a floating point value, and store
  //  an otherwise-impossible float if our prediction was correct
  inline void predictAccelItem(unsigned p, unsigned off) {
    predictAccel(p,off,_x_item[p],_x_item_2[p],_x_item_3[p]);
  }

  //Using the previous two pre-frame events as reference, predict a floating point value, and store
  //  an otherwise-impossible float if our prediction was correct
  inline void predictVelocPre(unsigned p, unsigned off) {
    predictVeloc(p,off,_x_pre_frame[p],_x_pre_frame_2[p]);
  }

  //Using the previous two post-frame events as reference, predict a floating point value, and store
  //  an otherwise-impossible float if our prediction was correct
  inline void predictVelocPost(unsigned p, unsigned off) {
    predictVeloc(p,off,_x_post_frame[p],_x_post_frame_2[p]);
  }

  //Using the previous two item events with slot id as reference, predict a floating point value, and store
  //  an otherwise-impossible float if our prediction was correct
  inline void predictVelocItem(unsigned p, unsigned off) {
    predictVeloc(p,off,_x_item[p],_x_item_2[p]);
  }

  //Predict the RNG by reading a full (not delta-encoded) frame and
  //  counting the number of rolls it takes to arrive at the correct seed
  inline void predictRNG(unsigned frameoff, unsigned rngoff) {
    // return;
    const uint32_t MAX_ROLLS = 128;

    //Read true frame from write buffer
    int32_t frame;
    if (_is_encoded) {  //Read from write buffer
      frame = readBE4S(&_wb[_bp+frameoff]);
    } else {            //Read from read buffer
      frame = readBE4S(&_rb[_bp+frameoff]);
    }
    bool rng_is_raw = checkRawRNG(frame);
    if (rng_is_raw) {
      frame ^= RAW_RNG_MASK;
    }

    //Get RNG seed and record number of rolls it takes to get to that point
    if ((100 *_slippi_maj + _slippi_min) >= 307) {  //New RNG
      _rng = computeRNGRollback(frame);
      if (_is_encoded) {  //Decode
        if (rng_is_raw == 0) { //Roll RNG a few times until we get to the desired value
          unsigned rolls = readBE4U(&_rb[_bp+rngoff]);
          if (rolls < MAX_ROLLS) {
            for(unsigned i = 0; i < rolls; ++i) {
              _rng = rollRNGRollback(_rng);
            }
          } else {  //Fallback on old RNG rolling method
            for(unsigned i = MAX_ROLLS; i < rolls; ++i) {
              _rng = rollRNGLegacy(_rng);
            }
          }
          writeBE4U(_rng,&_wb[_bp+rngoff]);
        } else {
          writeBE4U(frame,&_wb[_bp+frameoff]);
        }
      } else {  //Encode
        unsigned rolls  = 0;
        unsigned target = readBE4U(&_rb[_bp+rngoff]);
        for(uint32_t start_rng = _rng; (start_rng != target) && (rolls < MAX_ROLLS); ++rolls) {
          start_rng = rollRNGRollback(start_rng);
        }
        if (rolls < MAX_ROLLS) {  //If we can encode RNG as < 256 rolls, do it
          writeBE4U(rolls,&_wb[_bp+rngoff]);
          _rng = target;
        }
        else {
          rolls  = 0;
          for(uint32_t start_rng = _rng; (start_rng != target) && (rolls < MAX_ROLLS); ++rolls) {
            start_rng = rollRNGLegacy(start_rng);
          }
          if (rolls < MAX_ROLLS) {
            writeBE4U(rolls+MAX_ROLLS,&_wb[_bp+rngoff]);
          } else { //Store the raw RNG value
            //Load the predicted frame value
            int32_t predicted_frame = readBE4S(&_wb[_bp+frameoff]);
            //Flip second bit of cached frame number to signal raw rng
            writeBE4S(predicted_frame ^ RAW_RNG_MASK,&_wb[_bp+frameoff]);
          }
        }
      }
    } else { //Old RNG
      if (_is_encoded) {
        unsigned rolls = readBE4U(&_rb[_bp+rngoff]);
        for(unsigned i = 0; i < rolls; ++i) {
          _rng = rollRNGLegacy(_rng);
        }
        writeBE4U(_rng,&_wb[_bp+rngoff]);
      } else { //Roll RNG until we hit the target, write the # of rolls
        unsigned rolls, seed = readBE4U(&_rb[_bp+rngoff]);
        for(rolls = 0;_rng != seed; ++rolls) {
          _rng = rollRNGLegacy(_rng);
        }
        writeBE4U(rolls,&_wb[_bp+rngoff]);
      }
    }
  }

  //XOR bytes in the current read buffer with bytes from another buffer
  inline void xorEncodeRange(unsigned start, unsigned stop, char* buffer) {
    for(unsigned i = start; i < stop; ++i) {
      _wb[_bp+i] = _rb[_bp+i] ^ buffer[i];
      buffer[i]  = _is_encoded ? _wb[_bp+i] : _rb[_bp+i];
    }
  }

  inline bool _shuffleColumns(unsigned *offset) {
      char* main_buf = _wb;

      // Track the starting position of the buffer
      unsigned s = _game_loop_start;
      unsigned *mem_size = new unsigned[1];

      // Shuffle frame start columns
      if (main_buf[s] == Event::FRAME_START) {
        *mem_size = offset[0];
        _transposeEventColumns(&main_buf[s],mem_size,cw_start,false);
        s += *mem_size;
      }

      // Shuffle pre frame columns
      for(unsigned i = 0; i < 8; ++i) {
          if (main_buf[s] != Event::PRE_FRAME) {
              break;
          }
          *mem_size = offset[1+i];
          _transposeEventColumns(&main_buf[s],mem_size,cw_pre,false);
          s += *mem_size;
      }

      // Shuffle item columns
      if (main_buf[s] == Event::ITEM_UPDATE) {
        *mem_size = offset[9];
        _transposeEventColumns(&main_buf[s],mem_size,cw_item,false);
        s += *mem_size;
      }

      // Shuffle post frame columns
      for(unsigned i = 0; i < 8; ++i) {
          if (main_buf[s] != Event::POST_FRAME) {
              break;
          }
          *mem_size = offset[10+i];
          _transposeEventColumns(&main_buf[s],mem_size,cw_post,false);
          s += *mem_size;
      }

      // Shuffle frame end columns
      if (main_buf[s] == Event::BOOKEND) {
        *mem_size = offset[18];
        _transposeEventColumns(&main_buf[s],mem_size,cw_end,false);
        s += *mem_size;
      }

      delete mem_size;

      return true;
  }

  inline bool _unshuffleColumns(char* main_buf) {
      // We need to unshuffle event columns before doing anything else
      // Track the starting position of the buffer
      unsigned s         = _game_loop_start;
      unsigned *mem_size = new unsigned[1]{0};

      // Unshuffle frame start columns
      if (main_buf[s] == Event::FRAME_START) {
        _revertEventColumns(&main_buf[s],mem_size,cw_start);
        s += *mem_size;
      }

      // Unshuffle pre frame columns
      for(unsigned i = 0; i < 8; ++i) {
          if (main_buf[s] != Event::PRE_FRAME) {
              break;
          }
          _revertEventColumns(&main_buf[s],mem_size,cw_pre);
          s += *mem_size;
      }

      // Unshuffle item columns
      if (main_buf[s] == Event::ITEM_UPDATE) {
        _revertEventColumns(&main_buf[s],mem_size,cw_item);
        s += *mem_size;
      }

      // Unshuffle post frame columns
      for(unsigned i = 0; i < 8; ++i) {
          if (main_buf[s] != Event::POST_FRAME) {
              break;
          }
          _revertEventColumns(&main_buf[s],mem_size,cw_post);
          s += *mem_size;
      }

      // Unshuffle frame end columns
      if (main_buf[s] == Event::BOOKEND) {
        _revertEventColumns(&main_buf[s],mem_size,cw_end);
        s += *mem_size;
      }

      // Reset _game_loop_start
      delete mem_size;

      // All done!
      return true;
  }

  inline bool _transposeEventColumns(char* mem_start, unsigned* mem_size, const unsigned col_widths[40], bool unshuffle=false) {

    // Compute the total size of all columns in the event struct
    unsigned struct_size = 0;
    unsigned col_offsets[40] = {0};
    for(unsigned i = 0; i < 40; ++i) {
        col_offsets[i] = struct_size;
        struct_size += col_widths[i];
    }

    // (Unshuffle only) Compute mem_size from consecutive event codes if necessary
    if (unshuffle) {
      // Ignore the existing mem_size, since it's invalid anyway
      *mem_size = 0;
      // We can safely assume the first byte is the event code
      uint8_t ev_code = mem_start[0];
      // While the current byte is still the event code, increment the memory size
      while(mem_start[*mem_size] == ev_code) {
        ++(*mem_size);
      }
      // Multiple the memory size by the size of the struct to get the true memory size
      *mem_size *= struct_size;
    }

    // Create a new buffer for storing all intermediate data
    char* buff = new char[*mem_size];

    // Use struct size to get the total number of entries in the event array
    unsigned num_entries = (*mem_size) / struct_size;
    // std::cout << "Shuffling " << num_entries << " entries" << std::endl;

    // Transpose the columns!
    unsigned b = 0;
    for(unsigned i = 0; i < 40; ++i) {
        if (col_widths[i] == 0) {
            break;
        }
        for (unsigned e = 0; e < num_entries; ++e) {
            unsigned tpos = (e*struct_size+col_offsets[i]);
            memcpy(
                &buff[     unshuffle ? tpos : b],
                &mem_start[unshuffle ? b : tpos],
                sizeof(char)*col_widths[i]
                );
            b += col_widths[i];
        }
    }

    // Copy back the shuffled columns
    memcpy(&mem_start[0], &buff[0], *mem_size);

    // Clear the memory buffer
    delete buff;

    // All done!
    return true;
  }

  inline bool _revertEventColumns(char* mem_start, unsigned* mem_size, const unsigned col_widths[40]) {
    return _transposeEventColumns(mem_start,mem_size,col_widths,true);
  }

};

}

#endif /* COMPRESSOR_H_ */
