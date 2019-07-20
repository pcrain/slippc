#include "parser.h"

namespace slip {

  Parser::Parser() {
    _rb = new char[BUFFERMAXSIZE];
    _bp = 0;
  }

  Parser::Parser(const char* replayfilename) {
    _rb = new char[BUFFERMAXSIZE];
    _bp = 0;
    this->load(replayfilename);
  }

  Parser::~Parser() {
    delete _rb;
  }

  void Parser::load(const char* replayfilename) {
    std::cout << "Loading " << replayfilename << std::endl;
    std::ifstream myfile;
    myfile.open(replayfilename,std::ios::binary | std::ios::in);
    myfile.read(_rb,BUFFERMAXSIZE);
    myfile.close();
    this->_parse();
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
    std::cout << "Successfully parsed replay!";
    return true;
  }

  bool Parser::_parseHeader() {
    std::cout << "Parsing header" << std::endl;
    printBytes(_rb,8);
    _bp = 0; //Start reading from byte 0
    //First 15 bytes contain header information
    if (same8(&_rb[_bp],SLP_HEADER)) {
      std::cout << "  Slippi Header Matched" << std::endl;
    } else {
      std::cerr << "  Slippi Header Did Not Match" << std::endl;
      return false;
    }
    _length_raw = readBE4U(&_rb[_bp+11]);
    std::cout << "  Raw portion = " << _length_raw << " bytes" << std::endl;
    _bp += 15;
    return true;
  }

  bool Parser::_parseEventDescriptions() {
    std::cout << "Parsing event descriptions" << std::endl;

    //Next 2 bytes should be 0x35
    if (_rb[_bp] != Event::EV_PAYLOADS) {
      std::cerr << "  Expected Event 0x" << std::hex
        << Event::EV_PAYLOADS << std::dec << " (Event Payloads)" << std::endl;
      return false;
    }
    uint8_t ev_bytes = _rb[_bp+1]-1; //Subtract 1 because the last byte we read counted as part of the payload
    std::cout << "  Event description length = " << int(ev_bytes+1) << " bytes" << std::endl;
    _bp += 2;

    //Next ev_bytes bytes describe events
    for(unsigned i = 0; i < ev_bytes; i+=3) {
      _payload_sizes[(unsigned)_rb[_bp+i]] = readBE2U(&_rb[_bp+i+1]);
      std::cout << "  Payload size for event "
        << hex(_rb[_bp+i]) << std::dec << ": " << _payload_sizes[(unsigned)_rb[_bp+i]]
        << " bytes" << std::endl;
    }

    //Update the remaining length of the raw data to sift through
    _bp += ev_bytes;
    _length_raw -= (2+ev_bytes);
    return true;
  }

  bool Parser::_parseEvents() {
    std::cout << "Parsing events proper" << std::endl;
    _jout["events"] = Json::arrayValue;

    for( ; _length_raw > 0; ) {
      switch(_rb[_bp]) { //Determine the event code
        case Event::GAME_START: _parseGameStart(); break;
        case Event::PRE_FRAME:  _parsePreFrame();  break;
        case Event::POST_FRAME: _parsePostFrame(); break;
        case Event::GAME_END:   _parseGameEnd();   break;
        default:
          std::cerr << "  Warning: unknown event code " << hex(_rb[_bp]) << " encountered; refusing to parse move event descriptions" << std::endl;
          return true;
      }
      unsigned shift  = _payload_sizes[(unsigned)_rb[_bp]]+1; //Add one byte for event code
      _length_raw    -= shift;
      _bp            += shift;
    }

    return true;
  }

