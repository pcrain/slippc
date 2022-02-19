#include "compressor.h"

namespace slip {

  Compressor::Compressor(int debug_level) {
    _debug = debug_level;
    _bp    = 0;
  }

  Compressor::~Compressor() {
    if (_rb != nullptr)               { delete[] _rb; }
    if (_wb != nullptr)               { delete[] _wb; }
    if (_outfilename != nullptr)      { delete   _outfilename; }
    if (_outgeckofilename != nullptr) { delete   _outgeckofilename; }
  }

  bool Compressor::loadFromFile(const char* replayfilename) {
    DOUT1("  Loading " << replayfilename);
    std::ifstream myfile;
    myfile.open(replayfilename,std::ios::binary | std::ios::in);
    if (myfile.fail()) {
      FAIL("    File " << replayfilename << " could not be opened or does not exist");
      return false;
    }

    myfile.seekg(0, myfile.end);
    _file_size = myfile.tellg();
    if (_file_size < MIN_REPLAY_LENGTH) {
      FAIL("    File " << replayfilename << " is too short to be a valid Slippi replay");
      return false;
    }
    myfile.seekg(0, myfile.beg);

    _rb = new char[_file_size];
    myfile.read(_rb,_file_size);
    myfile.close();

    // Check if we have a compressed stream
    bool is_compressed = same4(&_rb[0],LZMA_HEADER);
    if (is_compressed) {
      DOUT1("    File Size: " << +_file_size << ", compressed");
      // Decompress the read buffer
      std::string decomp = decompressWithLzma(_rb, _file_size);
      // Get the new file size
      _file_size    = decomp.size();
      // Delete the old read buffer
      delete [] _rb;
      // Reallocate it with more spce
      _rb = new char[_file_size];
      // Copy buffer from the decompressed string
      memcpy(_rb,decomp.c_str(),_file_size);
    } else {
      DOUT1("    File Size: " << +_file_size);
    }

    _infilename = replayfilename;

    _wb           = new char[_file_size];
    memcpy(_wb,_rb,sizeof(char)*_file_size);

    return this->_parse();
  }

  void Compressor::saveToFile(bool rawencode) {
    if (fileExists(*_outfilename)) {
      FAIL("  File " << *_outfilename << " exists, refusing to overwrite");
      return;
    }

    std::ofstream ofile;
    ofile.open(*_outfilename, std::ios::binary | std::ios::out);
    // If this is the unencoded version, compress it first
    if (!(_encode_ver || rawencode)) {
      // Compress the write buffer
      std::string comp = compressWithLzma(_wb, _file_size);
      DOUT1("  Compression Ratio = " << float(_file_size-comp.size())/_file_size);
      // Write compressed buffer to file
      ofile.write(comp.c_str(),sizeof(char)*comp.size());
    } else {
      // Write normal buffer to file
      ofile.write(_wb,sizeof(char)*_file_size);
    }

    ofile.close();

  }

  bool Compressor::setOutputFilename(const char* fname) {
    if (fileExists(fname)) {
      return false;
    }
    if (_outfilename != nullptr) {
      delete   _outfilename;
    }
    _outfilename = new std::string(fname);
    return true;
  }

  bool Compressor::setGeckoOutputFilename(const char* fname) {
    if (_outgeckofilename != nullptr) {
      delete   _outgeckofilename;
    }
    _outgeckofilename = new std::string(fname);
    _outgeckofilename->append(".dat");
    if (fileExists(*_outgeckofilename)) {
      return false;
    }
    return true;
  }

  unsigned Compressor::saveToBuff(char** buffer) {
    // buffer = new char[_file_size];
    *buffer = new char[_file_size];
    memcpy(*buffer,_wb,sizeof(char)*_file_size);
    return _file_size;
  }

  bool Compressor::loadFromBuff(char** buffer, unsigned size) {
    _file_size = size;
    _wb        = new char[_file_size];
    _rb        = new char[_file_size];
    memcpy(_wb,*buffer,sizeof(char)*_file_size);
    memcpy(_rb,*buffer,sizeof(char)*_file_size);
    return this->_parse();
  }

  bool Compressor::validate() {
    if (_encode_ver) {
      return true;
    }

    char *enc_buff = nullptr, *dec_buff = nullptr;
    unsigned size  = this->saveToBuff(&enc_buff);
    Compressor *d  = new slip::Compressor(_debug);
    d->loadFromBuff(&enc_buff,size);
    d->saveToBuff(&dec_buff);

    bool success = (memcmp(_rb,dec_buff,size) == 0);
    if (!success) {
      unsigned diff = 0;
      for(unsigned i = 0; i < size; ++i) {
        if(_rb[i] != dec_buff[i]) {
          if ((++diff) <= 500) {
            DOUT2("    Byte " << i << " differs! orig "
            << int(_rb[i]) << " != " << int(dec_buff[i]) << " new");
          }
        }
      }
      DOUT1("Differs in " << diff << "/" << size << " bytes");
    }

    delete[] dec_buff;
    delete[] enc_buff;
    delete d;

    return success;
  }

  bool Compressor::_parse() {
    _bp = 0; //Start reading from byte 0
    if (not this->_parseHeader()) {
      FAIL("  Failed to parse header");
      return false;
    }
    if (not this->_parseEventDescriptions()) {
      FAIL("  Failed to parse event descriptions");
      return false;
    }
    if (not this->_parseEvents()) {
      FAIL("  Failed to parse events proper");
      return false;
    }
    DOUT1("  Successfully parsed replay!");
    return true;
  }

  bool Compressor::_parseHeader() {
    DOUT1("  Parsing header");

    //First 15 bytes contain header information
    if (same8(&_rb[_bp],SLP_HEADER)) {
      DOUT1("    Slippi Header Matched");
    } else {
      FAIL_CORRUPT("    Header did not match expected Slippi file header");
      return false;
    }
    _length_raw_start = readBE4U(&_rb[_bp+11]);
    if(_length_raw_start == 0) {  //TODO: this is /technically/ recoverable
      FAIL_CORRUPT("    0-byte raw data detected");
      return false;
    }
    DOUT1("    Raw portion = " << _length_raw_start << " bytes");
    if (_length_raw_start > _file_size) {
      FAIL_CORRUPT("      Raw data size exceeds file size of " << _file_size << " bytes");
      return false;
    }
    _length_raw = _length_raw_start;
    _bp += 15;
    return true;
  }

