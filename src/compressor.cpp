#include "compressor.h"

//Debug output convenience macros
#define DOUT1(s) if (_debug >= 1) { std::cout << s; }
#define DOUT2(s) if (_debug >= 2) { std::cout << s; }
#define FAIL(e) std::cerr << "ERROR: " << e << std::endl
#define FAIL_CORRUPT(e) std::cerr << "ERROR: " << e << "; replay may be corrupt" << std::endl

namespace slip {

  Compressor::Compressor(int debug_level) {
    _debug = debug_level;
    _bp    = 0;
  }

  Compressor::~Compressor() {
    if (_rb != nullptr) { delete [] _rb; }
    if (_wb != nullptr) { delete [] _wb; }
    _cleanup();
  }

  bool Compressor::load(const char* replayfilename) {
    DOUT1("Loading " << replayfilename << std::endl);
    std::ifstream myfile;
    myfile.open(replayfilename,std::ios::binary | std::ios::in);
    if (myfile.fail()) {
      FAIL("File " << replayfilename << " could not be opened or does not exist");
      return false;
    }

    myfile.seekg(0, myfile.end);
    _file_size = myfile.tellg();
    if (_file_size < MIN_REPLAY_LENGTH) {
      FAIL("File " << replayfilename << " is too short to be a valid Slippi replay");
      return false;
    }
    DOUT1("  File Size: " << +_file_size << std::endl);
    myfile.seekg(0, myfile.beg);

    _rb = new char[_file_size];
    myfile.read(_rb,_file_size);
    myfile.close();

    _wb           = new char[_file_size];
    memcpy(_wb,_rb,sizeof(char)*_file_size);

    return this->_parse();
  }

  bool Compressor::_parse() {
    if (not this->_parseHeader()) {
      std::cerr << "Failed to parse header" << std::endl;
      return false;
    }
    if (not this->_parseEventDescriptions()) {
      std::cerr << "Failed to parse event descriptions" << std::endl;
      return false;
    }
    if (not this->_parseEvents()) {
      std::cerr << "Failed to parse events proper" << std::endl;
      return false;
    }
    if (not this->_parseMetadata()) {
      std::cerr << "Warning: failed to parse metadata" << std::endl;
      //Non-fatal if we can't parse metadata, so don't need to return false
    }
    DOUT1("Successfully parsed replay!" << std::endl);
    return true;
  }

  bool Compressor::_parseHeader() {
    DOUT1("Parsing header" << std::endl);
    _bp = 0; //Start reading from byte 0

    //First 15 bytes contain header information
    if (same8(&_rb[_bp],SLP_HEADER)) {
      DOUT1("  Slippi Header Matched" << std::endl);
    } else {
      FAIL_CORRUPT("Header did not match expected Slippi file header");
      return false;
    }
    _length_raw_start = readBE4U(&_rb[_bp+11]);
    if(_length_raw_start == 0) {  //TODO: this is /technically/ recoverable
      FAIL_CORRUPT("0-byte raw data detected");
      return false;
    }
    DOUT1("  Raw portion = " << _length_raw_start << " bytes" << std::endl);
    if (_length_raw_start > _file_size) {
      FAIL_CORRUPT("Raw data size exceeds file size of " << _file_size << " bytes");
      return false;
    }
    _length_raw = _length_raw_start;
    _bp += 15;
    return true;
  }

  bool Compressor::_parseEventDescriptions() {
    DOUT1("Parsing event descriptions" << std::endl);

    //Next 2 bytes should be 0x35
    if (_rb[_bp] != Event::EV_PAYLOADS) {
      FAIL_CORRUPT("Expected Event 0x" << std::hex
        << Event::EV_PAYLOADS << std::dec << " (Event Payloads)");
      return false;
    }
    uint8_t ev_bytes = _rb[_bp+1]-1; //Subtract 1 because the last byte we read counted as part of the payload
    _payload_sizes[Event::EV_PAYLOADS] = int32_t(ev_bytes+1);
    DOUT1("  Event description length = " << int32_t(ev_bytes+1) << " bytes" << std::endl);
    _bp += 2;

    //Next ev_bytes bytes describe events
    for(unsigned i = 0; i < ev_bytes; i+=3) {
      unsigned ev_code = uint8_t(_rb[_bp+i]);
      if (_payload_sizes[ev_code] > 0) {
        if (ev_code >= Event::EV_PAYLOADS && ev_code <= Event::GAME_END) {
          FAIL_CORRUPT("Event " << Event::name[ev_code-Event::EV_PAYLOADS] << " payload size set multiple times");
        } else {
          FAIL_CORRUPT("Event " << hex(ev_code) << std::dec << " payload size set multiple times");
        }
        return false;
      }
      _payload_sizes[ev_code] = readBE2U(&_rb[_bp+i+1]);
      DOUT1("  Payload size for event "
        << hex(ev_code) << std::dec << ": " << _payload_sizes[ev_code]
        << " bytes" << std::endl);
    }

    //Sanity checks to verify we at least have Payload Sizes, Game Start, Pre Frame, Post Frame, and Game End Events
    for(unsigned i = Event::EV_PAYLOADS; i <= Event::GAME_END; ++i) {
      if (_payload_sizes[i] == 0) {
        FAIL_CORRUPT("Event " << Event::name[i-Event::EV_PAYLOADS] << " payload size not set");
        return false;
      }
    }

    //Update the remaining length of the raw data to sift through
    _bp += ev_bytes;
    _length_raw -= (2+ev_bytes);
    return true;
  }