  bool Parser::_parseGameStart() {
    // std::cout << "  Parsing start of game event" << std::endl;
    Json::Value j = Json::objectValue;
    j["_event"] = "game-start";

    //Get Slippi version
    std::stringstream ss;
    ss
      << +uint8_t(_rb[_bp+0x1]) << "." //Major version
      << +uint8_t(_rb[_bp+0x2]) << "." //Minor version
      << +uint8_t(_rb[_bp+0x3]);       //Build version (_rb[3] unused)
    _slippi_version = ss.str();
    j["_slippi-version"] = _slippi_version;
    j["~raw-gamestart"] = base64_encode(reinterpret_cast<const unsigned char *>(&_rb[_bp+0x5]),312);
    // std::cout << "    Slippi version = " << j["slippi-version"] << std::endl;
    j["is-teams"] = bool(_rb[_bp+0xD]);
    // std::cout << "    Teams = " << j["is-teams"] << std::endl;
    j["stage"] = readBE2U(&_rb[_bp+0x13]);
    // std::cout << "    Stage = " << j["stage"] << std::endl;
    for(unsigned p = 0; p < 4; ++p) {
      unsigned i = 0x65 + 0x24*p;
      j["p"+std::to_string(p+1)+"-char-id"] = _rb[_bp+i];
      // std::cout << "    P" << (p+1) << " charid = " << +_rb[i] << std::endl;
      j["p"+std::to_string(p+1)+"-char-type"] = _rb[_bp+i+0x1];
      // std::cout << "    P" << (p+1) << " chartype = " << +_rb[i+0x1] << std::endl;
      j["p"+std::to_string(p+1)+"-start-stocks"] = _rb[_bp+i+0x2];
      // std::cout << "    P" << (p+1) << " startstocks = " << +_rb[i+0x2] << std::endl;
      j["p"+std::to_string(p+1)+"-color"] = _rb[_bp+i+0x3];
      // std::cout << "    P" << (p+1) << " color = " << +_rb[i+0x3] << std::endl;
      j["p"+std::to_string(p+1)+"-team-id"] = _rb[_bp+i+0x9];
      // std::cout << "    P" << (p+1) << " teamid = " << +_rb[i+0x9] << std::endl;

      unsigned m = 0x141 + 0x8*p;
      j["p"+std::to_string(p+1)+"-dash-back"] = readBE4U(&_rb[_bp+m]);
      // std::cout << "    P" << (p+1) << " dashback = " << readBE4U(&_rb[j]) << std::endl;
      j["p"+std::to_string(p+1)+"-shield-drop"] = readBE4U(&_rb[_bp+m+0x4]);
      // std::cout << "    P" << (p+1) << " shielddrop = " << readBE4U(&_rb[j+0x4]) << std::endl;

      //TODO: not entirely sure if this section is working
      unsigned k = 0x161 + 0x10*p;
      std::string tag;
      for(unsigned n = 0; n < 16; n+=2) {
        tag += readBE2U(_rb+_bp+k+n);
      }
      j["p"+std::to_string(p+1)+"-tag"] = tag;
      // std::cout << "    P" << (p+1) << " tag = " << tag << std::endl;
    }

    j["random-seed"] = readBE4U(&_rb[_bp+0x13D]);
    // std::cout << "    Random Seed = " << readBE4U(&_rb[0x13D]) << std::endl;
    j["is-pal"] = bool(_rb[_bp+0x1A1]);
    // std::cout << "    PAL = " << bool(_rb[0x1A1]) << std::endl;
    j["is-frozen-stadium"] = bool(_rb[_bp+0x1A2]);
    // std::cout << "    Frozen PS = " << bool(_rb[0x1A2]) << std::endl;

    _jout["events"].append(j);

    return true;
  }

  bool Parser::_parsePreFrame() {
    // std::cout << "  Parsing pre frame event" << std::endl;
    Json::Value j = Json::objectValue;
    j["_event"] = "pre-frame";

    // std::cout << "    Frame number = "     << readBE4S(&_rb[0x1])  << std::endl;
    // std::cout << "    Player index = "     << +_rb[0x5]            << std::endl;
    // std::cout << "    Is follower = "      << +_rb[0x6]            << std::endl;
    // std::cout << "    Random Seed = "      << readBE4U(&_rb[0x7])  << std::endl;
    // std::cout << "    Action State = "     << readBE2U(&_rb[0xB])  << std::endl;
    // std::cout << "    X Position = "       << readBE4F(&_rb[0xD])  << std::endl;
    // std::cout << "    Y Position = "       << readBE4F(&_rb[0x11]) << std::endl;
    // std::cout << "    Facing Direction = " << readBE4F(&_rb[0x15]) << std::endl;
    // std::cout << "    Joystick X = "       << readBE4F(&_rb[0x19]) << std::endl;
    // std::cout << "    Joystick Y = "       << readBE4F(&_rb[0x1D]) << std::endl;
    // std::cout << "    Cstick X = "         << readBE4F(&_rb[0x21]) << std::endl;
    // std::cout << "    Cstick Y = "         << readBE4F(&_rb[0x25]) << std::endl;
    // std::cout << "    Trigger = "          << readBE4F(&_rb[0x29]) << std::endl;
    // std::cout << "    Phys. Buttons = "    << readBE4U(&_rb[0x31]) << std::endl;
    // std::cout << "    Phys. L Trigger = "  << readBE4F(&_rb[0x33]) << std::endl;
    // std::cout << "    Phys. R Trigger = "  << readBE4F(&_rb[0x37]) << std::endl;
    // std::cout << "    X Analog UCF = "     << +_rb[0x3B]           << std::endl;
    // std::cout << "    Percent = "          << readBE4F(&_rb[0x3C]) << std::endl;

    j["_frame"]    = readBE4S(&_rb[_bp+0x1]);
    j["_player"]   = _rb[_bp+0x5]+4*_rb[_bp+0x6]; //Includes follower
    // j["follower"] = _rb[_bp+0x6];
    j["seed"]     = readBE4U(&_rb[_bp+0x7]);
    j["action"]   = readBE2U(&_rb[_bp+0xB]);
    j["pos-x"]    = readBE4F(&_rb[_bp+0xD]);
    j["pos-y"]    = readBE4F(&_rb[_bp+0x11]);
    j["face-dir"] = readBE4F(&_rb[_bp+0x15]);
    j["joy-x"]    = readBE4F(&_rb[_bp+0x19]);
    j["joy-y"]    = readBE4F(&_rb[_bp+0x1D]);
    j["c-x"]      = readBE4F(&_rb[_bp+0x21]);
    j["c-y"]      = readBE4F(&_rb[_bp+0x25]);
    j["trigger"]  = readBE4F(&_rb[_bp+0x29]);
    j["buttons"]  = readBE4U(&_rb[_bp+0x31]);
    j["phys-l"]   = readBE4F(&_rb[_bp+0x33]);
    j["phys-r"]   = readBE4F(&_rb[_bp+0x37]);
    j["ucf-x"]    = uint8_t(_rb[_bp+0x3B]);
    j["percent"]  = readBE4F(&_rb[_bp+0x3C]);

    _jout["events"].append(j);
    return true;
  }