  bool Compressor::_parseEventDescriptions() {
    DOUT1("  Parsing event descriptions");

    //Next 2 bytes should be 0x35
    if (_rb[_bp] != Event::EV_PAYLOADS) {
      FAIL_CORRUPT("    Expected Event 0x" << std::hex
        << Event::EV_PAYLOADS << std::dec << " (Event Payloads)");
      return false;
    }
    uint8_t ev_bytes = _rb[_bp+1]-1; //Subtract 1 because the last byte we read counted as part of the payload
    _payload_sizes[Event::EV_PAYLOADS] = int32_t(ev_bytes+1);
    DOUT1("    Event description length = " << int32_t(ev_bytes+1) << " bytes");
    _bp += 2;

    //Next ev_bytes bytes describe events
    for(unsigned i = 0; i < ev_bytes; i+=3) {
      unsigned ev_code = uint8_t(_rb[_bp+i]);
      if (_payload_sizes[ev_code] > 0) {
        if (ev_code >= Event::EV_PAYLOADS && ev_code <= Event::GAME_END) {
          FAIL_CORRUPT("    Event " << Event::name[ev_code-Event::EV_PAYLOADS] << " payload size set multiple times");
        } else {
          FAIL_CORRUPT("    Event " << hex(ev_code) << std::dec << " payload size set multiple times");
        }
        return false;
      }
      _payload_sizes[ev_code] = readBE2U(&_rb[_bp+i+1])+1; //Add one because of the event code itself
      DOUT1("    Payload size for event "
        << hex(ev_code) << std::dec << ": " << _payload_sizes[ev_code]
        << " bytes");
    }

    //Sanity checks to verify we at least have Payload Sizes, Game Start, Pre Frame, Post Frame, and Game End Events
    for(unsigned i = Event::EV_PAYLOADS; i <= Event::GAME_END; ++i) {
      if (_payload_sizes[i] == 0) {
        FAIL_CORRUPT("    Event " << Event::name[i-Event::EV_PAYLOADS] << " payload size not set");
        return false;
      }
    }

    //Update the remaining length of the raw data to sift through
    _bp         += ev_bytes;
    _length_raw -= (2+ev_bytes);
    return true;
  }

  bool Compressor::_parseEvents() {
    DOUT1("  Parsing events proper");

    bool success = true;
    for( ; _length_raw > 0; ) {
      unsigned ev_code = uint8_t(_rb[_bp]);

      unsigned shift   = _payload_sizes[ev_code];
      if (shift > _length_raw) {
        FAIL_CORRUPT("    Event byte offset exceeds raw data length");
        return false;
      }
      DOUT2("    EV code " << hex(ev_code) << " encountered");
      switch(ev_code) { //Determine the event code
        case Event::GAME_START:  success = _parseGameStart(); break;
        case Event::SPLIT_MSG:   success = _parseGeckoCodes(); break;
        case Event::FRAME_START: success = _parseFrameStart(); break;
        case Event::PRE_FRAME:   success = _parsePreFrame();   break;
        case Event::ITEM_UPDATE: success = _parseItemUpdate(); break;
        case Event::POST_FRAME:  success = _parsePostFrame();  break;
        case Event::BOOKEND:     success = _parseBookend();    break;
        case Event::GAME_END:
            _game_end_found = true;
            _game_loop_end = _bp;
            if (! _encode_ver) {
                _shuffleEvents();
            }
            success        = true;
            break;
        default:
          DOUT1("    Warning: unknown event code " << hex(ev_code) << " encountered; skipping");
          break;
      }
      if (not success) {
        return false;
      }
      if (_payload_sizes[ev_code] == 0) {
        FAIL_CORRUPT("    Uninitialized event " << hex(ev_code) << " encountered");
        return false;
      }
      _length_raw    -= shift;
      _bp            += shift;
      DOUT2("    Raw bytes remaining: " << +_length_raw);
    }
    if(!_game_end_found) {
      FAIL_CORRUPT("    No game end event found");
      return false;
    }
    return true;
  }

  bool Compressor::_parseGameStart() {
    DOUT1("  Parsing game start event at byte " << +_bp);

    //Get Slippi version
    _slippi_maj = uint8_t(_rb[_bp+O_SLP_MAJ]); //Major version
    _slippi_min = uint8_t(_rb[_bp+O_SLP_MIN]); //Minor version
    _slippi_rev = uint8_t(_rb[_bp+O_SLP_REV]); //Build version (4th char unused)
    _encode_ver = uint8_t(_rb[_bp+O_SLP_ENC]); //Whether the file is encoded
    if (MIN_VERSION(3,13,0)) {
      FAIL("    Version is " << GET_VERSION() << ". Replays from Slippi 3.13.0 and higher are not supported");
      return false;
    }

    //Set encoding status for output buffer
    if(_wb[_bp+O_SLP_ENC]) {
      _wb[_bp+O_SLP_ENC] = 0;  //Input is encoded, so we will decode
    } else {
      _wb[_bp+O_SLP_ENC] = COMPRETZ_VERSION;  //Input is not encoded, use current version
    }

    // ensure we have a good output filename based on encode status
    if (!ensureAppropriateFilename()) {
      return false;
    }

    //Print slippi version
    std::stringstream ss;
    ss << +_slippi_maj << "." << +_slippi_min << "." << +_slippi_rev;
    DOUT1("    Slippi Version: " << ss.str() << ", " << (_encode_ver ? "encoded" : "raw"));
    if(_encode_ver) {
      DOUT1("      Compressed with: slippc compression ver. " << int(_encode_ver));
    }

    //Get starting RNG state
    _rng       = readBE4U(&_rb[_bp+O_RNG_GAME_START]);
    _rng_start = _rng;
    DOUT1("    Starting RNG: " << _rng);

    return true;
  }

  bool Compressor::_parseGeckoCodes() {
    if(ENCODE_VERSION_MIN(2)) {
      // Debug output for actually dumping the gecko code messages to a file
      if(_outgeckofilename) {
        char* main_buf = (_encode_ver ? _wb : _rb);
        std::fstream fin;
        if(_message_count) {
          fin.open(*_outgeckofilename, std::ios::out | std::ios::app | std::ios::binary);
        } else {
          fin.open(*_outgeckofilename, std::ios::out | std::ios::trunc | std::ios::binary);
        }
        fin.write(&(main_buf[_bp]), MESSAGE_SIZE);
        fin.close();
        ++_message_count;
      }
      //Assuming everyone has the same default Gecko codes doesn't work, so V2 doesn't do anything fancy
      return true;
    }

    // V1 tried to XOR gecko codes but implemented it wrong anyway
    //   because it used a function-local variable to count messages,
    //   so it ended up XORing the same 512 bytes over and over again
    //   Must keep this here as a reminder of my shame ):

    // Load default Gecko codes
    const char* codes = readLegacyGeckoCodes();

    // XOR encode everything but the command byte
    for(unsigned i = 1; i < MESSAGE_SIZE; ++i) {
      _wb[_bp+i] = _rb[_bp+i] ^ codes[i];
    }

    return true;
  }