  bool Compressor::_parseEvents() {
    DOUT1("Parsing events proper" << std::endl);

    bool success = true;
    for( ; _length_raw > 0; ) {
      unsigned ev_code = uint8_t(_rb[_bp]);
      unsigned shift   = _payload_sizes[ev_code]+1; //Add one byte for event code
      if (shift > _length_raw) {
        FAIL_CORRUPT("Event byte offset exceeds raw data length");
        return false;
      }
      switch(ev_code) { //Determine the event code
        case Event::GAME_START:  success = _parseGameStart(); break;
        case Event::PRE_FRAME:   success = _parsePreFrame();  break;
        case Event::POST_FRAME:  success = _parsePostFrame(); break;
        // case Event::GAME_END:    success = _parseGameEnd();   break;
        case Event::ITEM_UPDATE: success = _parseItemUpdate(); break;
        case Event::FRAME_START: success = _parseFrameStart(); break;
        case Event::BOOKEND:     success = _parseBookend(); break;
        default:
          DOUT1("  Warning: unknown event code " << hex(ev_code) << " encountered; skipping" << std::endl);
          break;
      }
      if (not success) {
        return false;
      }
      if (_payload_sizes[ev_code] == 0) {
        FAIL_CORRUPT("Uninitialized event " << hex(ev_code) << " encountered");
        return false;
      }
      _length_raw    -= shift;
      _bp            += shift;
      DOUT2("  Raw bytes remaining: " << +_length_raw << std::endl);
    }

    return true;
  }

  bool Compressor::_parseGameStart() {
    DOUT1("  Parsing game start event at byte " << +_bp << std::endl);

    if (_slippi_maj > 0) {
      FAIL_CORRUPT("Duplicate game start event");
      return false;
    }

    //Get Slippi version
    _slippi_maj = uint8_t(_rb[_bp+0x1]); //Major version
    _slippi_min = uint8_t(_rb[_bp+0x2]); //Minor version
    _slippi_rev = uint8_t(_rb[_bp+0x3]); //Build version (4th char unused)
    _is_encoded = uint8_t(_rb[_bp+0x4]); //Whether the file is encrypted

    _wb[_bp+0x4] ^= 1; //Flip encryption status for output

    if (_slippi_maj == 0) {
      FAIL("Replays from Slippi 0.x.x are not supported");
      return false;
    }

    std::stringstream ss;
    ss << +_slippi_maj << "." << +_slippi_min << "." << +_slippi_rev;
    _slippi_version = ss.str();
    DOUT1("    Slippi Version: " << _slippi_version << std::endl);

    //Get player info
    for(unsigned p = 0; p < 4; ++p) {
      unsigned i                     = 0x65 + 0x24*p;
      unsigned m                     = 0x141 + 0x8*p;
      unsigned k                     = 0x161 + 0x10*p;
      std::string ps                 = std::to_string(p+1);

      _replay.player[p].ext_char_id  = uint8_t(_rb[_bp+i]);
      if (_replay.player[p].ext_char_id >= CharExt::__LAST) {
        FAIL_CORRUPT("External character ID " << +_replay.player[p].ext_char_id << " is invalid");
        return false;
      }
      _replay.player[p].player_type  = uint8_t(_rb[_bp+i+0x1]);
      _replay.player[p].start_stocks = uint8_t(_rb[_bp+i+0x2]);
      _replay.player[p].color        = uint8_t(_rb[_bp+i+0x3]);
      _replay.player[p].team_id      = uint8_t(_rb[_bp+i+0x9]);
      _replay.player[p].cpu_level    = uint8_t(_rb[_bp+i+0xF]);
      _replay.player[p].dash_back    = readBE4U(&_rb[_bp+m]);
      _replay.player[p].shield_drop  = readBE4U(&_rb[_bp+m+0x4]);

      //Decode Melee's internal tag as a proper UTF-8 string
      if(_slippi_maj >= 2 || _slippi_min >= 3) {
        std::u16string tag;
        for(unsigned n = 0; n < 16; n+=2) {
          uint16_t b = (readBE2U(_rb+_bp+k+n));
          if ((b>>8) == 0x82) {
            if ( (b&0xff) < 0x59) { //Roman numerals
              b -= 0x821f;
            } else if ( (b&0xff) < 0x80) { //Roman letters
              b -= 0x81ff;
            } else { //Hiragana
              b -= 0x525e;
            }
          } else if ((b>>8) == 0x83) {
            if ( (b&0xff) < 0x80) { //Katakana, part 1
              b -= 0x529f;
            } else { //Katakana, part 2
              b -= 0x52a0;
            }
          } else if ((b>>8) == 0x81) {  //Miscellaneous characters
            switch(b&0xff) {
              case(0x81): b = 0x003D; break; // =
              case(0x5b): b = 0x30FC; break; // Hiragana Wave Dash (lol)
              case(0x7c): b = 0x002D; break; // -
              case(0x7b): b = 0x002B; break; // +
              case(0x49): b = 0x0021; break; // !
              case(0x48): b = 0x003F; break; // ?
              case(0x97): b = 0x0040; break; // @
              case(0x93): b = 0x0025; break; // %
              case(0x95): b = 0x0026; break; // &
              case(0x90): b = 0x0024; break; // $
              case(0x44): b = 0x002E; break; // .
              case(0x60): b = 0x301C; break; // Japanese tilde
              case(0x42): b = 0x3002; break; // Japanese period
              case(0x41): b = 0x3001; break; // Japanese comma
              case(0x40): b = 0x3000; break; // Space
              default:
                DOUT1("    Encountered unknown character in tag: " << b << std::endl);
                b = 0;
                break;
            }
          } else {
            DOUT1("    Encountered unknown character in tag: " << b << std::endl);
            b = 0;
          }
          tag += b;
        }
        _replay.player[p].tag_css = to_utf8(tag);
      }
    }

    //Write to replay data structure
    _replay.slippi_version = std::string(_slippi_version);
    // _replay.parser_version = PARSER_VERSION;
    _replay.game_start_raw = std::string(base64_encode(reinterpret_cast<const unsigned char *>(&_rb[_bp+0x5]),312));
    _replay.metadata       = "";
    _replay.sudden_death   = bool(_rb[_bp+0xB]);
    _replay.teams          = bool(_rb[_bp+0xD]);
    _replay.items          = int8_t(_rb[_bp+0x10]);
    _replay.stage          = readBE2U(&_rb[_bp+0x13]);
    bool is_stock_match    = (uint8_t(_rb[_bp+0x5]) & 0x02) > 0;
    _replay.timer          = is_stock_match ? readBE4U(&_rb[_bp+0x15])/60 : 0;
    if (_replay.stage >= Stage::__LAST) {
      FAIL_CORRUPT("Stage ID " << +_replay.stage << " is invalid");
      return false;
    }
    _replay.seed           = readBE4U(&_rb[_bp+0x13D]);
    _rng                   = _replay.seed;
    std::cout << "Starting RNG: " << _rng << std::endl;

    if(_slippi_maj >= 2 || _slippi_min >= 5) {
      _replay.pal            = bool(_rb[_bp+0x1A1]);
    }
    if(_slippi_maj >= 2) {
      _replay.frozen         = bool(_rb[_bp+0x1A2]);
    }

    // _max_frames = getMaxNumFrames();
    _replay.setFrames(_max_frames);
    DOUT1("    Estimated " << _max_frames << " gameplay frames (" << (_replay.frame_count) << " total frames)" << std::endl);
    return true;
  }