  bool Parser::_parsePostFrame() {
    // std::cout << "  Parsing post frame event" << std::endl;
    Json::Value j = Json::objectValue;
    j["_event"] = "post-frame";

    // std::cout << "    Frame number = "     << readBE4S(&_rb[0x1])  << std::endl;
    // std::cout << "    Player index = "     << +_rb[0x5]            << std::endl;
    // std::cout << "    Is follower = "      << +_rb[0x6]            << std::endl;
    // std::cout << "    Int. Char. ID = "    << +_rb[0x7]            << std::endl;
    // std::cout << "    Action State = "     << readBE2U(&_rb[0x8])  << std::endl;
    // std::cout << "    X Position = "       << readBE4F(&_rb[0xA])  << std::endl;
    // std::cout << "    Y Position = "       << readBE4F(&_rb[0xE])  << std::endl;
    // std::cout << "    Facing Direction = " << readBE4F(&_rb[0x12]) << std::endl;
    // std::cout << "    Percent = "          << readBE4F(&_rb[0x16]) << std::endl;
    // std::cout << "    Shield Size = "      << readBE4F(&_rb[0x1A]) << std::endl;
    // std::cout << "    Last Attack Land = " << +_rb[0x1E]           << std::endl;
    // std::cout << "    Cur. Combo Count = " << +_rb[0x1F]           << std::endl;
    // std::cout << "    Last Hit By = "      << +_rb[0x20]           << std::endl;
    // std::cout << "    Stocks Remaining = " << +_rb[0x21]           << std::endl;
    // std::cout << "    ASFC = "             << readBE4F(&_rb[0x22]) << std::endl;
    // std::cout << "    State Flags 1 = "    << hex(_rb[0x26])       << std::endl;
    // std::cout << "    State Flags 2 = "    << hex(_rb[0x27])       << std::endl;
    // std::cout << "    State Flags 3 = "    << hex(_rb[0x28])       << std::endl;
    // std::cout << "    State Flags 4 = "    << hex(_rb[0x29])       << std::endl;
    // std::cout << "    State Flags 5 = "    << hex(_rb[0x2A])       << std::endl;
    // std::cout << "    Hitstun, Misc AS = " << readBE4U(&_rb[0x2B]) << std::endl;
    // std::cout << "    Is Airborne = "      << +_rb[0x2F]           << std::endl;
    // std::cout << "    Ground ID = "        << readBE2U(&_rb[0x30]) << std::endl;
    // std::cout << "    Jumps Left = "       << int8_t(_rb[0x32])    << std::endl;
    // std::cout << "    L-Cancel Status = "  << +_rb[0x33]           << std::endl;

    j["_frame"]     = readBE4S(&_rb[_bp+0x1]);
    j["_player"]    = _rb[_bp+0x5]+4*_rb[_bp+0x6]; //Includes follower
    // j["follower"]  = _rb[_bp+0x6] ;
    j["char-id"]   = uint8_t(_rb[_bp+0x7]);
    j["action"]    = readBE2U(&_rb[_bp+0x8]);
    j["pos-x"]     = readBE4F(&_rb[_bp+0xA]);
    j["pos-y"]     = readBE4F(&_rb[_bp+0xE]);
    j["face-dir"]  = readBE4F(&_rb[_bp+0x12]);
    j["percent"]   = readBE4F(&_rb[_bp+0x16]);
    j["shield"]    = readBE4F(&_rb[_bp+0x1A]);
    j["hit-with"]  = uint8_t(_rb[_bp+0x1E]);
    j["combo"]     = uint8_t(_rb[_bp+0x1F]);
    j["hurt-by"]   = uint8_t(_rb[_bp+0x20]);
    j["stocks"]    = uint8_t(_rb[_bp+0x21]);
    j["action-fc"] = readBE4F(&_rb[_bp+0x22]);
    j["flags-1"]   = uint8_t(_rb[_bp+0x26]);
    j["flags-2"]   = uint8_t(_rb[_bp+0x27]);
    j["flags-3"]   = uint8_t(_rb[_bp+0x28]);
    j["flags-4"]   = uint8_t(_rb[_bp+0x29]);
    j["flags-5"]   = uint8_t(_rb[_bp+0x2A]);
    j["hitstun"]   = readBE4U(&_rb[_bp+0x2B]);
    j["airborne"]  = bool(_rb[_bp+0x2F]);
    j["ground-id"] = readBE2U(&_rb[_bp+0x30]);
    j["jumps"]     = uint8_t(_rb[_bp+0x32]);
    j["l-cancel"]  = uint8_t(_rb[_bp+0x33]);

    _jout["events"].append(j);
    return true;
  }