  bool Compressor::_parseItemUpdate() {
    //Encodings so far
      //0x00 - 0x00 | Command Byte         | No encoding
      //0x01 - 0x04 | Frame Number         | Predictive encoding (last frame + 1)
      //0x05 - 0x06 | Type ID              | XORed with last item-slot value
      //0x07 - 0x07 | State                | XORed with last item-slot value
      //0x08 - 0x0B | Facing Direction     | XORed with last item-slot value
      //0x0C - 0x0F | X Velocity           | Predicted from X position on this / last frame
      //0x10 - 0x13 | Y Velocity           | Predicted from Y position on this / last frame
      //0x14 - 0x17 | X Position           | Predictive encoding (2-frame delta)
      //0x18 - 0x1B | Y Position           | Predictive encoding (2-frame delta)
      //0x1C - 0x1D | Damage Taken         | XORed with last item-slot value
      //0x1E - 0x21 | Expiration Timer     | Predictive encoding (2-frame delta)
      //0x22 - 0x25 | Spawn ID             | No encoding
      //0x26 - 0x29 | Misc Values          | XORed with last item-slot value
      //0x2A - 0x2A | Owner                | XORed with last item-slot value

    DOUT2("  Compressing item event at byte " << +_bp);
    char* main_buf = (_encode_ver ? _wb : _rb);

    //Encode frame number, predicting the last frame number +1
    int cur_item_frame  = readBE4S(&_rb[_bp+O_FRAME]);
    int pred_item_frame = predictFrame(cur_item_frame, lastitemstartframe, &_wb[_bp+O_FRAME]);
    lastitemstartframe = _encode_ver ? pred_item_frame : cur_item_frame;
    DOUT2("    Item frame is " << +lastitemstartframe);

    if (_debug >= 3) {
        int fnum = readBE4S(&_rb[_bp+O_FRAME]);
        int inum = readBE4U(&_rb[_bp+O_ITEM_ID]);
        DOUT3("    EVID " << +fnum << ".3" << +inum);
    }

    //Get a storage slot for the item
    uint8_t slot = readBE4U(&_rb[_bp+O_ITEM_ID]) % ITEM_SLOTS;

    //XOR all of the remaining data for the item
    uint16_t itype;
    // TODO: not sure if I even need this split
    if (ENCODE_VERSION_MIN(2)) {
      // we use ITEM_TYPE for other purposes, so don't XOR it
      xorEncodeRange(O_ITEM_STATE,O_ITEM_XVEL,_x_item[slot]);
      // Ignore the bits used for storing defer information
      itype = decodeDeferInfo(readBE2U(&main_buf[_bp+O_ITEM_TYPE]));
    } else {
      xorEncodeRange(O_ITEM_TYPE,O_ITEM_XVEL,_x_item[slot]);
      itype = readBE2U(&main_buf[_bp+O_ITEM_TYPE]);
    }
    xorEncodeRange(O_ITEM_DAMAGE,O_ITEM_EXPIRE,_x_item[slot]);

    //Predict item positions based on velocity
    predictVelocItem(slot,O_ITEM_XPOS);

    //V2 and up of compressor predicts velocities y position based on item type
    if (ENCODE_VERSION_MIN(2)) {
      switch(itype) {
        case 0xd2:  //Shy Guy
          // Shy guys are jerks (yay puns)
          predictJoltItem(slot,O_ITEM_YPOS); break;
        default:  //Everything else just uses V1 behavior
          predictAccelItem(slot,O_ITEM_YPOS); break;
      }
    } else { // V1 of compressor always used predictAccelItem()
      predictAccelItem(slot,O_ITEM_YPOS);
    }

    // Use item positions to determine their velocties
    predictAsDifference(O_ITEM_XVEL,O_ITEM_XPOS,_x_item_p[slot]);
    predictAsDifference(O_ITEM_YVEL,O_ITEM_YPOS,_x_item_p[slot]);

    //Predict item expiration based on velocity
    predictVelocItem(slot,O_ITEM_EXPIRE);

    if(MIN_VERSION(3,2,0)) {
      xorEncodeRange(O_ITEM_MISC,O_ITEM_OWNER,_x_item[slot]);
      if (MIN_VERSION(3,6,0)) {
        xorEncodeRange(O_ITEM_OWNER,O_ITEM_END,_x_item[slot]);
      }
    }

    if (_debug == 0) { return true; }

    return true;
  }

  bool Compressor::_parseFrameStart() {
    //Encodings so far
      //0x00 - 0x00 | Command Byte         | No encoding
      //0x01 - 0x04 | Frame Number         | Predictive encoding (last frame + 1), second bit flipped if raw RNG
      //0x05 - 0x08 | RNG Seed             | Predictive encoding (# of RNG rolls, or raw)

    if (_game_loop_start == 0) {
        _game_loop_start = _bp;
        if (_encode_ver) {
          _unshuffleEvents();
        }
    }

    if (_debug >= 3) {
        int fnum = readBE4S(&_rb[_bp+O_FRAME]);
        DOUT3("    EVID " << +fnum << ".10");
    }

    //Encode frame number, predicting the last frame number +1
    int cur_frame  = readBE4S(&_rb[_bp+O_FRAME]);
    int pred_frame = predictFrame(cur_frame, laststartframe, &_wb[_bp+O_FRAME]);

    //Update laststartframe since we just crossed a new frame boundary
    laststartframe = _encode_ver ? pred_frame : cur_frame;

    //Predict RNG value from real (not delta encoded) frame number
    predictRNG(O_FRAME,O_RNG_FS);

    return true;
  }

  bool Compressor::_parseBookend() {
    //Encodings so far
      //0x00 - 0x00 | Command Byte         | No encoding
      //0x01 - 0x04 | Frame Number         | Predictive encoding (last frame + 1)
      //0x05 - 0x08 | Rollback Frame       | Predictive encoding (last frame + 1)

    if (_debug >= 3) {
      if (MIN_VERSION(3,7,0)) {
        int fnum = readBE4S(&_rb[_bp+O_FRAME]);
        int ffin = readBE4S(&_rb[_bp+O_ROLLBACK_FRAME]);
        DOUT3("    EVID " << +fnum << ".50 (" << ffin << ")");
      }
    }

    //Encode actual frame number, predicting and updating laststartframe
    predictFrame(readBE4S(&_rb[_bp+O_FRAME]), laststartframe, &_wb[_bp+O_FRAME]);
    if (MIN_VERSION(3,7,0)) {
      //Encode rollback frame number, predicting laststartframe without update
      predictFrame(readBE4S(&_rb[_bp+O_ROLLBACK_FRAME]), laststartframe, &_wb[_bp+O_ROLLBACK_FRAME]);
    }

    return true;
  }