  bool Compressor::_parseItemUpdate() {
    //Encode frame number, predicting the last frame number
    int32_t frame = readBE4S(&_rb[_bp+0x1]);
    int32_t lastframe = laststartframe;
    if (_is_encoded) {                  //Decode
      int32_t frame_delta_pred = frame + (lastframe + 1);
      writeBE4S(frame_delta_pred,&_wb[_bp+0x1]);
      laststartframe = frame_delta_pred;
    } else {                            //Encode
      int32_t frame_delta_pred = frame - (lastframe + 1);
      writeBE4S(frame_delta_pred,&_wb[_bp+0x1]);
      laststartframe = frame;
    }

    //Get a storage slot for the item
    uint8_t slot = readBE4U(&_rb[_bp+0x22]) % 16;

    //XOR all of the remaining data for the item
    //(Everything but command byte, frame, and spawn id)
    for(unsigned i = 0x05; i < 0x22; ++i) {
      _wb[_bp+i] = _rb[_bp+i] ^ _x_item[slot][i];
      _x_item[slot][i] = _is_encoded ? _wb[_bp+i] : _rb[_bp+i];
    }
    for(unsigned i = 0x26; i < 0x2B; ++i) {
      _wb[_bp+i] = _rb[_bp+i] ^ _x_item[slot][i];
      _x_item[slot][i] = _is_encoded ? _wb[_bp+i] : _rb[_bp+i];
    }

    // std::cout << "ID: "     << readBE4U(&_rb[_bp+0x22]) << std::endl;
    // std::cout << "Expire: " << readBE4F(&_rb[_bp+0x1E]) << std::endl;

    if (_debug == 0) { return true; }

    return true;
  }

  bool Compressor::_parseFrameStart() {
    //Determine RNG storage from 2nd bit in frame
    int32_t frame = readBE4S(&_rb[_bp+0x1]);
    //If first and second bits are different, rng is stored raw
    uint8_t bit1 = (frame >> 31) & 0x01;
    uint8_t bit2 = (frame >> 30) & 0x01;
    uint8_t rng_is_raw = bit1 ^ bit2;
    if (rng_is_raw) {
      //Flip second bit back to what it should be
      frame ^= 0x40000000;
    }

    //Encode frame number, predicting the last frame number +1
    int32_t lastframe = laststartframe;
    if (_is_encoded) {                  //Decode
      int32_t frame_delta_pred = frame + (lastframe + 1);
      writeBE4S(frame_delta_pred,&_wb[_bp+0x1]);
      laststartframe = frame_delta_pred;
    } else {                            //Encode
      int32_t frame_delta_pred = frame - (lastframe + 1);
      writeBE4S(frame_delta_pred,&_wb[_bp+0x1]);
      laststartframe = frame;
    }

    //Get RNG seed and record number of rolls it takes to get to that point
    if (_slippi_maj >= 3 && _slippi_min >= 7) {  //New RNG
      if (_is_encoded) {  //Decode
        if (not rng_is_raw) { //Roll RNG a few times until we get to the desired value
          unsigned rolls = readBE4U(&_rb[_bp+0x7]);
          for(unsigned i = 0; i < rolls; ++i) {
            _rng = rollRNGNew(_rng);
          }
          writeBE4U(_rng,&_wb[_bp+0x7]);
        }
      } else {  //Encode
        uint32_t start_rng = _rng;
        unsigned rolls     = 0;
        unsigned target    = readBE4U(&_rb[_bp+0x7]);
        for(; (start_rng != target) && (rolls < 256); ++rolls) {
          start_rng = rollRNGNew(start_rng);
        }
        if (rolls < 256) {  //If we can encode RNG as < 256 rolls, do it
          writeBE4U(rolls,&_wb[_bp+0x7]);
          _rng = target;
        } else {  //Store the raw RNG value, flipping bits as necessary
          //Flip second bit of cached frame number to signal raw rng
          writeBE4S(
            readBE4S(&_wb[_bp+0x1])^0x40000000,&_wb[_bp+0x1]);
        }
      }
    } else { //Old RNG
      if (_is_encoded) {
        unsigned rolls = readBE4U(&_rb[_bp+0x7]);
        for(unsigned i = 0; i < rolls; ++i) {
          _rng = rollRNG(_rng);
        }
        writeBE4U(_rng,&_wb[_bp+0x7]);
      } else { //Roll RNG until we hit the target, write the # of rolls
        unsigned rolls, seed = readBE4U(&_rb[_bp+0x7]);
        for(rolls = 0;_rng != seed; ++rolls) {
          _rng = rollRNG(_rng);
        }
        writeBE4U(rolls,&_wb[_bp+0x7]);
      }
    }

    return true;
  }