  bool Parser::_parseGameEnd() {
    // std::cout << "  Parsing game end event" << std::endl;
    Json::Value j = Json::objectValue;
    j["_event"] = "game-end";

    // std::cout << "    Game End Method = "     << +_rb[0x1]            << std::endl;
    // std::cout << "    LRAS Initiator = "      << +_rb[0x2]            << std::endl;

    j["game-end-method"] = _rb[_bp+0x1] ;
    j["lras-initiator"]  = _rb[_bp+0x2] ;

    _jout["events"].append(j);
    return true;
  }

  bool Parser::_parseMetadata() {
    std::cout << "Parsing metadata" << std::endl;

    //Parse metadata from UBJSON as regular JSON
    std::stringstream ss, ss2;

    std::string indent = "  ";
    std::string key = "", val = "";
    int32_t n;
    bool done = false;

    std::regex comma_killer("(,)(\\s*})");

    uint8_t strlen = 0;
    for(unsigned i = 0;;) {
      //Get next key
      switch(_rb[_bp+i]) {
        case 0x55: //U -> Length upcoming
          strlen = _rb[_bp+i+1];
          key.assign(&_rb[_bp+i+2],strlen);
          ss << indent << "\"" << key << "\" : ";
          i = i+2+strlen;
          break;
        case 0x7d: //} -> Object ending
          indent = indent.substr(2);
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
          indent = indent+"  ";
          i = i+1;
          break;
        case 0x53: //S -> string upcoming
          ss << "\"";
          if (_rb[_bp+i+1] != 0x55) {  //If the string is not of length U
            std::cerr << "Warning: found a long string we can't parse yet" << std::endl;
            std::cout << ss.str() << std::endl;
            return false;
          }
          strlen = _rb[_bp+i+2];
          val.assign(&_rb[_bp+i+3],strlen);
          ss << val << "\"," << std::endl;
          i = i+3+strlen;
          break;
        case 0x6c: //l -> 32-bit signed int upcoming
          n = readBE4S(&_rb[_bp+i+1]);
          ss << indent << std::dec << n << "," << std::endl;
          i = i+5;
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
    ss2 << "{" << mjson << "}" << std::endl;
    mjson = ss2.str();
    std::cout << mjson << std::endl;

    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string errs;
    Json::Value jmeta;
    reader->parse(mjson.c_str(),mjson.c_str()+mjson.size(),&jmeta,&errs);
    _jout["_metadata"] = jmeta["metadata"];

    return true;
  }

  void Parser::summary() {

  }

  void Parser::save(const char* outfilename) {
    std::ofstream ofile;
    ofile.open(outfilename);
    Json::StreamWriterBuilder builder;
    // builder["indentation"] = "";  // assume default for comments is None
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(_jout,&ofile);
    ofile.close();
  }


}
