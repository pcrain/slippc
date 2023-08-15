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
#include <filesystem>

#include "util.h"
#include "enums.h"
#include "schema.h"
#include "gecko-legacy.h"

// Replay File (.slp) Spec: https://github.com/project-slippi/slippi-wiki/blob/master/SPEC.md

const uint8_t  COMPRETZ_VERSION      = 2;           //Internal version of this compressor

const uint32_t RAW_RNG_MASK          = 0x40000000;  //Second bit of unsigned int
const uint32_t MAGIC_FLOAT           = 0xFF000000;  //First 8 bits of float
const uint32_t EXPONENT_BITS         = 0x7F800000;  //Exponent bits 2-9 of a float
const uint32_t DEFER_ITEM_BITS       = 0xC000;      //Bitmask for storing deferred writes of items

const uint32_t ITEM_SLOTS            = 256;         //Max number of items we expect to track at once
const uint32_t MESSAGE_SIZE          = 517;         //Size of Message Splitter event
const uint32_t MAX_ITEMS_C           = 2048;        //Max number of items to track initially
const uint32_t MAX_ROLLBACK          = 128;         //Max number of frames game can roll back
const int      RB_SIZE               = 4;           //Size of circular queue for tracking repeated frames

const int      FRAME_ENC_DELTA       = 1;           //Delta when predicting and encoding next frame
const int      ALLOC_EVENTS          = 100000;      //Number of events to initially allocate space for shuffling

namespace slip {

class Compressor {
private:

  int             _debug;                         //Current debug level
  uint16_t        _payload_sizes[256] = {0};      //Size of payload for each event
  uint8_t         _slippi_maj         =  0;       //Major version number of replay being parsed
  uint8_t         _slippi_min         =  0;       //Minor version number of replay being parsed
  uint8_t         _slippi_rev         =  0;       //Revision number of replay being parsed
  uint8_t         _encode_ver         =  0;       //Encryption status of replay being parsed
  int32_t         _max_frames         =  0;       //Maximum number of frames that there will be in the replay file
  std::string*    _outfilename        =  nullptr; //Name of the file to write
  std::string*    _outgeckofilename   =  nullptr; //Name of gecko file to write

  //Variables needed for mapping floats to ints and vice versa
  std::map<unsigned,unsigned> float_to_int;  //Map of floats to ints
  std::map<unsigned,unsigned> int_to_float;  //Reverse map of ints back to floats
  std::map<unsigned,unsigned> int_to_float_c;//Counter for reverse map of ints back to floats
  unsigned            num_floats = 0;        //Total number of floats in float map

  uint32_t        _preds = 0;
  uint32_t        _fails = 0;

  const char*     _infilename = "";

  uint32_t        _rng;                                //Current RNG seed we're working with
  uint32_t        _rng_start;                          //Starting RNG seed
  char            _x_pre_frame[8][256]       = {0};    //Delta for pre-frames
  char            _x_pre_frame_2[8][256]     = {0};    //Delta for 2 pre-frames ago
  char            _x_post_frame[8][256]      = {0};    //Delta for post-frames
  char            _x_post_frame_2[8][256]    = {0};    //Delta for 2 post-frames ago
  char            _x_post_frame_3[8][256]    = {0};    //Delta for 3 post-frames ago
  char            _x_item[ITEM_SLOTS][256]   = {0};    //Delta for item updates
  char            _x_item_2[ITEM_SLOTS][256] = {0};    //Delta for item updates 2 frames ago
  char            _x_item_3[ITEM_SLOTS][256] = {0};    //Delta for item updates 3 frames ago
  char            _x_item_4[ITEM_SLOTS][256] = {0};    //Delta for item updates 4 frames ago
  char            _x_item_p[ITEM_SLOTS][256] = {0};    //Delta for item position updates
  int32_t         laststartframe             = -123;   //Last frame used in frame start event, encoding
  int32_t         lastitemstartframe         = -123;   //Last frame used in item event, encoding
  int32_t         lastshuffleframe           = -123;   //Last frame used in frame start event, shuffling
  int32_t         lastitemshuffleframe       = -123;   //Last frame used in item event, shuffling
  int32_t         lastpreframe[8]            = {-123}; //Last frame used in pre frame event, encoding
  int32_t         lastshufflepreframe[8]     = {-123}; //Last frame used in pre frame event, shuffling
  int32_t         lastpostframe[8]           = {-123}; //Last frame used in post frame event, encoding
  int32_t         lastshufflepostframe[8]    = {-123}; //Last frame used in post frame event, shuffling

  char*           _rb                        = nullptr; //Read buffer
  char*           _wb                        = nullptr; //Write buffer
  unsigned        _bp                        = 0;       //Current position in buffer
  uint32_t        _length_raw                = 0;       //Remaining length of raw payload
  uint32_t        _length_raw_start          = 0;       //Total length of raw payload
  uint32_t        _game_loop_start           = 0;       //First byte AFTER game start event
  uint32_t        _game_loop_end             = 0;       //First byte OF game end event
  uint32_t        _file_size                 = 0;       //Total size of the replay file on disk
  uint32_t        _message_count             = 0;       //Number of gecko messages we've parsed thus far
  bool            _game_end_found            = false;   //Whether we've found the game end event