  bool Compressor::_parseBookend() {
    //Encode actual frame number, predicting laststartframe
    int32_t frame     = readBE4S(&_rb[_bp+0x1]);
    if (_is_encoded) {                  //Decode
      int32_t frame_delta_pred = frame + laststartframe;
      writeBE4S(frame_delta_pred,&_wb[_bp+0x1]);
      laststartframe = frame_delta_pred;
    } else {                            //Encode
      int32_t frame_delta_pred = frame - laststartframe;
      writeBE4S(frame_delta_pred,&_wb[_bp+0x1]);
      laststartframe = frame;
    }

    if (_slippi_maj < 4 && _slippi_min < 6) {
      return true;
    }

    //Encode rollback frame number, predicting laststartframe
    int32_t framerb   = readBE4S(&_rb[_bp+0x5]);
    if (_is_encoded) {                  //Decode
      int32_t frame_delta_pred_rb = framerb + laststartframe;
      writeBE4S(frame_delta_pred_rb,&_wb[_bp+0x5]);
    } else {                            //Encode
      int32_t frame_delta_pred_rb = framerb - laststartframe;
      writeBE4S(frame_delta_pred_rb,&_wb[_bp+0x5]);
    }

    return true;
  }

  bool Compressor::_parsePreFrame() {
    DOUT2("  Compressing pre frame event at byte " << +_bp << std::endl);

    //First bit is now used for storing raw / delta RNG
    //  so we need to just get the last bit
    uint8_t is_follower = uint8_t(_rb[_bp+0x6]) & 0x01;

    //Get player index
    uint8_t p    = uint8_t(_rb[_bp+0x5])+4*uint8_t(_rb[_bp+0x6]); //Includes follower
    if (p > 7 || _replay.player[p].frame == nullptr) {
      FAIL_CORRUPT("Invalid player index " << +p);
      return false;
    }

    //Base copy of data into pre_frame event
    memcpy(&_last_pre_frame[p][0x1],&_rb[_bp+0x1],0x3C);

    //Encode frame number, predicting the last frame number +1
    int32_t frame     = readBE4S(&_rb[_bp+0x1]);
    int32_t lastframe = readBE4S(&_x_pre_frame[p][0x1]);
    if (_is_encoded) {                  //Decode
      int32_t frame_delta_pred = frame + (lastframe + 1);
      writeBE4S(frame_delta_pred,&_wb[_bp+0x1]);
      memcpy(&_x_pre_frame[p][0x1],&_wb[_bp+0x1],4);
    } else {                            //Encode
      int32_t frame_delta_pred = frame - (lastframe + 1);
      writeBE4S(frame_delta_pred,&_wb[_bp+0x1]);
      memcpy(&_x_pre_frame[p][0x1],&_rb[_bp+0x1],4);
    }

    //Get RNG seed and record number of rolls it takes to get to that point
    if (_slippi_maj >= 3 && _slippi_min >= 7) {  //New RNG
      if (_is_encoded) {  //Decode
        uint8_t rng_is_raw = uint8_t(_rb[_bp+0x6]) & 0x80;
        if (rng_is_raw) {
          _wb[_bp+0x6] = is_follower;  //Flip the follower bit to normal
        } else {  //Roll RNG a few times until we get to the desired value
          unsigned rolls = readBE4U(&_rb[_bp+0x7]);
          for(unsigned i = 0; i < rolls; ++i) {
            _rng = rollRNGNew(_rng);
          }
          writeBE4U(_rng,&_wb[_bp+0x7]);
        }
      } else {  //Encode
        uint32_t start_rng = _rng;
        unsigned rolls     = 0;
        unsigned target    = readBE4U(&_rb[_bp+0x7]);
        for(; (start_rng != target) && (rolls < 256); ++rolls) {
          start_rng = rollRNGNew(start_rng);
        }
        if (rolls < 256) {  //If we can encode RNG as < 256 rolls, do it
          writeBE4U(rolls,&_wb[_bp+0x7]);
          _rng = target;
          // std::cout << rolls << " rolls" << std::endl;
        } else {  //Store the raw RNG value, flipping bits as necessary
          //Flip first bit of port number to signal raw rng
          uint8_t raw_rng = uint8_t(_rb[_bp+0x6]) | 0x80;
          _wb[_bp+0x6]    = raw_rng;
          // std::cout << target << " raw" << std::endl;
        }
      }
    } else { //Old RNG
      if (_is_encoded) {
        unsigned rolls = readBE4U(&_rb[_bp+0x7]);
        for(unsigned i = 0; i < rolls; ++i) {
          _rng = rollRNG(_rng);
        }
        writeBE4U(_rng,&_wb[_bp+0x7]);
      } else { //Roll RNG until we hit the target, write the # of rolls
        unsigned rolls, seed  = readBE4U(&_rb[_bp+0x7]);
        for(rolls = 0;_rng != seed; ++rolls) {
          _rng = rollRNG(_rng);
        }
        writeBE4U(rolls,&_wb[_bp+0x7]);
      }
    }

    if (_debug == 0) { return true; }
    //Below here is still in testing mode

    return true;

    ////////////////////////////////////
    ////////////////////////////////////
    //Ineffective techniques beyond here
    ////////////////////////////////////
    ////////////////////////////////////

    // //Encode buttons using simple XORing
    // for(unsigned i = 0x31; i < 0x33; ++i) {
    //   _wb[_bp+i] = _rb[_bp+i] ^ _x_pre_frame[p][i];
    //   // std::cout << "Abs: " << hex(_rb[_bp+i]) << " Delta: " << hex(_wb[_bp+i]) << std::endl;
    //   if (_is_encoded) {                  //Decode
    //     _x_pre_frame[p][i] = _wb[_bp+i];
    //   } else {                            //Encode
    //     _x_pre_frame[p][i] = _rb[_bp+i];
    //   }
    // }

    // //Encode facing direction using ints instead of floats (less 1s)
    // //Improves xzip but not gzip
    // if (_is_encoded) {                  //Decode
    //   int fdir     = readBE4U(&_rb[_bp+0x15]);
    //   float face   = (fdir > 0) ? 1 : -1;
    //   writeBE4F(face,&_wb[_bp+0x15]);
    // } else {                            //Encode
    //   float face   = readBE4F(&_rb[_bp+0x15]);
    //   int32_t fdir = (face > 0) ? 1 : 0;
    //   writeBE4U(fdir,&_wb[_bp+0x15]);
    // }

    // //Encode trigger states using simple XORing
    // for(unsigned i = 0x33; i < 0x3B; ++i) {
    //   _wb[_bp+i] = _rb[_bp+i] ^ _x_pre_frame[p][i];
    //   // std::cout << "Abs: " << hex(_rb[_bp+i]) << " Delta: " << hex(_wb[_bp+i]) << std::endl;
    //   if (_is_encoded) {                  //Decode
    //     _x_pre_frame[p][i] = _wb[_bp+i];
    //   } else {                            //Encode
    //     _x_pre_frame[p][i] = _rb[_bp+i];
    //   }
    // }

    // //Encode joystick states using simple XORing
    // for(unsigned i = 0x19; i < 0x29; ++i) {
    //   _wb[_bp+i] = _rb[_bp+i] ^ _x_pre_frame[p][i];
    //   // std::cout << "Abs: " << hex(_rb[_bp+i]) << " Delta: " << hex(_wb[_bp+i]) << std::endl;
    //   if (_is_encoded) {                  //Decode
    //     _x_pre_frame[p][i] = _wb[_bp+i];
    //   } else {                            //Encode
    //     _x_pre_frame[p][i] = _rb[_bp+i];
    //   }
    // }

    // //Encode action states using simple XORing
    // for(unsigned i = 0x0B; i < 0x0D; ++i) {
    //   _wb[_bp+i] = _rb[_bp+i] ^ _x_pre_frame[p][i];
    //   // std::cout << "Abs: " << hex(_rb[_bp+i]) << " Delta: " << hex(_wb[_bp+i]) << std::endl;
    //   if (_is_encoded) {                  //Decode
    //     _x_pre_frame[p][i] = _wb[_bp+i];
    //   } else {                            //Encode
    //     _x_pre_frame[p][i] = _rb[_bp+i];
    //   }
    // }

    return false;

    //Old code beyond here

    // for(unsigned i = 0x01; i < 0x3C; ++i) {
    //   if (i >= 0x05 && i <= 0x06) { continue; }
    //   _wb[_bp+i] = _rb[_bp+i] ^ _x_pre_frame[p][i];
    //   if (_is_encoded) {                  //Decode
    //     _x_pre_frame[p][i] = _wb[_bp+i];
    //   } else {                            //Encode
    //     _x_pre_frame[p][i] = _rb[_bp+i];
    //   }
    // }
    // return true;

    // DOUT2("  Parsing pre frame event at byte " << +_bp << std::endl);
    // int32_t fnum = readBE4S(&_rb[_bp+0x1]);
    // int32_t f    = fnum-LOAD_FRAME;

    // if (fnum < LOAD_FRAME) {
    //   FAIL_CORRUPT("Frame index " << fnum << " less than " << +LOAD_FRAME);
    //   return false;
    // }
    // if (fnum >= _max_frames) {
    //   FAIL_CORRUPT("Frame index " << fnum
    //     << " greater than max frames computed from reported raw size (" << _max_frames << ")");
    //   return false;
    // }

    // _replay.last_frame                      = fnum;
    // _replay.frame_count                     = f+1; //Update the last frame we actually read
    // _replay.player[p].frame[f].frame        = fnum;
    // _replay.player[p].frame[f].player       = p%4;
    // _replay.player[p].frame[f].follower     = (p>3);
    // _replay.player[p].frame[f].alive        = 1;
    // _replay.player[p].frame[f].seed         = readBE4U(&_rb[_bp+0x7]);
    // _replay.player[p].frame[f].action_pre   = readBE2U(&_rb[_bp+0xB]);
    // _replay.player[p].frame[f].pos_x_pre    = readBE4F(&_rb[_bp+0xD]);
    // _replay.player[p].frame[f].pos_y_pre    = readBE4F(&_rb[_bp+0x11]);
    // _replay.player[p].frame[f].face_dir_pre = readBE4F(&_rb[_bp+0x15]);
    // _replay.player[p].frame[f].joy_x        = readBE4F(&_rb[_bp+0x19]);
    // _replay.player[p].frame[f].joy_y        = readBE4F(&_rb[_bp+0x1D]);
    // _replay.player[p].frame[f].c_x          = readBE4F(&_rb[_bp+0x21]);
    // _replay.player[p].frame[f].c_y          = readBE4F(&_rb[_bp+0x25]);
    // _replay.player[p].frame[f].trigger      = readBE4F(&_rb[_bp+0x29]);
    // _replay.player[p].frame[f].buttons      = readBE4U(&_rb[_bp+0x31]);
    // _replay.player[p].frame[f].phys_l       = readBE4F(&_rb[_bp+0x33]);
    // _replay.player[p].frame[f].phys_r       = readBE4F(&_rb[_bp+0x37]);

    // if(_slippi_maj >= 2 || _slippi_min >= 2) {
    //   _replay.player[p].frame[f].ucf_x        = uint8_t(_rb[_bp+0x3B]);
    //   if(_slippi_maj >= 2 || _slippi_min >= 4) {
    //     _replay.player[p].frame[f].percent_pre  = readBE4F(&_rb[_bp+0x3C]);
    //   }
    // }

    // return true;
  }