  bool Compressor::_parsePreFrame() {
    //Encodings so far
      //0x00 - 0x00 | Command Byte         | No encoding
      //0x01 - 0x04 | Frame Number         | Predictive encoding (last frame + 1), second bit flipped if raw RNG
      //0x05 - 0x05 | Player Index         | No encoding (never changes)
      //0x06 - 0x06 | Is Follower          | No encoding (never changes)
      //0x07 - 0x0A | RNG Seed             | Predictive encoding (# of RNG rolls, or raw)
      //0x0B - 0x0C | Action State         | XORed with last post-frame value
      //0x0D - 0x10 | X Position           | XORed with last post-frame value
      //0x11 - 0x14 | Y Position           | XORed with last post-frame value
      //0x15 - 0x18 | Facing Direction     | XORed with last post-frame value
      //0x19 - 0x1C | Joystick X           | Value retrieved from float map
      //0x1D - 0x20 | Joystick Y           | Value retrieved from float map
      //0x21 - 0x24 | C-Stick X            | Value retrieved from float map
      //0x25 - 0x28 | C-Stick Y            | Value retrieved from float map
      //0x29 - 0x2C | Trigger              | Value retrieved from float map
      //0x2D - 0x30 | Processed Buttons    | No encoding (already sparse)
      //0x31 - 0x32 | Physical Buttons     | XORed with lower bits of Processed Buttons
      //0x33 - 0x36 | Physical L           | Value retrieved from float map
      //0x37 - 0x3A | Physical R           | Value retrieved from float map
      //0x3B - 0x3B | UCF X analog         | No encoding
      //0x3C - 0x3F | Damage               | No encoding (already sparse)

    // Old code for trying to predict Joy X from Joy Y and UCF X
    // if (!_encode_ver) {
    //   float   joyx  = readBE4F(&_rb[_bp+O_JOY_X]);
    //   float   joyy  = readBE4F(&_rb[_bp+O_JOY_Y]);
    //   int8_t  ucfx  = _rb[_bp+O_UCF_ANALOG];
    //   int8_t  intx  = float(joyx*80.0f);
    //   int8_t  inty  = float(joyy*80.0f);

    //   int8_t predx;
    //   if (inty > 27 || inty < -58) {
    //     predx = 0;
    //   } else if (ucfx < -22) {
    //     predx = ucfx;
    //   } else if (ucfx > 22) {
    //     predx = ucfx;
    //   } else {
    //     predx = 0;
    //   }

    //   if (predx > 79) {
    //     predx = 79;
    //   } else if (predx < -79) {
    //     predx = -79;
    //   }

    //   std::cout
    //     << " PRED "  << padString(predx,4)
    //     << " INT X " << padString(intx,4)
    //     << " UCF X " << padString(ucfx,4)
    //     << " INT Y " << padString(inty,4)
    //     << (((predx != 0) && (predx != intx)) ? "   MISS" : "")
    //     << std::endl;
    // }

    DOUT2("  Compressing pre frame event at byte " << +_bp);
    char* main_buf = (_encode_ver ? _wb : _rb);

    //Get player index
    uint8_t p = uint8_t(_rb[_bp+O_PLAYER])+4*uint8_t(_rb[_bp+O_FOLLOWER]); //Includes follower
    if (p > 7) {
      FAIL_CORRUPT("    Invalid player index " << +p);
      return false;
    }

    if (_debug >= 3) {
        int fnum = readBE4S(&_rb[_bp+O_FRAME]);
        DOUT3("    EVID " << +fnum << ".2" << +p);
    }

    //Encode frame number, predicting the last frame number +1
    int cur_frame  = readBE4S(&_rb[_bp+O_FRAME]);
    int pred_frame = predictFrame(cur_frame, lastpreframe[p], &_wb[_bp+O_FRAME]);
    //Update lastpreframe[p] since we just crossed a new frame boundary
    lastpreframe[p] = _encode_ver ? pred_frame : cur_frame;

    DOUT2("    Pre frame is " << +lastpreframe[p]);

    //Predict RNG value from real (not delta encoded) frame number
    predictRNG(O_FRAME,O_RNG_PRE);

    //Carry over action state from last post-frame
    writeBE2U(readBE2U(&_rb[_bp+O_ACTION_PRE]) ^ readBE2U(&_x_post_frame[p][O_ACTION_POST]),&_wb[_bp+O_ACTION_PRE]);
    memcpy(&_x_pre_frame[p][O_ACTION_PRE],&main_buf[_bp+O_ACTION_PRE],2);

    //Carry over x position from last post-frame
    writeBE4U(readBE4U(&_rb[_bp+O_XPOS_PRE]) ^ readBE4U(&_x_post_frame[p][O_XPOS_POST]),&_wb[_bp+O_XPOS_PRE]);
    memcpy(&_x_pre_frame[p][O_XPOS_PRE],&main_buf[_bp+O_XPOS_PRE],4);

    //Carry over y position from last post-frame
    writeBE4U(readBE4U(&_rb[_bp+O_YPOS_PRE]) ^ readBE4U(&_x_post_frame[p][O_YPOS_POST]),&_wb[_bp+O_YPOS_PRE]);
    memcpy(&_x_pre_frame[p][O_YPOS_PRE],&main_buf[_bp+O_YPOS_PRE],4);

    //Carry over facing direction from last post-frame
    writeBE4U(readBE4U(&_rb[_bp+O_FACING_PRE]) ^ readBE4U(&_x_post_frame[p][O_FACING_POST]),&_wb[_bp+O_FACING_PRE]);
    memcpy(&_x_pre_frame[p][O_FACING_PRE],&main_buf[_bp+O_FACING_PRE],4);

    // Encode analog stick and trigger values as integers
    encodeAnalog(O_JOY_X,   80.0f);
    encodeAnalog(O_JOY_Y,   80.0f);
    encodeAnalog(O_CX,      80.0f);
    encodeAnalog(O_CY,      80.0f);
    encodeAnalog(O_TRIGGER,140.0f);
    encodeAnalog(O_PHYS_L, 140.0f);
    encodeAnalog(O_PHYS_R, 140.0f);

    // Encode physical buttons from lower bits of processed buttons
    uint16_t phys_buttons = readBE2U(&main_buf[_bp+O_BUTTONS]);
    uint16_t proc_buttons = readBE2U(&main_buf[_bp+O_PROCESSED+2]);
    writeBE2U(proc_buttons^phys_buttons,&_wb[_bp+O_BUTTONS]);

    return true;
  }

