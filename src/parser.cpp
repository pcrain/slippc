#include "parser.h"

namespace slip {

  Parser::Parser(bool debug) {
    _debug = debug;
    _rb    = new char[BUFFERMAXSIZE];
    _bp    = 0;

    if(_debug) {
        _sbuf  = std::cout.rdbuf();
    } else {
        _dnull = new std::ofstream;
        _dnull->open("/dev/null", std::ofstream::out | std::ofstream::app );
        _sbuf  = _dnull->rdbuf();
    }
    _dout  = new std::ostream(_sbuf);

  }

  Parser::~Parser() {
    if(not _debug) {
      _dnull->close();
      delete _dnull;
    }
    delete _rb;
    delete _dout;
  }

  bool Parser::load(const char* replayfilename) {
    (*_dout) << "Loading " << replayfilename << std::endl;
    std::ifstream myfile;
    myfile.open(replayfilename,std::ios::binary | std::ios::in);
    myfile.read(_rb,BUFFERMAXSIZE);
    myfile.close();
    return this->_parse();
  }

  bool Parser::_parse() {
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
      std::cerr << "Failed to parse metadata" << std::endl;
      return false;
    }
    (*_dout) << "Successfully parsed replay!" << std::endl;
    return true;
  }

  bool Parser::_parseHeader() {
    (*_dout) << "Parsing header" << std::endl;
    _bp = 0; //Start reading from byte 0

    //First 15 bytes contain header information
    if (same8(&_rb[_bp],SLP_HEADER)) {
      (*_dout) << "  Slippi Header Matched" << std::endl;
    } else {
      std::cerr << "  Slippi Header Did Not Match" << std::endl;
      return false;
    }
    _length_raw_start = readBE4U(&_rb[_bp+11]);
    if(_length_raw_start == 0) {
      std::cerr << "  0-byte raw data detected; aborting" << std::endl;
      return false;
    }
    (*_dout) << "  Raw portion = " << _length_raw_start << " bytes" << std::endl;
    _length_raw = _length_raw_start;
    _bp += 15;
    return true;
  }

  bool Parser::_parseEventDescriptions() {
    (*_dout) << "Parsing event descriptions" << std::endl;

    //Next 2 bytes should be 0x35
    if (_rb[_bp] != Event::EV_PAYLOADS) {
      std::cerr << "  Expected Event 0x" << std::hex
        << Event::EV_PAYLOADS << std::dec << " (Event Payloads)" << std::endl;
      return false;
    }
    uint8_t ev_bytes = _rb[_bp+1]-1; //Subtract 1 because the last byte we read counted as part of the payload
    _payload_sizes[Event::EV_PAYLOADS] = int32_t(ev_bytes+1);
    (*_dout) << "  Event description length = " << int32_t(ev_bytes+1) << " bytes" << std::endl;
    _bp += 2;

    //Next ev_bytes bytes describe events
    for(unsigned i = 0; i < ev_bytes; i+=3) {
      _payload_sizes[(unsigned)_rb[_bp+i]] = readBE2U(&_rb[_bp+i+1]);
      (*_dout) << "  Payload size for event "
        << hex(_rb[_bp+i]) << std::dec << ": " << _payload_sizes[(unsigned)_rb[_bp+i]]
        << " bytes" << std::endl;
    }

    //Update the remaining length of the raw data to sift through
    _bp += ev_bytes;
    _length_raw -= (2+ev_bytes);
    return true;
  }

  bool Parser::_parseEvents() {
    (*_dout) << "Parsing events proper" << std::endl;

    for( ; _length_raw > 0; ) {
      switch(_rb[_bp]) { //Determine the event code
        case Event::GAME_START: _parseGameStart(); break;
        case Event::PRE_FRAME:  _parsePreFrame();  break;
        case Event::POST_FRAME: _parsePostFrame(); break;
        case Event::GAME_END:   _parseGameEnd();   break;
        default:
          std::cerr << "  Warning: unknown event code " << hex(_rb[_bp]) << " encountered" << std::endl;
          return false;
      }
      unsigned shift  = _payload_sizes[(unsigned)_rb[_bp]]+1; //Add one byte for event code
      _length_raw    -= shift;
      _bp            += shift;
    }

    return true;
  }

  bool Parser::_parseGameStart() {
    // std::cout << "  Parsing start of game event" << std::endl;

    //Get Slippi version
    std::stringstream ss;
    ss
      << +uint8_t(_rb[_bp+0x1]) << "." //Major version
      << +uint8_t(_rb[_bp+0x2]) << "." //Minor version
      << +uint8_t(_rb[_bp+0x3]);       //Build version (4th char unused)
    _slippi_version = ss.str();

    //Get player info
    for(unsigned p = 0; p < 4; ++p) {
      unsigned i                     = 0x65 + 0x24*p;
      unsigned m                     = 0x141 + 0x8*p;
      unsigned k                     = 0x161 + 0x10*p;
      std::string ps                 = std::to_string(p+1);

      _replay.player[p].ext_char_id  = uint8_t(_rb[_bp+i]);
      _replay.player[p].player_type  = uint8_t(_rb[_bp+i+0x1]);
      _replay.player[p].start_stocks = uint8_t(_rb[_bp+i+0x2]);
      _replay.player[p].color        = uint8_t(_rb[_bp+i+0x3]);
      _replay.player[p].team_id      = uint8_t(_rb[_bp+i+0x9]);
      _replay.player[p].dash_back    = readBE4U(&_rb[_bp+m]);
      _replay.player[p].shield_drop  = readBE4U(&_rb[_bp+m+0x4]);

      std::string tag;
      for(unsigned n = 0; n < 16; n+=2) {
        tag += (readBE2U(_rb+_bp+k+n)+1); //TODO: not sure why we have to add 1 here
      }
      _replay.player[p].tag_css = tag;
    }

    //Write to replay data structure
    _replay.slippi_version = std::string(_slippi_version);
    _replay.parser_version = PARSER_VERSION;
    _replay.game_start_raw = std::string(base64_encode(reinterpret_cast<const unsigned char *>(&_rb[_bp+0x5]),312));
    _replay.metadata       = "";
    _replay.teams          = bool(_rb[_bp+0xD]);
    _replay.stage          = readBE2U(&_rb[_bp+0x13]);
    _replay.seed           = readBE4U(&_rb[_bp+0x13D]);
    _replay.pal            = bool(_rb[_bp+0x1A1]);
    _replay.frozen         = bool(_rb[_bp+0x1A2]);

    _replay.setFrames(getMaxNumFrames());
    (*_dout) << "  Estimated " << _replay.frame_count << " (+" << START_FRAMES << ") frames" << std::endl;
    return true;
  }

  bool Parser::_parsePreFrame() {
    // std::cout << "  Parsing pre frame event" << std::endl;
    int32_t fnum = readBE4S(&_rb[_bp+0x1]);
    int32_t f    = fnum+START_FRAMES;
    uint8_t p    = uint8_t(_rb[_bp+0x5])+4*uint8_t(_rb[_bp+0x6]); //Includes follower

    _replay.last_frame                      = fnum;
    _replay.frame_count                     = f+1; //Update the last frame we actually read
    _replay.player[p].frame[f].frame        = fnum;
    _replay.player[p].frame[f].player       = p%4;
    _replay.player[p].frame[f].follower     = (p>3);
    _replay.player[p].frame[f].alive        = 1;
    _replay.player[p].frame[f].seed         = readBE4U(&_rb[_bp+0x7]);
    _replay.player[p].frame[f].action_pre   = readBE2U(&_rb[_bp+0xB]);
    _replay.player[p].frame[f].pos_x_pre    = readBE4F(&_rb[_bp+0xD]);
    _replay.player[p].frame[f].pos_y_pre    = readBE4F(&_rb[_bp+0x11]);
    _replay.player[p].frame[f].face_dir_pre = readBE4F(&_rb[_bp+0x15]);
    _replay.player[p].frame[f].joy_x        = readBE4F(&_rb[_bp+0x19]);
    _replay.player[p].frame[f].joy_y        = readBE4F(&_rb[_bp+0x1D]);
    _replay.player[p].frame[f].c_x          = readBE4F(&_rb[_bp+0x21]);
    _replay.player[p].frame[f].c_y          = readBE4F(&_rb[_bp+0x25]);
    _replay.player[p].frame[f].trigger      = readBE4F(&_rb[_bp+0x29]);
    _replay.player[p].frame[f].buttons      = readBE4U(&_rb[_bp+0x31]);
    _replay.player[p].frame[f].phys_l       = readBE4F(&_rb[_bp+0x33]);
    _replay.player[p].frame[f].phys_r       = readBE4F(&_rb[_bp+0x37]);
    _replay.player[p].frame[f].ucf_x        = uint8_t(_rb[_bp+0x3B]);
    _replay.player[p].frame[f].percent_pre  = readBE4F(&_rb[_bp+0x3C]);

    return true;
  }

  bool Parser::_parsePostFrame() {
    // std::cout << "  Parsing post frame event" << std::endl;
    int32_t f = readBE4S(&_rb[_bp+0x1])+START_FRAMES;
    uint8_t p = uint8_t(_rb[_bp+0x5])+4*uint8_t(_rb[_bp+0x6]); //Includes follower

    _replay.player[p].frame[f].char_id       = uint8_t(_rb[_bp+0x7]);
    _replay.player[p].frame[f].action_post   = readBE2U(&_rb[_bp+0x8]);
    _replay.player[p].frame[f].pos_x_post    = readBE4F(&_rb[_bp+0xA]);
    _replay.player[p].frame[f].pos_y_post    = readBE4F(&_rb[_bp+0xE]);
    _replay.player[p].frame[f].face_dir_post = readBE4F(&_rb[_bp+0x12]);
    _replay.player[p].frame[f].percent_post  = readBE4F(&_rb[_bp+0x16]);
    _replay.player[p].frame[f].shield        = readBE4F(&_rb[_bp+0x1A]);
    _replay.player[p].frame[f].hit_with      = uint8_t(_rb[_bp+0x1E]);
    _replay.player[p].frame[f].combo         = uint8_t(_rb[_bp+0x1F]);
    _replay.player[p].frame[f].hurt_by       = uint8_t(_rb[_bp+0x20]);
    _replay.player[p].frame[f].stocks        = uint8_t(_rb[_bp+0x21]);
    _replay.player[p].frame[f].action_fc     = readBE4F(&_rb[_bp+0x22]);
    _replay.player[p].frame[f].flags_1       = uint8_t(_rb[_bp+0x26]);
    _replay.player[p].frame[f].flags_2       = uint8_t(_rb[_bp+0x27]);
    _replay.player[p].frame[f].flags_3       = uint8_t(_rb[_bp+0x28]);
    _replay.player[p].frame[f].flags_4       = uint8_t(_rb[_bp+0x29]);
    _replay.player[p].frame[f].flags_5       = uint8_t(_rb[_bp+0x2A]);
    _replay.player[p].frame[f].hitstun       = readBE4U(&_rb[_bp+0x2B]);
    _replay.player[p].frame[f].airborne      = bool(_rb[_bp+0x2F]);
    _replay.player[p].frame[f].ground_id     = readBE2U(&_rb[_bp+0x30]);
    _replay.player[p].frame[f].jumps         = uint8_t(_rb[_bp+0x32]);
    _replay.player[p].frame[f].l_cancel      = uint8_t(_rb[_bp+0x33]);

    return true;
  }

  bool Parser::_parseGameEnd() {
    // std::cout << "  Parsing game end event" << std::endl;
    _replay.end_type       = uint8_t(_rb[_bp+0x1]);
    _replay.lras           = int8_t(_rb[_bp+0x2]);
    return true;
  }

  bool Parser::_parseMetadata() {
    (*_dout) << "Parsing metadata" << std::endl;

    //Parse metadata from UBJSON as regular JSON
    std::stringstream ss;

    std::string indent = " ";
    std::string key = "", val = "";
    int32_t n;
    bool done = false;

    std::string keypath = "";  //Flattened representation of current JSON key

    std::regex comma_killer("(,)(\\s*})");

    uint8_t strlen = 0;
    for(unsigned i = 0;;) {
      //Get next key
      switch(_rb[_bp+i]) {
        case 0x55: //U -> Length upcoming
          strlen = _rb[_bp+i+1];
          key.assign(&_rb[_bp+i+2],strlen);
          keypath += ","+key;
          // std::cout << keypath << std::endl;
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
          ss << "\"";
          if (_rb[_bp+i+1] != 0x55) {  //If the string is not of length U
            std::cerr << "Warning: found a long string we can't parse yet:" << std::endl;
            std::cerr << "  " << ss.str() << std::endl;
            return false;
          }
          strlen = _rb[_bp+i+2];
          val.assign(&_rb[_bp+i+3],strlen);
          ss << val << "\"," << std::endl;
          if (key.compare("startAt") == 0) {
            // std::cout << "start time: " << val << std::endl;
            _replay.start_time = val;
          } else if (key.compare("playedOn") == 0) {
            // std::cout << "played on: " << val << std::endl;
            _replay.played_on = val;
          } else if (key.compare("netplay") == 0) {
            unsigned portpos = keypath.find("players,");
            if (portpos != std::string::npos) {
              int port = keypath.substr(portpos+8,1).c_str()[0] - '0';
              // std::cout << "Port " << port <<  " tag : " << val << std::endl;
              _replay.player[port].tag = val;
            }
          }
          i = i+3+strlen;
          keypath = keypath.substr(0,keypath.find_last_of(","));
          break;
        case 0x6c: //l -> 32-bit signed int upcoming
          n = readBE4S(&_rb[_bp+i+1]);
          ss << std::dec << n << "," << std::endl;
          i = i+5;
          keypath = keypath.substr(0,keypath.find_last_of(","));
          break;
        default:
          std::cerr << "Warning: don't know what's happening; expected value" << std::endl;
          return false;
      }
      continue;
    }

    std::string metadata = ss.str();
    metadata = metadata.substr(0,metadata.length()-2);  //Remove trailing comma
    std::string mjson;
    std::regex_replace(
      std::back_inserter(mjson),
      metadata.begin(),
      metadata.end(),
      comma_killer,
      "$2"
      );
    // std::cout << mjson << std::endl;

    _replay.metadata = mjson;

    return true;
  }

  Analysis* Parser::analyze() {
    Analyzer a(_dout);
    return a.analyze(_replay);
  }

  void Parser::cleanup() {
    _replay.cleanup();
  }

  void Parser::save(const char* outfilename,bool delta) {
    (*_dout) << "Saving JSON" << std::endl;
    std::ofstream ofile2;
    ofile2.open(outfilename);
    ofile2 << _replay.replayAsJson(delta) << std::endl;
    ofile2.close();
    (*_dout) << "Saved to " << outfilename << "!" << std::endl;
  }


}