  bool Compressor::_parsePostFrame() {
    DOUT2("  Parsing post frame event at byte " << +_bp << std::endl);
    int32_t fnum = readBE4S(&_rb[_bp+0x1]);
    int32_t f    = fnum-LOAD_FRAME;

    //Get port and follower status from merged bytes
    //  (works regardless of encoding)
    uint8_t merged = uint8_t(_rb[_bp+0x5]);
    uint8_t port   = merged & 0x03;
    uint8_t follow = uint8_t(_rb[_bp+0x6])+ ((merged & 0x04) >> 2);
    uint8_t p    = port+4*follow; //Includes follower
    if (p > 7 || _replay.player[p].frame == nullptr) {
      FAIL_CORRUPT("Invalid player index " << +p);
      return false;
    }

    //Encode frame number, predicting the last frame number +1
    int32_t frame     = readBE4S(&_rb[_bp+0x1]);
    int32_t lastframe = readBE4S(&_x_post_frame[p][0x1]);
    if (_is_encoded) {                  //Decode
      int32_t frame_delta_pred = frame + (lastframe + 1);
      writeBE4S(frame_delta_pred,&_wb[_bp+0x1]);
      memcpy(&_x_post_frame[p][0x1],&_wb[_bp+0x1],4);
    } else {                            //Encode
      int32_t frame_delta_pred = frame - (lastframe + 1);
      writeBE4S(frame_delta_pred,&_wb[_bp+0x1]);
      memcpy(&_x_post_frame[p][0x1],&_rb[_bp+0x1],4);
    }

    //XOR encode damage
    for(unsigned i = 0x16; i < 0x1A; ++i) {
      _wb[_bp+i] = _rb[_bp+i] ^ _x_post_frame[p][i];
      _x_post_frame[p][i] = _is_encoded ? _wb[_bp+i] : _rb[_bp+i];
    }

    //XOR encode shields
    for(unsigned i = 0x1A; i < 0x1E; ++i) {
      _wb[_bp+i] = _rb[_bp+i] ^ _x_post_frame[p][i];
      _x_post_frame[p][i] = _is_encoded ? _wb[_bp+i] : _rb[_bp+i];
    }

    //Compress single byte values with XOR encoding
    for(unsigned i = 0x1E; i < 0x22; ++i) {
      _wb[_bp+i] = _rb[_bp+i] ^ _x_post_frame[p][i];
      _x_post_frame[p][i] = _is_encoded ? _wb[_bp+i] : _rb[_bp+i];
    }

    //XOR encode state bit flags
    for(unsigned i = 0x26; i < 0x2B; ++i) {
      _wb[_bp+i] = _rb[_bp+i] ^ _x_post_frame[p][i];
      _x_post_frame[p][i] = _is_encoded ? _wb[_bp+i] : _rb[_bp+i];
    }

    // float action = readBE4F(&_rb[_bp+0x22]);
    // std::cout << char('1'+p) << " action: " << action << std::endl;

    if (_debug == 0) { return true; }
    //Below here is still in testing mode

    // if(_slippi_maj > 3 || _slippi_min >= 5) {
    //   for(unsigned i = 0x35; i < 0x49; ++i) {
    //     _wb[_bp+i] = _rb[_bp+i] ^ _x_post_frame[p][i];
    //     _x_post_frame[p][i] = _is_encoded ? _wb[_bp+i] : _rb[_bp+i];
    //   }
    // }

    return true;

    ////////////////////////////////////
    ////////////////////////////////////
    //Ineffective techniques beyond here
    ////////////////////////////////////
    ////////////////////////////////////

    //Coalesce and delta encode bits from other values into port byte
      //0x5  - port     - 2-bits (can't delta encode)
      //0x6  - follower - 1-bit
      //0x12 - facing   - 1-bit
      //0x20 - hitby    - 3-bits
      //0x2f - grounded - 1-bit

    //Can't delta encode last two port bits
    // uint8_t lastmagic = _x_post_frame[p][0x5] & 0xFC;
    // if (_is_encoded) {
    //   uint8_t magic         = _rb[_bp+0x5] ^ lastmagic;
    //   _x_post_frame[p][0x5] = magic;

    //   _wb[_bp+0x5]    =  magic & 0x03;
    //   _wb[_bp+0x6]    = (magic & 0x04) >> 2;
    //   _wb[_bp+0x20]   = (magic & 0x70) >> 4;
    //   _wb[_bp+0x2f]   = (magic & 0x80) >> 7;

    //   float facing    = (magic & 0x08) ? 1.0f : -1.0f;
    //   writeBE4F(facing, &_wb[_bp+0x12]);
    // } else {
    //   uint8_t magic   = 0;
    //   uint8_t facing  = (readBE4F(&_rb[_bp+0x12]) > 0) ? 1 : 0;
    //   magic          ^= _rb[_bp+0x5];
    //   magic          ^= _rb[_bp+0x6] << 2;
    //   magic          ^=       facing << 3;
    //   magic          ^= _rb[_bp+0x20] << 4;
    //   magic          ^= _rb[_bp+0x2f] << 7;

    //   _wb[_bp+0x6]    = 0;
    //   _wb[_bp+0x12]   = 0;
    //   _wb[_bp+0x13]   = 0;
    //   _wb[_bp+0x14]   = 0;
    //   _wb[_bp+0x15]   = 0;
    //   _wb[_bp+0x20]   = 0;
    //   _wb[_bp+0x2f]   = 0;

    //   _wb[_bp+0x5]          = magic ^ lastmagic;
    //   _x_post_frame[p][0x5] = magic;
    // }

    //Predict post_frame action state as identical to pre_frame one
    // uint16_t alast = readBE2U(&_last_pre_frame[p][0xB]);
    // if (_is_encoded) {                  //Decode
    //   uint16_t pred = readBE2U(&_rb[_bp+0x8]);
    //   uint16_t action  = pred ^ alast;
    //   // std::cout << "Action Ret: " << action << std::endl;
    //   writeBE2U(action,&_wb[_bp+0x08]);
    //   _x_post_frame[p][0x08] = _wb[_bp+0x08];
    //   memcpy(&_x_post_frame[p][0x08],&_wb[_bp+0x08],2);
    // } else {                            //Encode
    //   uint16_t action = readBE2U(&_rb[_bp+0x8]);
    //   uint16_t pred  = alast ^ action;
    //   // std::cout << "Action Delta: " << pred << std::endl;
    //   writeBE2U(pred,&_wb[_bp+0x08]);
    //   memcpy(&_x_post_frame[p][0x08],&_rb[_bp+0x08],2);
    // }

    //Predict post_frame X as identical to pre_frame one
    // uint32_t alast = readBE4U(&_last_pre_frame[p][0xD]);
    // if (_is_encoded) {                  //Decode
    //   uint32_t pred = readBE4U(&_rb[_bp+0xA]);
    //   uint32_t action  = pred ^ alast;
    //   // std::cout << "Action Ret: " << action << std::endl;
    //   writeBE4U(action,&_wb[_bp+0x0A]);
    //   _x_post_frame[p][0x0A] = _wb[_bp+0x0A];
    //   memcpy(&_x_post_frame[p][0x0A],&_wb[_bp+0x0A],4);
    // } else {                            //Encode
    //   uint32_t action = readBE4U(&_rb[_bp+0xA]);
    //   uint32_t pred  = alast ^ action;
    //   // std::cout << "Action Delta: " << pred << std::endl;
    //   writeBE4U(pred,&_wb[_bp+0x0A]);
    //   memcpy(&_x_post_frame[p][0x0A],&_rb[_bp+0x0A],4);
    // }

    //Always predict last frame + 1 for action frame counter
    // float lastaction = readBE4F(&_x_post_frame[p][0x22]);
    // if (_is_encoded) {                  //Decode
    //   float pred = readBE4F(&_rb[_bp+0x22]);
    //   float action = pred + lastaction + 1.0f;
    //   writeBE4F(action,&_wb[_bp+0x22]);
    //   memcpy(&_x_post_frame[p][0x22],&_wb[_bp+0x22],4);
    // } else {                            //Encode
    //   float action = readBE4F(&_rb[_bp+0x22]);
    //   float pred   = (action - lastaction) - 1.0f;
    //   writeBE4F(pred,&_wb[_bp+0x22]);
    //   memcpy(&_x_post_frame[p][0x22],&_rb[_bp+0x22],4);
    // }

    // X-position delta encoding
    // int32_t lastx  = readBE4S(&_x_post_frame[p][0xA]);
    // // float lastx = 0;
    // if (_is_encoded) {                  //Decode
    //   int32_t xdelta = readBE4S(&_rb[_bp+0xA]);
    //   int32_t x      = xdelta + lastx;
    //   writeBE4S(xdelta,&_wb[_bp+0xA]);
    //   memcpy(&_x_post_frame[p][0xA],&_wb[_bp+0xA],4);
    // } else {                            //Encode
    //   int32_t x      = readBE4S(&_rb[_bp+0xA]);
    //   int32_t xdelta = x - lastx;
    //   writeBE4S(xdelta,&_wb[_bp+0xA]);
    //   memcpy(&_x_post_frame[p][0xA],&_rb[_bp+0xA],4);
    // }

    //Encode facing direction using ints instead of floats (less 1s)
    // if (_is_encoded) {                  //Decode
    //   int fdir     = readBE4U(&_rb[_bp+0x12]);
    //   float face   = (fdir > 0) ? 1 : -1;
    //   writeBE4F(face,&_wb[_bp+0x12]);
    // } else {                            //Encode
    //   float face   = readBE4F(&_rb[_bp+0x12]);
    //   int32_t fdir = (face > 0) ? 1 : 0;
    //   writeBE4U(fdir,&_wb[_bp+0x12]);
    // }

    return false;

    //Old code

    // if (fnum < LOAD_FRAME) {
    //   FAIL_CORRUPT("Frame index " << fnum << " less than " << +LOAD_FRAME);
    //   return false;
    // }
    // if (fnum >= _max_frames) {
    //   FAIL_CORRUPT("Frame index " << fnum << " greater than max frames computed from reported raw size ("
    //     << _max_frames << ")");
    //   return false;
    // }

    // _replay.player[p].frame[f].char_id       = uint8_t(_rb[_bp+0x7]);
    // if (_replay.player[p].frame[f].char_id >= CharInt::__LAST) {
    //     FAIL_CORRUPT("Internal characater ID " << +_replay.player[p].frame[f].char_id << " is invalid");
    //     return false;
    //   }
    // _replay.player[p].frame[f].action_post   = readBE2U(&_rb[_bp+0x8]);
    // _replay.player[p].frame[f].pos_x_post    = readBE4F(&_rb[_bp+0xA]);
    // _replay.player[p].frame[f].pos_y_post    = readBE4F(&_rb[_bp+0xE]);
    // _replay.player[p].frame[f].face_dir_post = readBE4F(&_rb[_bp+0x12]);
    // _replay.player[p].frame[f].percent_post  = readBE4F(&_rb[_bp+0x16]);
    // _replay.player[p].frame[f].shield        = readBE4F(&_rb[_bp+0x1A]);
    // _replay.player[p].frame[f].hit_with      = uint8_t(_rb[_bp+0x1E]);
    // _replay.player[p].frame[f].combo         = uint8_t(_rb[_bp+0x1F]);
    // _replay.player[p].frame[f].hurt_by       = uint8_t(_rb[_bp+0x20]);
    // _replay.player[p].frame[f].stocks        = uint8_t(_rb[_bp+0x21]);
    // _replay.player[p].frame[f].action_fc     = readBE4F(&_rb[_bp+0x22]);

    // if(_slippi_maj >= 2) {
    //   _replay.player[p].frame[f].flags_1       = uint8_t(_rb[_bp+0x26]);
    //   _replay.player[p].frame[f].flags_2       = uint8_t(_rb[_bp+0x27]);
    //   _replay.player[p].frame[f].flags_3       = uint8_t(_rb[_bp+0x28]);
    //   _replay.player[p].frame[f].flags_4       = uint8_t(_rb[_bp+0x29]);
    //   _replay.player[p].frame[f].flags_5       = uint8_t(_rb[_bp+0x2A]);
    //   _replay.player[p].frame[f].hitstun       = readBE4F(&_rb[_bp+0x2B]);
    //   _replay.player[p].frame[f].airborne      = bool(_rb[_bp+0x2F]);
    //   _replay.player[p].frame[f].ground_id     = readBE2U(&_rb[_bp+0x30]);
    //   _replay.player[p].frame[f].jumps         = uint8_t(_rb[_bp+0x32]);
    //   _replay.player[p].frame[f].l_cancel      = uint8_t(_rb[_bp+0x33]);
    // }

    return true;
  }