  // Frame event column byte widths (negative numbers denote bit shuffling)
  int32_t         _cw_start[5] = {1,4,4,4,0};
  int32_t         _cw_mesg[6]  = {1,512,2,1,1,0};
  int32_t         _cw_pre[21]  = {1,4,1,1,4,2,4,4,4,4,4,4,4,4,4,2,4,4,1,4,0};
  int32_t         _cw_item[21] = {1,4,2,1,4,4,4,4,4,2,4, 1,1,1,1 ,1,1,1,1,1,0}; // Shuffle bytes of item id
  int32_t         _cw_post[35] = {1,4,1,1,1,2,4,4,4,4,4,1,1,1,1,4,1,1,1,1,1,4,1,2,1,1,1,4,4,4,4,4,4,4,0};
  int32_t         _cw_end[4]   = {1,4,4,0};

  // Debug Frame event column byte widths (negative numbers denote bit shuffling)
  int32_t         _dw_start[5] = {1,4,4,4,0};
  int32_t         _dw_mesg[6]  = {1,512,2,1,1,0};
  int32_t         _dw_pre[21]  = {1,4,1,1,4,2,4,4,4,4,4,4,4,4,4,2,4,4,1,4,0};
  int32_t         _dw_item[21] = {1,4,2,1,4,4,4,4,4,2,4, 1,1,1,1 ,1,1,1,1,1,0}; // Shuffle bytes of item id
  int32_t         _dw_post[35] = {1,4,1,1,1,2,4,4,4,4,4,1,1,1,1,4,1,1,1,1,1,4,1,2,1,1,1,4,4,4,4,4,4,4,0};
  int32_t         _dw_end[4]   = {1,4,4,0};