  bool Compressor::_parsePostFrame() {
    //Encodings so far
      //0x00 - 0x00 | Command Byte         | No encoding
      //0x01 - 0x04 | Frame Number         | Predictive encoding (last frame + 1)
      //0x05 - 0x05 | Player Index         | No encoding (never changes)
      //0x06 - 0x06 | Is Follower          | No encoding (never changes)
      //0x07 - 0x07 | Char ID              | No encoding (already sparse)
      //0x08 - 0x09 | Action State         | No encoding, but used to predict pre-frame
      //0x0A - 0x0D | X Position           | Predictive encoding (3-frame accel)
      //0x0E - 0x11 | Y Position           | Predictive encoding (3-frame accel)
      //0x12 - 0x15 | Facing Direction     | No encoding, but used to predict pre-frame
      //0x16 - 0x19 | Damage               | No encoding, but used to predict pre-frame
      //0x1A - 0x1D | Shield               | Predictive encoding (2-frame delta)
      //0x1E - 0x1E | Last Hitting Attack  | XOR encoding
      //0x1F - 0x1F | Combo Count          | XOR encoding
      //0x20 - 0x20 | Last Hit By          | XOR encoding
      //0x21 - 0x21 | Stocks Remaining     | XOR encoding
      //0x22 - 0x25 | AS Frame Counter     | Predictive encoding (2-frame delta)
      //0x26 - 0x2A | State Bit Flags      | XOR encoding
      //0x2B - 0x2E | Hitstun Counter      | Predictive encoding (2-frame delta)
      //0x2F - 0x2F | Ground / Air State   | No encoding (already sparse)
      //0x30 - 0x31 | Ground ID            | No encoding (already sparse)
      //0x32 - 0x32 | Jumps Remaining      | No encoding (already sparse)
      //0x33 - 0x33 | L Cancel Status      | No encoding (already sparse)
      //0x34 - 0x34 | Hurtbox Status       | No encoding (already sparse)
      //0x35 - 0x38 | Self Air X-velocity  | Predictive encoding (2-frame delta)
      //0x39 - 0x3C | Self Y-velocity      | Predictive encoding (2-frame delta)
      //0x3D - 0x40 | Attack X-velocity    | Predictive encoding (2-frame delta)
      //0x41 - 0x44 | Attack Y-velocity    | Predictive encoding (2-frame delta)
      //0x45 - 0x48 | Self Ground X-vel.   | Predictive encoding (2-frame delta)

    DOUT2("  Compressing post frame event at byte " << +_bp);
    char* main_buf = (_encode_ver ? _wb : _rb);

    union { float f; uint32_t u; } float_true, float_pred, float_temp;
    int32_t fnum = readBE4S(&_rb[_bp+O_FRAME]);
    int32_t f    = fnum-LOAD_FRAME;

    //Get player index
    uint8_t p = uint8_t(_rb[_bp+O_PLAYER])+4*uint8_t(_rb[_bp+O_FOLLOWER]); //Includes follower
    if (p > 7) {
      FAIL_CORRUPT("    Invalid player index " << +p);
      return false;
    }

    if (_debug >= 3) {
        int fnum = readBE4S(&_rb[_bp+O_FRAME]);
        DOUT3("    EVID " << +fnum << ".4" << +p);
    }

    //Encode frame number, predicting the last frame number +1
    int cur_frame  = readBE4S(&_rb[_bp+O_FRAME]);
    int pred_frame = predictFrame(cur_frame, lastpostframe[p], &_wb[_bp+O_FRAME]);
    //Update lastpreframe[p] since we just crossed a new frame boundary
    lastpostframe[p] = _encode_ver ? pred_frame : cur_frame;

    DOUT2("    Post frame is " << +lastpostframe[p]);

    //Predict x position based on velocity and acceleration
    predictAccelPost(p,O_XPOS_POST);
    //Predict y position based on velocity and acceleration
    predictAccelPost(p,O_YPOS_POST);

    //Copy action state to post-frame
    memcpy(&_x_post_frame[p][O_ACTION_POST],&main_buf[_bp+O_ACTION_POST],2);
    //Copy x position to post-frame
    memcpy(&_x_post_frame[p][O_XPOS_POST],&main_buf[_bp+O_XPOS_POST],4);
    //Copy y position to post-frame
    memcpy(&_x_post_frame[p][O_YPOS_POST],&main_buf[_bp+O_YPOS_POST],4);
    //Copy facing direction to post-frame
    memcpy(&_x_post_frame[p][O_FACING_POST],&main_buf[_bp+O_FACING_POST],4);
    //Copy damage to post-frame
    memcpy(&_x_post_frame[p][O_DAMAGE_POST],&main_buf[_bp+O_DAMAGE_POST],4);

    //Predict shield decay as velocity
    predictVelocPost(p,O_SHIELD);

    //Compress single byte values with XOR encoding
    xorEncodeRange(O_LAST_HIT_ID,O_ACTION_FRAMES,_x_post_frame[p]);

    //Predict this frame's action state counter from the last 2 frames' counters
    if (MIN_VERSION(0,2,0)) {
      predictVelocPost(p,O_ACTION_FRAMES);
    }

    if (MIN_VERSION(2,0,0)) {
      //XOR encode state bit flags
      xorEncodeRange(O_STATE_BITS_1,O_HITSTUN,_x_post_frame[p]);

      //Predict this frame's hitstun counter from the last 2 frames' counters
      predictVelocPost(p,O_HITSTUN);

      if (MIN_VERSION(3,5,0)) {
        //Predict delta of various speeds based on previous frames' velocities
        predictVelocPost(p,O_SELF_AIR_Y);
        predictVelocPost(p,O_ATTACK_X);
        predictVelocPost(p,O_ATTACK_Y);
        predictVelocPost(p,O_SELF_GROUND_X);
        predictVelocPost(p,O_SELF_AIR_X);

        if (MIN_VERSION(3,8,0)) {
          //Predict this frame's hitlag counter from the last 2 frames' counters
          predictVelocPost(p,O_HITLAG);
        }
      }
    }

    if (_debug == 0) { return true; }
    //Below here is still in testing mode

    return true;
  }

  bool Compressor::_unshuffleEvents() {
    if (MAX_VERSION(3,0,0)) {
        return true;
    }
    bool success = _shuffleEvents(true);
    if (success) {
        // Copy the relevant portion of _rb to _wb
        memcpy(
          &_wb[_game_loop_start],
          &_rb[_game_loop_start],
          sizeof(char)*(_game_loop_end-_game_loop_start)
          );
    } else {
        FAIL("BIG PROBLEM UNSHUFFLING");
    }
    return success;
  }

  #define DOUNSHUFFLE(index,event) \
    off         = _payload_sizes[event]; \
    memcpy(&main_buf[b],&ev_buf[index][cpos[index]],off); \
    cpos[index] += off; \
    b           += off;

  #define DECODEFRAMEMATCHES(index,last) \
    (decodeFrame(readBE4S(&ev_buf[index][cpos[index]+O_FRAME]), last) == fnum)