  bool Compressor::_parseGameEnd() {
    return true;
    DOUT1("  Parsing game end event at byte " << +_bp << std::endl);
    _replay.end_type       = uint8_t(_rb[_bp+0x1]);

    if(_slippi_maj >= 2) {
      _replay.lras           = int8_t(_rb[_bp+0x2]);
    }
    return true;
  }

  bool Compressor::_parseMetadata() {
    DOUT1("Parsing metadata" << std::endl);

    //Parse metadata from UBJSON as regular JSON
    std::stringstream ss;

    std::string indent  = " ";
    std::string key     = "";
    std::string val     = "";
    std::string keypath = "";  //Flattened representation of current JSON key
    bool        done    = false;
    bool        fail    = false;
    int32_t     n       = 0;

    std::regex comma_killer("(,)(\\s*\\})");

    uint8_t strlen = 0;
    for(unsigned i = 0; _bp+i < _file_size;) {
      //Get next key
      switch(_rb[_bp+i]) {
        case 0x55: //U -> Length upcoming
          if (_bp+i+1 >= _file_size) {
            fail = true; break;
          }
          strlen = _rb[_bp+i+1];
          if (strlen > 0) {
            if (_bp+i+1+strlen >= _file_size) {
              fail = true; break;
            }
            key.assign(&_rb[_bp+i+2],strlen);
            // std::cout << keypath << std::endl;
          } else {
            key = "";  //Not sure if empty strings are actually valid, but just in case
          }
          keypath += ","+key;
          if (key.compare("metadata") != 0) {
            ss << indent << "\"" << key << "\" : ";
          }
          i = i+2+strlen;
          break;
        case 0x7d: //} -> Object ending
          keypath = keypath.substr(0,keypath.find_last_of(","));
          indent = indent.substr(1);
          if (indent.length() == 0) {
            done = true;
            break;
          }
          ss << indent << "}," << std::endl;
          i = i+1;
          continue;
        default:
          std::cerr << "Warning: don't know what's happening; expected key" << std::endl;
          return false;
      }
      if (done) {
        break;
      } else if (fail || _bp+i >= _file_size) {
        std::cerr << "Warning: metadata shorter than expected" << std::endl;
        return false;
      }
      //Get next value
      switch(_rb[_bp+i]) {
        case 0x7b: //{ -> Object upcoming
          ss << "{" << std::endl;
          if (key.compare("metadata") != 0) {
            indent = indent+" ";
          }
          i = i+1;
          break;
        case 0x53: //S -> string upcoming
          if (_bp+i+2 >= _file_size) {
            fail = true; break;
          }
          ss << "\"";
          if (_rb[_bp+i+1] != 0x55) {  //If the string is not of length U
            std::cerr << "Warning: found a long string we can't parse yet:" << std::endl;
            std::cerr << "  " << ss.str() << std::endl;
            return false;
          }
          strlen = _rb[_bp+i+2];
          if (_bp+i+2+strlen >= _file_size) {
            fail = true; break;
          }
          val.assign(&_rb[_bp+i+3],strlen);
          ss << val << "\"," << std::endl;
          if (key.compare("startAt") == 0) {
            _replay.start_time = val;
          } else if (key.compare("playedOn") == 0) {
            _replay.played_on = val;
          } else if (key.compare("netplay") == 0) {
            size_t portpos = keypath.find("players,");
            if (portpos != std::string::npos) {
              int port = keypath.substr(portpos+8,1).c_str()[0] - '0';
              _replay.player[port].tag = val;
            }
          } else if (key.compare("code") == 0) {
            size_t portpos = keypath.find("players,");
            if (portpos != std::string::npos) {
              int port = keypath.substr(portpos+8,1).c_str()[0] - '0';
              _replay.player[port].tag_code = val;
            }
          }
          i = i+3+strlen;
          keypath = keypath.substr(0,keypath.find_last_of(","));
          break;
        case 0x6c: //l -> 32-bit signed int upcoming
          if (_bp+i+4 >= _file_size) {
            fail = true; break;
          }
          n = readBE4S(&_rb[_bp+i+1]);
          ss << std::dec << n << "," << std::endl;
          i = i+5;
          keypath = keypath.substr(0,keypath.find_last_of(","));
          break;
        default:
          std::cerr << "Warning: don't know what's happening; expected value" << std::endl;
          return false;
      }
      if (fail) {
        std::cerr << "Warning: metadata shorter than expected" << std::endl;
        return false;
      }
      continue;
    }

    std::string metadata = ss.str();
    metadata = metadata.substr(0,metadata.length()-2);  //Remove trailing comma
    std::string mjson;
    std::regex_replace(  //Get rid of extraneous commas in our otherwise valid JSON
      std::back_inserter(mjson),
      metadata.begin(),
      metadata.end(),
      comma_killer,
      "$2"
      );

    _replay.metadata = mjson;

    return true;
  }

  Analysis* Compressor::analyze() {
    Analyzer a(_debug);
    return a.analyze(_replay);
  }

  void Compressor::_cleanup() {
    _replay.cleanup();
  }

  std::string Compressor::asJson(bool delta) {
    return _replay.replayAsJson(delta);
  }

  void Compressor::save(const char* outfilename) {
    DOUT1("Saving encoded replay" << std::endl);
    std::ofstream ofile2;
    ofile2.open(outfilename, std::ios::binary | std::ios::out);
    ofile2.write(_wb,sizeof(char)*_file_size);
    ofile2.close();
    DOUT1("Saved to " << outfilename << "!" << std::endl);
  }


}