  bool            _parse();             //Internal main parsing funnction
  bool            _parseHeader();
  bool            _parseEventDescriptions();
  bool            _parseEvents();
  bool            _parseGameStart();
  bool            _parseGeckoCodes();
  bool            _parsePreFrame();
  bool            _parsePostFrame();
  bool            _parseFrameStart();
  bool            _parseItemUpdate();
  bool            _parseBookend();
  bool            _shuffleEvents(bool unshuffle = false);
  bool            _unshuffleEvents();

public:
  Compressor(int debug_level);                     //Instantiate the parser (possibly in debug mode)
  ~Compressor();                                   //Destroy the parser
  bool loadFromFile(const char* replayfilename);   //Load a replay file
  void saveToFile(bool rawencode);              //Save an encoded replay file
  bool setOutputFilename(const char* fname);       //Set output file name
  bool setGeckoOutputFilename(const char* fname);  //Set gecko code output filename
  bool loadFromBuff(char** buffer, unsigned size); //Load a replay from a buffer
  unsigned saveToBuff(char** buffer);              //Save an encoded replay buffer
  bool validate();                                 //Validate the encoding

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
    if (_encode_ver) { //Decode
      frame_delta_pred = frame + (ref_frame + FRAME_ENC_DELTA);
      if (write_addr) {
        writeBE4S(frame_delta_pred ^ bitmask,write_addr);
      }
    } else {           //Encode
      frame_delta_pred = frame - (ref_frame + FRAME_ENC_DELTA);
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
    if (_encode_ver) {                  //Decode
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

  //Compress analog float values by converting them to ints
  inline void encodeAnalog(unsigned off, float mult) {
    union { float f; uint32_t u; uint8_t c[4]; int8_t i[4]; } float_true, float_pred, float_rest;

    // If any of exponent bits are set, we still have a float
    char* main_buf = _encode_ver ? _wb : _rb;
    float_true.u   = readBE4U(&main_buf[_bp+off]);
    if ((float_true.u & EXPONENT_BITS) && _encode_ver) {
      return;  //If we have a float in the encoded state, nothing to do
    }

    float_pred.u = 0;  //Initialize all bytes to zero
    if (_encode_ver) {
      if (mult > 127) {
        float_pred.f = float(float_true.c[0])/mult;
      } else {
        float_pred.f = float(float_true.i[0])/mult;
      }
      writeBE4F(float_pred.f,&_wb[_bp+off]);  //Write our decoded float
    } else {
      if (mult > 127) {
        float_pred.c[0] = uint8_t(round(float(float_true.f*mult)));
        float_rest.f = float(float_pred.c[0])/mult;
      } else {
        float_pred.i[0] = int8_t(round(float(float_true.f*mult)));
        float_rest.f = float(float_pred.i[0])/mult;
      }
      if (float_rest.u == float_true.u) {  //Verify we can properly decode
        writeBE4U(float_pred.u,&_wb[_bp+off]);  //Write our encoded int
      }
    }
  }

  //General purpose function for encoding the difference
  //  between floats in two buffers, using a magic float value
  //  to flag whether such an encoded value is present
  //  (useful for computing item velocities in terms of positions)
  inline void predictAsDifference(unsigned off, unsigned buffoff, char* buff) {
    union { float f; uint32_t u; } float_true, float_pred, float_temp;
    //If our prediction is accurate to at least the last 10 bits, it's close enough
    uint32_t MAXDIFF = 0x03FF;

    char* main_buf = _encode_ver ? _wb : _rb;

    float_true.f = readBE4F(&main_buf[_bp+off]);
    float_pred.f = readBE4F(&main_buf[_bp+buffoff]) - readBE4F(&buff[buffoff]);
    float_temp.u = float_pred.u ^ float_true.u;

    if (_encode_ver) {
      uint32_t diff = float_true.u ^ MAGIC_FLOAT;
      if (diff <= MAXDIFF) {  //If our prediction was accurate to at least the last few bits
        float_pred.u ^= diff;
        writeBE4F(float_pred.f,&_wb[_bp+off]);  //Write our predicted float
      }
    } else {
      //If at most the last few bits are off, we succeeded
      if (float_temp.u <= MAXDIFF) {
        writeBE4U(MAGIC_FLOAT ^ float_temp.u,&_wb[_bp+off]);  //Write an impossible float
      } else {
        // std::cout << "DIFF =" << float_temp.u << std::endl;
      }
    }

    memcpy(&buff[buffoff],&main_buf[_bp+buffoff],4);
  }

  //General purpose function for predicting jolt based
  //  on float data in four buffers, using a magic float value
  //  to flag whether such an encoded value is present
  inline void predictJolt(unsigned p, unsigned off, char* buff1, char* buff2, char* buff3, char* buff4, bool verbose = false) {
    union { float f; uint32_t u; } float_true, float_pred, float_temp;
    //If our prediction is accurate to at least the last 8 bits, it's close enough
    const uint32_t MAXDIFF = 0xFF;

    float_true.f = readBE4F(&_rb[_bp+off]);
    float pos1   = readBE4F(&buff1[off]);
    float pos2   = readBE4F(&buff2[off]);
    float pos3   = readBE4F(&buff3[off]);
    float pos4   = readBE4F(&buff4[off]);
    float vel1   = pos1 - pos2;
    float vel2   = pos2 - pos3;
    float vel3   = pos3 - pos4;
    float acc1   = vel1 - vel2;
    float acc2   = vel2 - vel3;
    float jlt1   = acc1 - acc2;
    float_pred.f = pos1 + vel1 + acc1 + jlt1;

    if (_encode_ver) {
      uint32_t diff = float_true.u ^ MAGIC_FLOAT;
      if (diff <= MAXDIFF) {  //If our prediction was accurate to at least the last few bits
        float_pred.u ^= diff;
        writeBE4F(float_pred.f,&_wb[_bp+off]);  //Write our predicted float
      }
    } else {
      //If at most the last few bits are off, we succeeded
      if (verbose) {
        if (float_pred.u==float_true.u) {
          std::cout << " predicted " << float_pred.f << ", exact" << std::endl;
        } else {
          std::streamsize ss = std::cout.precision();
          std::cout << std::setprecision(30);
          std::cout << " predicted " << float_pred.f << ", error = " << (float_pred.f-float_true.f) << std::endl;
          std::cout << "   pos1 " << pos1 << std::endl;
          std::cout << "   vel1 " << vel1 << std::endl;
          std::cout << "   acc1 " << acc1 << std::endl;
          std::cout << "   jlt1 " << jlt1 << std::endl;
          std::cout << "   pos2 " << pos2 << std::endl;
          std::cout << "   vel2 " << vel2 << std::endl;
          std::cout << "   acc2 " << acc2 << std::endl;
          std::cout << "   pos3 " << pos3 << std::endl;
          std::cout << "   vel3 " << vel3 << std::endl;
          std::cout << "   pos4 " << pos4 << std::endl;
          std::cout << std::setprecision(ss);
        }
      }
      float_temp.u = float_pred.u ^ float_true.u;
      if (float_temp.u <= MAXDIFF) {
        if (verbose) {
          std::cout << "    we compress those " << pos3 << std::endl;
        }
        writeBE4U(MAGIC_FLOAT ^ float_temp.u,&_wb[_bp+off]);  //Write an impossible float
      }
    }

    memcpy(&buff4[off], &buff3[off],4);
    memcpy(&buff3[off], &buff2[off],4);
    memcpy(&buff2[off], &buff1[off],4);
    memcpy(&buff1[off],_encode_ver ? &_wb[_bp+off] : &_rb[_bp+off],4);
  }

  //General purpose function for predicting acceleration based
  //  on float data in three buffers, using a magic float value
  //  to flag whether such an encoded value is present
  inline void predictAccel(unsigned p, unsigned off, char* buff1, char* buff2, char* buff3, bool verbose = false) {
    union { float f; uint32_t u; } float_true, float_pred, float_temp;
    //If our prediction is accurate to at least the last 8 bits, it's close enough
    const uint32_t MAXDIFF = 0xFF;

    float_true.f = readBE4F(&_rb[_bp+off]);
    float pos1   = readBE4F(&buff1[off]);
    float pos2   = readBE4F(&buff2[off]);
    float pos3   = readBE4F(&buff3[off]);
    float vel1   = pos1 - pos2;
    float vel2   = pos2 - pos3;
    float acc1   = vel1 - vel2;
    float_pred.f = pos1 + vel1 + acc1;

    if (_encode_ver) {
      uint32_t diff = float_true.u ^ MAGIC_FLOAT;
      if (diff <= MAXDIFF) {  //If our prediction was accurate to at least the last few bits
        float_pred.u ^= diff;
        writeBE4F(float_pred.f,&_wb[_bp+off]);  //Write our predicted float
      }
    } else {
      //If at most the last few bits are off, we succeeded
      if (verbose) {
        if (float_pred.u==float_true.u) {
          std::cout << " predicted " << float_pred.f << ", exact" << std::endl;
        } else {
          std::streamsize ss = std::cout.precision();
          std::cout << std::setprecision(30);
          std::cout << " predicted " << float_pred.f << ", error = " << (float_pred.f-float_true.f) << std::endl;
          std::cout << "   pos1 " << pos1 << std::endl;
          std::cout << "   vel1 " << vel1 << std::endl;
          std::cout << "   acc1 " << acc1 << std::endl;
          std::cout << "   pos2 " << pos2 << std::endl;
          std::cout << "   vel2 " << vel2 << std::endl;
          std::cout << "   pos3 " << pos3 << std::endl;
          std::cout << std::setprecision(ss);
        }
      }
      float_temp.u = float_pred.u ^ float_true.u;
      if (float_temp.u <= MAXDIFF) {
        writeBE4U(MAGIC_FLOAT ^ float_temp.u,&_wb[_bp+off]);  //Write an impossible float
      }
    }

    memcpy(&buff3[off], &buff2[off],4);
    memcpy(&buff2[off], &buff1[off],4);
    memcpy(&buff1[off],_encode_ver ? &_wb[_bp+off] : &_rb[_bp+off],4);
  }

  //General purpose function for predicting velocity based
  //  on float data in two buffers, using a magic float value
  //  to flag whether such an encoded value is present
  inline void predictVeloc(unsigned p, unsigned off, char* buff1, char* buff2) {
    union { float f; uint32_t u; } float_true, float_pred, float_temp;

    float_true.f = readBE4F(&_rb[_bp+off]);
    float_pred.f = 2*readBE4F(&buff1[off]) - readBE4F(&buff2[off]);
    float_temp.u = float_pred.u ^ float_true.u;
    if (_encode_ver) {
      if (float_true.u == MAGIC_FLOAT) {  //If our prediction was exactly accurate
        writeBE4F(float_pred.f,&_wb[_bp+off]);  //Write our predicted float
      }
    } else {
      if (float_temp.u == 0) {  //If our prediction was exactly accurate
        writeBE4U(MAGIC_FLOAT,&_wb[_bp+off]);  //Write an impossible float
      }
    }

    memcpy(&buff2[off], &buff1[off],4);
    memcpy(&buff1[off],_encode_ver ? &_wb[_bp+off] : &_rb[_bp+off],4);
  }

  //Using the previous four item events as reference, predict a floating point value, and store
  //  an otherwise-impossible float if our prediction was correct
  inline void predictJoltItem(unsigned p, unsigned off, bool verbose = false) {
    predictJolt(p,off,_x_item[p],_x_item_2[p],_x_item_3[p],_x_item_4[p],verbose);
  }

  //Using the previous three post-frame events as reference, predict a floating point value, and store
  //  an otherwise-impossible float if our prediction was correct
  inline void predictAccelPost(unsigned p, unsigned off) {
    predictAccel(p,off,_x_post_frame[p],_x_post_frame_2[p],_x_post_frame_3[p]);
  }

  //Using the previous three item events as reference, predict a floating point value, and store
  //  an otherwise-impossible float if our prediction was correct
  inline void predictAccelItem(unsigned p, unsigned off, bool verbose = false) {
    predictAccel(p,off,_x_item[p],_x_item_2[p],_x_item_3[p],verbose);
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
    const uint32_t MAX_ROLLS = 128;  //must be a power of 2

    //Read true frame from write buffer
    int32_t frame;
    if (_encode_ver) {  //Read from write buffer
      frame = readBE4S(&_wb[_bp+frameoff]);
    } else {            //Read from read buffer
      frame = readBE4S(&_rb[_bp+frameoff]);
    }
    bool rng_is_raw = checkRawRNG(frame);
    if (rng_is_raw) {
      frame ^= RAW_RNG_MASK;
    }

    //Get RNG seed and record number of rolls it takes to get to that point
    if (MIN_VERSION(3,6,0)) {  //New RNG
      _rng = computeRNGRollback(frame);
      if (_encode_ver) {  //Decode
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
          // std::cout << "Rollback rolled " << rolls << " at byte " << _bp << " frame " << frame << std::endl;
          writeBE4U(rolls,&_wb[_bp+rngoff]);
          _rng = target;
        } else {
          rolls  = 0;
          for(uint32_t start_rng = _rng; (start_rng != target) && (rolls < MAX_ROLLS); ++rolls) {
            start_rng = rollRNGLegacy(start_rng);
          }
          if (rolls < MAX_ROLLS) {
            // std::cout << "Legacy rolled " << rolls << " at byte " << _bp << " frame " << frame << std::endl;
            writeBE4U(rolls+MAX_ROLLS,&_wb[_bp+rngoff]);
          } else { //Store the raw RNG value
            // std::cout << "Legacy failed at byte " << _bp << " frame " << frame << std::endl;
            //Load the predicted frame value
            int32_t predicted_frame = readBE4S(&_wb[_bp+frameoff]);
            //Flip second bit of cached frame number to signal raw rng
            writeBE4S(predicted_frame ^ RAW_RNG_MASK,&_wb[_bp+frameoff]);
          }
        }
      }
    } else { //Old RNG
      if (_encode_ver) {
        unsigned rolls = readBE4U(&_rb[_bp+rngoff]);
        for(unsigned i = 0; i < rolls; ++i) {
          _rng = rollRNGLegacy(_rng);
        }
        writeBE4U(_rng,&_wb[_bp+rngoff]);
      } else { //Roll RNG until we hit the target, write the # of rolls
        unsigned rolls, seed = readBE4U(&_rb[_bp+rngoff]);
        for(rolls = 0; _rng != seed; ++rolls) {
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
      buffer[i]  = _encode_ver ? _wb[_bp+i] : _rb[_bp+i];
    }
  }

  inline bool _shuffleItems(char* iblock_start, unsigned iblock_len, bool shuffle=true) {
    // allocate temporary buffers for shuffling
    // TODO: dynamically reallocate this array later
    unsigned ps         = _payload_sizes[Event::ITEM_UPDATE];
    unsigned* icount    = new unsigned[MAX_ITEMS_C]{0};
    unsigned* ilast     = new unsigned[MAX_ITEMS_C]{0};
    char** ibuffs       = new char*[MAX_ITEMS_C];
    for(unsigned i = 0; i < MAX_ITEMS_C; ++i) {
      ibuffs[i] = nullptr;
    }

    // bin item payloads by item ID
    bool success      = true;
    bool first_item   = true;  //apparently the first item isn't always id 0
    unsigned wait     = 0;
    unsigned ev_total = 0;
    unsigned cur_id   = 0;
    unsigned max_item = 0;
    for (unsigned i = 0; i < iblock_len; i += ps) {
      // read the raw (possibly encoded) item id
      uint32_t ouid   = readBE4U(&iblock_start[i+O_ITEM_ID]);
      // parse out the frame modulus from the item id
      uint32_t uid    = encodeFrameIntoItemId(ouid,getFrameModFromItemId(ouid));

      // if we're shuffling, we need to do some counting
      if(shuffle) {
        unsigned encid;
        if(icount[uid] == 0) {
          // get the number of elapsed item events since the last new item,
          encid = encodeWaitIntoItemId(ouid,wait);
          // if this isn't item 0, flag this as a new item
          if(!first_item) {
            // ids aren't always consecutive, so count the item id delta since last item
            unsigned skip = 0;
            while(icount[uid-skip] == 0) {
              ++skip;
            }
            encid = encodeNewItemIntoId(encid,skip);
          } else {
            // if our first item isn't id 0, encode its uid as the skip value
            encid = encodeNewItemIntoId(encid,uid);
            // set first item status to false
            first_item = false;
          }

          // set the wait since last new item to 1
          wait = 0;
        } else {
          // get the number of elapsed item events since the last time we saw this item
          encid = encodeWaitIntoItemId(ouid,ev_total-ilast[uid]);
        }

        // increment the waiting counter
        wait += 1;

        // update the last time we saw this item with ev_total
        ilast[uid] = ev_total;

        // increment the total events
        ev_total += 1;

        // write the new encoded item back to the block to be copied
        writeBE4U(encid,&iblock_start[i+O_ITEM_ID]);
      } else {  // otherwise, we need to do some unpacking
        // if we've found a new item, increment our cur_id and copy as appropriate into the block
        unsigned skip = getIsNewItem(ouid);
        if(skip) {
          cur_id += skip;
          writeBE4U(encodeNewItemIntoId(ouid,skip),&iblock_start[i+O_ITEM_ID]);
        }
        // set uid to the current item
        uid = cur_id;
      }

      // allocate a buffer for this item if we don't have one already
      if(ibuffs[uid] == nullptr) {
        ibuffs[uid] = new char[iblock_len];
      }

      // copy an item payload into each item's individual buffer
      memcpy(&(ibuffs[uid][ps*icount[uid]]),&iblock_start[i],ps);

      // increment number of payloads for this item
      icount[uid] += 1;

      // keep track of the highest item id we've encountered so far
      if (uid >= max_item) {
        max_item = uid+1;
      }
    }

    // if we're shuffling, copy these payloads in item order back into main memory
    if(shuffle) {
      unsigned ind    = 0;
      for (unsigned n = 0; ind != iblock_len; ++n) {
        if (icount[n] == 0) {
          continue;
        }
        memcpy(&iblock_start[ind],&(ibuffs[n][0]),icount[n]*ps);
        ind += icount[n]*ps;
      }
    } else { // otherwise, gotta do lots of math to unshuffle everything
      ev_total        = 0;
      unsigned start  = 0;
      unsigned waited = 0;
      unsigned ind    = 0;
      unsigned* ipos  = new unsigned[max_item]{0};
      // until we've copied every block
      while(ind < iblock_len) {
        bool written = false;
        // from the first active item until the last one
        for (unsigned n = start; n < max_item; ++n) {
          // if this item doesn't acutally exist, we're done with this iteration
          if (icount[n] == 0) {
            continue;
          }
          // if we've already copied all of this item over, go on to the next one
          if (ipos[n] == icount[n]) {
            // if this item is the start, move our start forward
            if (n == start) {
              ++start;
            }
            continue;
          }

          // determine whether we've spent an appropriate amount time waiting for previous item events
          bool donewaiting;
          unsigned ouid = readBE4U(&(ibuffs[n][ipos[n]*ps+O_ITEM_ID]));
          unsigned wait = getWaitFromItemId(ouid);
          // if this is the first time this item appears, wait time is based off item events since last new item
          if (ipos[n] == 0) {
            donewaiting = (wait == waited);
          } else {  //otherwise, wait time is based on item events since this item last appeared
            donewaiting = (wait == (ev_total-ilast[n]));
          }

          // if we're done waiting, it's time to copy this item back into the buffer
          if (donewaiting) {
            if (ipos[n] == 0) {
              // if this is the first time this item has appeared, reset new item wait timer
              waited = 0;
            }
            ilast[n] = ev_total;
            // decode the actual item ID without the wait time and put it back into memory
            writeBE4U(encodeWaitIntoItemId(ouid,n),&ibuffs[n][ipos[n]*ps+O_ITEM_ID]);
            // copy over to main memory
            memcpy(&iblock_start[ind],&(ibuffs[n][ipos[n]*ps]),ps);
            // update counters as appropriate
            ind      += ps;
            ipos[n]  += 1;
            waited   += 1;
            ev_total += 1;
            written = true;
            continue;
          }

          // if we've never actually written this item before, we haven't written any items after it
          //   either, so we're done here
          if (ipos[n] == 0) {
            break;
          } else {  //otherwise, maybe we got rollbacked into this item, so we can continue looking
            continue;
          }
        }
        if(!written) {
          YIKES("  Got stuck writing item data");
          success = false;
          break;
        }
      }
      delete[] ipos;  //delete position buffer
    }

    // delete all the temporary buffers
    for(unsigned i = 0; i < MAX_ITEMS_C; ++i) {
      if (ibuffs[i] != nullptr) {
        delete[] ibuffs[i];
      }
    }
    delete[] ibuffs;
    delete[] icount;
    delete[] ilast;
    return success;
  }

  inline bool _unshuffleItems(char* iblock_start, unsigned iblock_len) {
    return _shuffleItems(iblock_start, iblock_len, false);
  }

  inline bool _shuffleColumns(unsigned *offset) {
      truncateColumnWidthsToVersion();
      char* main_buf = _wb;

      // Track the starting position of the buffer
      unsigned s         = _game_loop_start;
      unsigned *mem_size = new unsigned[1];

      // Shuffle message columns
      if (main_buf[s] == Event::SPLIT_MSG) {
        *mem_size = offset[19];
        _transposeEventColumns(main_buf,s,mem_size,_debug ? this->_dw_mesg : this->_cw_mesg,false);
        s += *mem_size;
      }

      // Shuffle frame start columns
      if (main_buf[s] == Event::FRAME_START) {
        *mem_size = offset[0];
        _transposeEventColumns(main_buf,s,mem_size,_debug ? this->_dw_start : this->_cw_start,false);
        s += *mem_size;
      }

      // Shuffle pre frame columns
      for(unsigned p = 0; p < 8; ++p) {
          // True player order: 0,4,1,5,2,6,3,7
          unsigned i = (4*(p%2)) + unsigned(p/2);
          if (main_buf[s] != Event::PRE_FRAME) {
              break;
          }
          *mem_size = offset[1+i];
          if (*mem_size == 0) {
            continue;
          }
          _transposeEventColumns(main_buf,s,mem_size,_debug ? this->_dw_pre : this->_cw_pre,false);
          s += *mem_size;
      }


      // Shuffle item columns
      if (main_buf[s] == Event::ITEM_UPDATE) {
        *mem_size = offset[9];
        if(ENCODE_VERSION_MIN(2)) {
          _shuffleItems(&main_buf[s],*mem_size);
        }
        _transposeEventColumns(main_buf,s,mem_size,_debug ? this->_dw_item : this->_cw_item,false);
        s += *mem_size;
      }

      // Shuffle post frame columns
      for(unsigned p = 0; p < 8; ++p) {
          // True player order: 0,4,1,5,2,6,3,7
          unsigned i = (4*(p%2)) + unsigned(p/2);
          if (main_buf[s] != Event::POST_FRAME) {
              break;
          }
          *mem_size = offset[10+i];
          if (*mem_size == 0) {
            continue;
          }
          _transposeEventColumns(main_buf,s,mem_size,_debug ? this->_dw_post : this->_cw_post,false);
          s += *mem_size;
      }

      // Shuffle frame end columns
      if (main_buf[s] == Event::BOOKEND) {
        *mem_size = offset[18];
        _transposeEventColumns(main_buf,s,mem_size,_debug ? this->_dw_end : this->_cw_end,false);
        s += *mem_size;
      }

      delete[] mem_size;

      return true;
  }

  inline bool _unshuffleColumns(char* main_buf) {
      truncateColumnWidthsToVersion();
      // We need to unshuffle event columns before doing anything else
      // Track the starting position of the buffer
      unsigned s         = _game_loop_start;
      unsigned *mem_size = new unsigned[1]{0};

      // Unshuffle message columns
      if (main_buf[s] == Event::SPLIT_MSG) {
        _revertEventColumns(main_buf,s,mem_size,_debug ? this->_dw_mesg : this->_cw_mesg);
        s += *mem_size;
      }

      // Unshuffle frame start columns
      if (main_buf[s] == Event::FRAME_START) {
        _revertEventColumns(main_buf,s,mem_size,_debug ? this->_dw_start : this->_cw_start);
        s += *mem_size;
      }

      // Unshuffle pre frame columns
      for(unsigned i = 0; i < 8; ++i) {
          if (main_buf[s] != Event::PRE_FRAME) {
              break;
          }
          _revertEventColumns(main_buf,s,mem_size,_debug ? this->_dw_pre : this->_cw_pre);
          s += *mem_size;
      }

      // Unshuffle item columns
      if (main_buf[s] == Event::ITEM_UPDATE) {
        _revertEventColumns(main_buf,s,mem_size,_debug ? this->_dw_item : this->_cw_item);
        if(ENCODE_VERSION_MIN(2)) {
          _unshuffleItems(&main_buf[s],*mem_size);
        }
        s += *mem_size;
      }

      // Unshuffle post frame columns
      for(unsigned i = 0; i < 8; ++i) {
          if (main_buf[s] != Event::POST_FRAME) {
              break;
          }
          _revertEventColumns(main_buf,s,mem_size,_debug ? this->_dw_post : this->_cw_post);
          s += *mem_size;
      }

      // Unshuffle frame end columns
      if (main_buf[s] == Event::BOOKEND) {
        _revertEventColumns(main_buf,s,mem_size,_debug ? this->_dw_end : this->_cw_end);
        s += *mem_size;
      }

      // Reset _game_loop_start
      delete[] mem_size;

      // All done!
      return true;
  }

  inline bool _transposeEventColumns(char* mem_buf, unsigned mem_off, unsigned* mem_size, const int col_widths[], bool unshuffle=false) {
    bool shuffle    = !unshuffle;        //Convenience reference to whether we're shuffled
    char* mem_start = &mem_buf[mem_off]; //Convenience reference to proper memory offset
    uint8_t ev_code = mem_start[0];      //Always an event code regardless of shuffling

    // Compute the total size of all columns in the event struct
    unsigned struct_size       = 0;
    unsigned col_offsets[1024] = {0};
    for(unsigned i = 0; col_widths[i] != 0; ++i) {
        col_offsets[i] = struct_size;
        if (col_widths[i] > 0) {
          struct_size += col_widths[i];
        } else {  //Bit shuffling columns are always one byte
          struct_size += 1;
        }
    }

    // (Unshuffle only) Compute mem_size from consecutive event codes if necessary
    if (unshuffle) {
      // Ignore the existing mem_size, since it's invalid anyway
      *mem_size = 0;
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
    DOUT3("Shuffling " << num_entries << " entries");

    // Transpose the columns!
    unsigned b = 0;
    for(unsigned i = 0; col_widths[i] != 0; ++i) {
      unsigned block_start = mem_off+b;
      if (col_widths[i] > 0) {  //Normal column shuffling
        for (unsigned e = 0; e < num_entries; ++e) {
          unsigned mempos = (e*struct_size+col_offsets[i]);
          memcpy(
              &buff[     unshuffle ? mempos : b],
              &mem_start[unshuffle ? b : mempos],
              col_widths[i]
              );
          b += col_widths[i];
        }
      } else {  //If col_widths[i] < 0, then use bitwise column shuffling
        for (int bit = 7, ib = 7; ib >= 0; --ib) {
          for (unsigned e = 0; e < num_entries; ++e) {
            unsigned mempos = (e*struct_size+col_offsets[i]);
            char r_byte     = mem_start[shuffle ? mempos : b];
            char r_bit      = (r_byte >> (shuffle ? ib : bit)) & 0x01;
            char w_bit      = r_bit << (shuffle ? bit : ib);
            buff[shuffle ? b : mempos] ^= w_bit;
            if ((--bit) < 0) {
              bit   = 7;
              b    += 1;
            }
          }
        }
      }
      DOUT3("SHUFFLE " << hex(ev_code) << " column " << i << " at " << block_start << " to " << mem_off+b);
    }

    // Copy back the shuffled columns
    memcpy(&mem_start[0], &buff[0], *mem_size);

    // Clear the memory buffer
    delete[] buff;

    // All done!
    return true;
  }

  inline bool _revertEventColumns(char* mem_buf, unsigned mem_off, unsigned* mem_size, const int col_widths[]) {
    return _transposeEventColumns(mem_buf,mem_off,mem_size,col_widths,true);
  }

  inline unsigned encodeFrameIntoItemId(unsigned item_id, int32_t frame) const {
    return item_id ^ (((frame+256) % 256) << 24);
  }

  inline unsigned getFrameModFromItemId(unsigned item_id) const {
    return (item_id >> 24);
  }

  // first two bytes from right
  inline unsigned encodeWaitIntoItemId(unsigned item_id, unsigned wait) const {
    return (item_id & 0xFFFF0000) ^ ((wait+65536) % 65536);
  }

  inline unsigned getWaitFromItemId(unsigned item_id) const {
    return item_id % 65536;
  }

  // 3rd byte from right
  inline unsigned encodeNewItemIntoId(unsigned item_id, unsigned skip) const {
    return (item_id ^ (skip << 16));
  }

  inline unsigned getIsNewItem(unsigned item_id) const {
    return ((item_id << 8) >> 24);
  }

  inline unsigned encodeDeferInfo(unsigned u16, unsigned defer) const {
    return u16 | (defer << 14);
  }

  inline unsigned getDeferInfo(unsigned u16) const {
    return (u16&DEFER_ITEM_BITS) >> 14;
  }

  inline unsigned decodeDeferInfo(unsigned u16) const {
    return u16 & (~DEFER_ITEM_BITS);
  }

  inline int32_t lookAheadToFinalizedFrame(char* mem_start, int32_t ref_frame=0) const {
    unsigned mem_off = 0;
    while(mem_start[mem_off] != Event::BOOKEND) {
      mem_off += _payload_sizes[uint8_t(mem_start[mem_off])];
    }
    return readBE4S(&mem_start[mem_off+O_ROLLBACK_FRAME]);
  }

  inline bool ensureAppropriateFilename() {
    const char* ext = (_encode_ver > 0) ? ".slp" : ".zlp";
    if (_outfilename == nullptr) {
      std::string fname = std::string(_infilename);
      _outfilename = new std::string(
        fname.substr(0,
          fname.find_last_of("."))+
          std::string(ext));
      if (fileExists(*_outfilename)) {
        FAIL("    File " << *_outfilename << " already exists or is invalid");
        return false;
      }
    } else {
      if (!ensureExt(ext,_outfilename->c_str())) {
        FAIL("    File " << *_outfilename << " does not have required extension " << ext);
        return false;
      }
    }
    return true;
  }

  static inline const char* readLegacyGeckoCodes() {
    static bool _legacy_codes_codes_read = false;
    static char _legacy_gecko_codes[32571];  //exact magic number of decompressed bytes
    if (! _legacy_codes_codes_read) {
      _legacy_codes_codes_read = true;
      // decompress the legacy gecko code data
      std::string decomp = decompressWithLzma(GECKO_LZMA,GECKO_LZMA_LEN);
      // Copy buffer from the decompressed string
      memcpy(_legacy_gecko_codes,decomp.c_str(),decomp.size());
    }
    return _legacy_gecko_codes;
  }

  inline void truncateColumnWidthsToVersion() {
    if (MAX_VERSION(3,11,0)) {
      this->_cw_post[33] = 0; //Animation index is invalid
      this->_dw_post[33] = 0;
    }
    if (MAX_VERSION(3,10,0)) {
      this->_cw_start[3] = 0; //Scene frame counter is invalid
      this->_dw_start[3] = 0;
    }
    if (MAX_VERSION(3,8,0)) {
      this->_cw_post[32] = 0; //Hitlag and onward is invalid
      this->_dw_post[32] = 0;
    }
    if (MAX_VERSION(3,7,0)) {
      this->_cw_end[2] = 0; //Rollback frame is invalid
      this->_dw_end[2] = 0;
    }
    if (MAX_VERSION(3,6,0)) {
      this->_cw_item[19] = 0; //Item owner is invalid
      this->_dw_item[19] = 0;
    }
    if (MAX_VERSION(3,5,0)) {
      this->_cw_post[27] = 0; //Self-induced Air x Speed and onward are invalid
      this->_dw_post[27] = 0;
    }
    if (MAX_VERSION(3,2,0)) {
      this->_cw_item[15] = 0; //Item state bits are invalid
      this->_dw_item[15] = 0;
    }
    if (MAX_VERSION(2,1,0)) {
      this->_cw_post[26] = 0; //Hurtbox Collision State and onward are invalid
      this->_dw_post[26] = 0;
    }
    if (MAX_VERSION(2,0,0)) {
      this->_cw_post[16] = 0; //State bit flags 1 and onward are invalid
      this->_dw_post[16] = 0;
    }
    if (MAX_VERSION(1,4,0)) {
      this->_cw_pre[19] = 0; //Pre-frame damage percent is invalid
      this->_dw_pre[19] = 0;
    }
    if (MAX_VERSION(1,2,0)) {
      this->_cw_pre[18] = 0; //UCF X-analog is invalid
      this->_dw_pre[18] = 0;
    }
  }

};

}

#endif /* COMPRESSOR_H_ */