  bool Compressor::_shuffleEvents(bool unshuffle) {
    const unsigned ETYPES = 64;  //Number of types of events
    const unsigned EMAX   = 30;  //Max event to include as part of the game loop

    // Can't do this for replays without frame start events yet
    if (MAX_VERSION(3,0,0)) {
        return true;
    }

    // Flag for if we fail anywhere
    bool success = true;

    // Determine if we're using the input or output buffer
    char* main_buf = unshuffle ? _rb : _wb;
    if (unshuffle) {
      // We don't actually know where _game_loop_end is yet
      _game_loop_end = _file_size;
      // We also need to unshuffle columns
      _unshuffleColumns(main_buf);
    }

    //Allocate space for storing shuffled events
    int max_events          = ALLOC_EVENTS;  //initial number of events to allocate space for
    unsigned offset[ETYPES] = {0};  //Size of individual event arrays
    unsigned* ev_counts     = new unsigned[ETYPES];
    unsigned* ev_max        = new unsigned[ETYPES];
    unsigned* ev_size       = new unsigned[ETYPES];
    ev_size[0]  = _payload_sizes[Event::FRAME_START];
    ev_size[9]  = _payload_sizes[Event::ITEM_UPDATE];
    ev_size[18] = _payload_sizes[Event::BOOKEND];
    ev_size[19] = _payload_sizes[Event::SPLIT_MSG];
    for (unsigned i = 0; i < 8; ++i) {
      ev_size[1+i]  = _payload_sizes[Event::PRE_FRAME];
      ev_size[10+i] = _payload_sizes[Event::POST_FRAME];
    }
    char** ev_buf           = new char*[ETYPES];
    for (unsigned i = 0; i < 20; ++i) {
      ev_counts[i] = 0;          //set event counter to 0
      ev_max[i]    = max_events; //initially allocate space for arrays
      ev_buf[i]    = new char[ev_max[i]*ev_size[i]];
    }
    int *frame_counter          = new int[max_events]{0};
    unsigned *finalized_counter = new unsigned[max_events]{0};
    unsigned start_fp           = 0;  //Frame pointer to next start frame
    unsigned end_fp             = 0;  //Frame pointer to next end frame

    //Initialize data for frame deferral counting
    char* defer_pre[8];
    char* defer_post[8];
    for (unsigned i = 0; i < 8; ++i) {
      defer_pre[i]  = new char[RB_SIZE]{0};
      defer_post[i] = new char[RB_SIZE]{0};
    }
    char* dupe_frames = new char[RB_SIZE]{0};
    int max_frame          = -125;
    unsigned max_item_seen = 0;
    int modframe           = 0;
    uint8_t defer          = 0;

    //Rearrange memory
    uint8_t pid; //Temporary variable for player id
    uint8_t oid; //Index into the offset array we're currently working with
    int cur_frame      = -125;
    unsigned oldshuffleframe = lastshuffleframe;
    for (unsigned b = _game_loop_start; b < _game_loop_end; ) {
      unsigned ev_code = uint8_t(main_buf[b]);
      unsigned shift   = _payload_sizes[ev_code];
      // std::cout << "ev code" << hex(ev_code) << " at byte " << b << "/" << _game_loop_end << std::endl;
      switch(ev_code) {
        case Event::FRAME_START:
            oid = 0;
            if (unshuffle) {
                cur_frame        = decodeFrame(readBE4S(&main_buf[b+O_FRAME]), lastshuffleframe);
                lastshuffleframe = cur_frame;
            } else {
                cur_frame        = readBE4S(&_rb[b+O_FRAME]);
            }

            // If we're shuffling, we need to keep track of deferred frames
            //   once we unshuffle events back into order
            if(!unshuffle) {
              // Determine the number of times each recent frame has been duplicated
              modframe = ((cur_frame+256)%RB_SIZE);
              if (cur_frame > max_frame) {
                max_frame = cur_frame;
                dupe_frames[modframe] = 0;
                // reset the defer counters for each player
                for (unsigned i = 0; i < 8; ++i) {
                  defer_pre[i][modframe]  = 0;
                  defer_post[i][modframe] = 0;
                }
              } else {
                dupe_frames[modframe] += 1;
              }
            }

            frame_counter[start_fp] = cur_frame;
            ++start_fp;

            // check if we've reached the event limit, and reallocate memory for more
            //   events if so
            if((int)start_fp == max_events) {
              DOUT1("    Maxed out frame count, resizing to " << max_events*2 << " events");
              // double the size of the buffer for storing frame count information
              int* newstartbuf = new int[max_events*2];
              // copy data from the old buffer to the new one
              memcpy(&newstartbuf[0],&frame_counter[0],max_events);
              // free the old frame counter buffer
              delete[] frame_counter;
              // set the buffer pointer to the new buffer
              frame_counter = newstartbuf;

              // double the size of the buffer for storing finalized frame information
              unsigned* newendbuf = new unsigned[max_events*2];
              // copy data from the old buffer to the new one
              memcpy(&newendbuf[0],&finalized_counter[0],max_events);
              // free the old finalized counter buffer
              delete[] finalized_counter;
              // set the buffer pointer to the new buffer
              finalized_counter = newendbuf;

              // actually properly double the size of the event buffer
              max_events *= 2;
            }
            // std::cout << "Started frame " << cur_frame << std::endl;
            break;
        case Event::PRE_FRAME: //Includes follower
            pid = uint8_t(main_buf[b+O_PLAYER])+4*uint8_t(main_buf[b+O_FOLLOWER]);
            oid = 1+pid;
            if(!unshuffle) {
              // check if we got rollback'd into existence
              defer = (dupe_frames[modframe] - defer_pre[pid][modframe]);
              if (defer > 0) {
                // encode the number of frames we need to defer writing
                //   this rollback'd preframe into the stream when decoding
                unsigned asid = readBE2U(&main_buf[b+O_ACTION_PRE]);
                asid = encodeDeferInfo(asid,defer);
                writeBE2U(asid,&main_buf[b+O_ACTION_PRE]);
              }
              // std::cout << "    set defer to " << +(dupe_frames[modframe]) << " - " << +(defer_pre[pid][modframe]) << "+ 1 = " << +(defer_pre[pid][modframe]) << std::endl;
              defer_pre[pid][modframe] = dupe_frames[modframe]+1;
            }
            break;
        case Event::ITEM_UPDATE:
            oid = 9;
            // XOR first byte of item id with last finalized frame
            if (! unshuffle) {
              unsigned item_id = readBE4U(&main_buf[b+O_ITEM_ID]);

              int ff = 0;
              if (MIN_VERSION(3,7,0)) {
                ff               = lookAheadToFinalizedFrame(&_rb[b]);

                // if we've seen a new item, check if it was rollbacked into existence
                if(item_id >= max_item_seen) {
                  max_item_seen = item_id+1;
                  if (dupe_frames[modframe] > 0) {
                    // encode the number of frames we need to defer writing
                    //   this rollback'd item into the stream when decoding
                    unsigned item_type = readBE2U(&main_buf[b+O_ITEM_TYPE]);
                    item_type = encodeDeferInfo(item_type,dupe_frames[modframe]);
                    writeBE2U(item_type,&main_buf[b+O_ITEM_TYPE]);
                  }
                }

                item_id          = encodeFrameIntoItemId(item_id,ff);
                unsigned dec_id  = encodeFrameIntoItemId(item_id,ff);
                if (encodeFrameIntoItemId(encodeFrameIntoItemId(dec_id,ff),ff) != dec_id) {
                  return false;
                }
                writeBE4U(item_id,&main_buf[b+O_ITEM_ID]);
              } else {
                ff = start_fp;
                int before = 0;
                // check last 128 frames for duplicates
                // start at 2 because frame_ptr is incremented
                for(unsigned fi = 2; fi < MAX_ROLLBACK; ++fi) {
                  if(frame_counter[start_fp-fi] == cur_frame) {
                    before += 1;
                  }
                }
                if (before > 0) {
                  // std::cout << "Item " << item_id << " has " << +before << " duplicate frames before frame " << cur_frame << std::endl;
                }
                item_id          = encodeFrameIntoItemId(item_id,before);
                unsigned dec_id  = encodeFrameIntoItemId(item_id,before);
                if (encodeFrameIntoItemId(encodeFrameIntoItemId(dec_id,ff),ff) != dec_id) {
                  return false;
                }
                writeBE4U(item_id,&main_buf[b+O_ITEM_ID]);
              }
              // std::cout << "ITEM ID = " << item_id << " (" << dec_id << ") FINAL " << ff << std::endl;
            }
            break;
        case Event::POST_FRAME: //Includes follower
            pid = uint8_t(main_buf[b+O_PLAYER])+4*uint8_t(main_buf[b+O_FOLLOWER]);
            oid = 10+pid;
            if(!unshuffle) {
              // check if we got rollback'd into existence
              defer = (dupe_frames[modframe] - defer_post[pid][modframe]);
              if (defer > 0) {
                // encode the number of frames we need to defer writing
                //   this rollback'd preframe into the stream when decoding
                unsigned asid = readBE2U(&main_buf[b+O_ACTION_POST]);
                asid = encodeDeferInfo(asid,defer);
                writeBE2U(asid,&main_buf[b+O_ACTION_POST]);
              }
              // std::cout << "    set defer to " << +(dupe_frames[modframe]) << " - " << +(defer_pre[pid][modframe]) << "+ 1 = " << +(defer_pre[pid][modframe]) << std::endl;
              defer_post[pid][modframe] = dupe_frames[modframe]+1;
            }
            break;
        case Event::BOOKEND:
            oid = 18;
            // Frame bookend rollback frames aren't defined before 3.7.0
            if (MIN_VERSION(3,7,0)) {
              int final_frame;
              if (unshuffle) {
                lastshuffleframe = frame_counter[end_fp];
                final_frame = decodeFrame(readBE4S(&main_buf[b+O_ROLLBACK_FRAME]),lastshuffleframe);
              } else {
                final_frame = readBE4S(&_rb[b+O_ROLLBACK_FRAME]);
              }
              finalized_counter[end_fp] = (final_frame+256) % 256;
              ++end_fp;
            } else {
              // TODO: MANUALLY COMPUTE FINALIZED FRAME
            }
            // std::cout << "Finalized frame " << cur_frame << std::endl;
            break;
        case Event::SPLIT_MSG: //Includes follower
            oid = EMAX+1; break;
        case Event::GAME_END:
            oid            = EMAX;
            _game_loop_end = b;
            // std::cout << "GAME LOOP END 0x"
            //     << std::hex << ev_code << std::dec
            //     << " at byte " << +b << std::endl;
            break;
        default:
            oid = ETYPES-1;
            // std::cout << "NOT GOOD 0x"
            //     << std::hex << ev_code << std::dec
            //     << " at byte " << +b << std::endl;
            break;
      }
      if (oid < EMAX) {
        // std::cout << "FINE 0x"
        //   << std::hex << ev_code << std::dec
        //   << " at byte " << +b << std::endl;
        memcpy(&ev_buf[oid][offset[oid]],&main_buf[b],sizeof(char)*shift);
        ++(ev_counts[oid]);
        if(ev_counts[oid] == ev_max[oid]) {
          DOUT1("    Maxed out events for " << +oid << ", resizing to " << ev_max[oid]*2 << " events");
          // double the size of the buffer for storing event information
          char* newbuf = new char[ev_max[oid]*ev_size[oid]*2];
          // copy data from the old buffer to the new one
          memcpy(&newbuf[0],&ev_buf[oid][0],ev_max[oid]*ev_size[oid]);
          // actually properly double the size of the event buffer
          ev_max[oid] *= 2;
          // free the old buffer
          delete[] ev_buf[oid];
          // set the buffer pointer to the new buffer
          ev_buf[oid] = newbuf;
        }
      }
      offset[oid] += shift;
      b           += shift;
    }
    lastshuffleframe = oldshuffleframe;

    //Sanity checks to make sure the number of individual event bytes
    //  sums to the total number of bytes in the main game loop
    unsigned tsize = 0;
    for(unsigned i = 0; i < EMAX; ++ i) {  //Skip game end event
        tsize += offset[i];
        // std::cout << "SIZE of " << i << " is " << offset[i] << std::endl;
    }
    if (tsize != (_game_loop_end-_game_loop_start)) {
        FAIL("    SOMETHING WENT HORRENDOUSLY WRONG D:");
        FAIL("      tsize=" << tsize);
        FAIL("      _game_loop_end=" << _game_loop_end);
        FAIL("      _game_loop_start=" << _game_loop_start);
        success = false;
    }

    // Copy memory over if we haven't failed horrendously yet
    if(success) {
        unsigned off = 0;
        unsigned b   = _game_loop_start;
        if (!unshuffle) { //Shuffle into main memory, excluding message event
            // Copy all events (except 19 == messages, makes things worse)
            for(unsigned i = 0; i <= 18; ++i) {
                memcpy(&main_buf[b],&ev_buf[i][0],offset[i]);
                b += offset[i];
            }
            // Shuffle columns
            _shuffleColumns(offset);
        } else { //Unshuffle into main memory, excluding message event
            unsigned cpos[EMAX] = {0};  //Buffer positions we're copying out of
            int* dec_frames = new int[_game_loop_end];
            for(unsigned frame_ptr = 0; b < _game_loop_end; ++frame_ptr) {
                // Sanity checks to make sure this is an actual frame start event
                if (cpos[0] >= offset[0]) {
                    FAIL("    THAT SUCKS D: at frame" << frame_ptr);
                    FAIL("      " << cpos[0] << " >= " << offset[0]);
                    success = false;
                    break;
                }
                if (ev_buf[0][cpos[0]] != Event::FRAME_START) {
                    FAIL("    OH SNAP D:");
                    success = false;
                    break;
                }

                // Get the current frame from the next frame start event
                int enc_frame             = readBE4S(&ev_buf[0][cpos[0]+O_FRAME]);
                int fnum                  = decodeFrame(enc_frame, lastshuffleframe);
                lastshuffleframe          = fnum;
                dec_frames[frame_ptr]     = fnum;

                // Determine the number of times each recent frame has been duplicated
                modframe = ((fnum+256)%RB_SIZE);
                if (fnum > max_frame) {
                  max_frame = fnum;
                  dupe_frames[modframe] = 0;
                } else {
                  dupe_frames[modframe] += 1;
                }

                // std::cout << "Decoded frame at " << frame_ptr << " as " << fnum << std::endl;
                DOUT3("    " << (b) << " bytes read, " << (_game_loop_end-b) << " bytes left at frame " << fnum);

                // TODO: this isn't working for Game_20210207T004448.slp
                if (fnum < -123 || fnum > (1 << 30)) {
                    FAIL("    SOMETHING WENT HORRENDOUSLY WRONG D:");
                    success = false;
                    break;
                }

                // Copy the frame start event over to the main buffer
                DOUNSHUFFLE(0,Event::FRAME_START);

                // Check if each player has a pre-frame this frame
                for(unsigned p = 0; p < 8; ++p) {
                    // True player order: 0,4,1,5,2,6,3,7
                    unsigned i = 1 + (4*(p%2)) + unsigned(p/2);
                    // If the player has no more pre-frame data, move on
                    if (cpos[i] >= offset[i]) {
                        continue;
                    }
                    // If the next frame isn't the one we're expecting, move on
                    if (!DECODEFRAMEMATCHES(i,lastshufflepreframe[p])) {
                        // std::cout << fnum << " NO MATCH pre-frame " << i+1 << " @ " << decodeFrame(readBE4S(&ev_buf[1+i][cpos[1+i]+O_FRAME]), lastshuffleframe) << std::endl;
                        continue;
                    }
                    // Verify we don't need to defer writing this preframe event
                    //   to the output stream due to rollback shenanigans
                    unsigned asid  = readBE2U(&ev_buf[i][cpos[i]+O_ACTION_PRE]);
                    unsigned defer = getDeferInfo(asid);
                    if (defer > 0) {
                      if ((int)defer != dupe_frames[modframe]) {
                        continue;
                      }
                      // zero out the first two bits of action state id holding defer information
                      writeBE2U(decodeDeferInfo(asid),&ev_buf[i][cpos[i]+O_ACTION_PRE]);
                    }
                    // Copy the pre frame event over to the main buffer
                    DOUNSHUFFLE(i,Event::PRE_FRAME);
                    lastshufflepreframe[p] = fnum;
                    // std::cout << fnum << " pre-frame " << i+1 << std::endl;
                }

                // Copy over any items with matching frames
                int last_id = -1;
                for(;;) {
                    // If there are no more items, move on
                    if (cpos[9] >= offset[9]) { break; }

                    // If the next item frame isn't the current frame, move on
                    if (!DECODEFRAMEMATCHES(9,lastitemshuffleframe)) {
                      break;
                    }

                    // Double check we're on the correctly finalized frame
                    unsigned item_id = readBE4U(&ev_buf[9][cpos[9]+O_ITEM_ID]);

                    if(MIN_VERSION(3,7,0)) {
                      unsigned ff = finalized_counter[frame_ptr];
                      // Decode item_id encoded with frame number
                      if (getFrameModFromItemId(item_id) != ff) {
                        break;
                      }
                      item_id  = encodeFrameIntoItemId(item_id,ff);
                    } else { // 3.6.x does not have a frame bookend rollback frame
                      unsigned dupes = 0;
                      // check last 128 frames for duplicates
                      // start at 1 because frame_ptr is not incremented
                      for(unsigned fi = 1; fi < MAX_ROLLBACK; ++fi) {
                        if (frame_ptr<fi) {
                          break;
                        }
                        if(dec_frames[frame_ptr-fi] == fnum) {
                          dupes += 1;
                        }
                      }

                      // verify item appeared on this duplicate frame
                      if (getFrameModFromItemId(item_id) != dupes) {
                        break;
                      }

                      item_id = encodeFrameIntoItemId(item_id,dupes);
                    }

                    // Verify we aren't repeating items this frame
                    if (int(item_id) <= last_id) { break; }

                    // Verify we don't need to defer writing this item event
                    //   to the output stream due to rollback shenanigans
                    unsigned item_type = readBE2U(&ev_buf[9][cpos[9]+O_ITEM_TYPE]);
                    unsigned defer     = getDeferInfo(item_type);
                    if (defer > 0) {
                      if ((int)defer != dupe_frames[modframe]) {
                        break;
                      }
                      // zero out the first two bits of item type holding defer information
                      writeBE2U(decodeDeferInfo(item_type),&ev_buf[9][cpos[9]+O_ITEM_TYPE]);
                    }

                    // Restore the actual item ID to the buffer
                    writeBE4U(item_id,&ev_buf[9][cpos[9]+O_ITEM_ID]);

                    // Update the last seen item id
                    last_id = item_id;

                    // Update lastitemshuffleframe
                    lastitemshuffleframe = fnum;

                    // Copy the item event over to the main buffer
                    DOUNSHUFFLE(9,Event::ITEM_UPDATE);
                }

                // Check if each player has a post-frame this frame
                for(unsigned p = 0; p < 8; ++p) {
                    // True player order: 0,4,1,5,2,6,3,7
                    unsigned i = 10 + (4*(p%2)) + unsigned(p/2);
                    //If the player has no more post-frame data, move on
                    if (cpos[i] >= offset[i]) {
                        continue;
                    }
                    // If the next frame isn't the one we're expecting, move on
                    if (!DECODEFRAMEMATCHES(i,lastshufflepostframe[p])) {
                        continue;
                    }
                    // Verify we don't need to defer writing this postframe event
                    //   to the output stream due to rollback shenanigans
                    unsigned asid  = readBE2U(&ev_buf[i][cpos[i]+O_ACTION_POST]);
                    unsigned defer = getDeferInfo(asid);
                    if (defer > 0) {
                      if ((int)defer != dupe_frames[modframe]) {
                        continue;
                      }
                      // zero out the first two bits of action state id holding defer information
                      writeBE2U(decodeDeferInfo(asid),&ev_buf[i][cpos[i]+O_ACTION_POST]);
                    }
                    // Copy the post frame event over to the main buffer
                    DOUNSHUFFLE(i,Event::POST_FRAME);
                    lastshufflepostframe[p] = fnum;
                    // std::cout << fnum << " post-frame " << i+1 << std::endl;
                }

                // Copy the frame end event over to the main buffer
                if (DECODEFRAMEMATCHES(18,lastshuffleframe)) {
                    DOUNSHUFFLE(18,Event::BOOKEND);
                }
            }
            delete[] dec_frames;
        }
    }

    //Free memory

    delete[] ev_counts;
    delete[] ev_max;
    delete[] ev_size;

    for (unsigned i = 0; i < 8; ++i) {
      delete[] defer_pre[i];
      delete[] defer_post[i];
    }
    delete[] dupe_frames;

    for (unsigned i = 0; i < 20; ++i) {
        delete[] ev_buf[i];
    }
    delete[] ev_buf;
    delete[] frame_counter;
    delete[] finalized_counter;

    return success;
  }

}
