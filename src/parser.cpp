#include "parser.h"

namespace slip {

  Parser::Parser(int debug_level) {
    _debug = debug_level;
    _bp    = 0;
  }

  Parser::~Parser() {
    if (_rb != nullptr) {
      delete [] _rb;
    }
    _cleanup();
  }

  bool Parser::load(const char* replayfilename) {
    DOUT1("  Loading " << replayfilename);
    _replay.original_file = std::string(replayfilename);
    std::ifstream myfile;
    myfile.open(replayfilename,std::ios::binary | std::ios::in);
    if (myfile.fail()) {
      FAIL("  File " << replayfilename << " could not be opened or does not exist");
      return false;
    }

    myfile.seekg(0, myfile.end);
    _file_size = myfile.tellg();
    if (_file_size < MIN_REPLAY_LENGTH) {
      FAIL("  File " << replayfilename << " is too short to be a valid Slippi replay");
      return false;
    }
    DOUT1("  File Size: " << +_file_size);
    myfile.seekg(0, myfile.beg);

    _rb = new char[_file_size];
    myfile.read(_rb,_file_size);
    myfile.close();

    // Check if we have a compressed .zlp file
    bool is_compressed = same4(&_rb[0],LZMA_HEADER);
    if (is_compressed) {
      DOUT1("  Decompressing file");
      // Decompress the read buffer
      std::string decomp = decompressWithLzma(_rb, _file_size);
      // Get the new file size
      _file_size    = decomp.size();
      // Delete the old read buffer
      delete[] _rb;
      // Reallocate it with more spce
      _rb = new char[_file_size];
      // Copy buffer from the decompressed string
      memcpy(_rb,decomp.c_str(),_file_size);
      DOUT1("  Decompressed File Size: " << +_file_size);
    }

    bool status = this->_parse();
    // if we attempted to parse an encoded replay
    if (_is_encoded) {
      DOUT1("  File was encoded, decoding" << +_file_size);
      // Create a Compressor object
      Compressor *d  = new slip::Compressor(0);
      // Decompress the buffer
      d->loadFromBuff(&_rb,_file_size);
      // Save it back to the original buffer
      d->saveToBuff(&_rb);
      // Unset encoded state
      _is_encoded = false;
      // restart the parsing process
      status = this->_parse();
    }
    return status;
  }

  bool Parser::_parse() {
    _bp = 0; //Start reading from byte 0
    if (not this->_parseHeader()) {
      WARN("  Failed to parse header");
      return false;
    }
    if (not this->_parseEventDescriptions()) {
      WARN("  Failed to parse event descriptions");
      return false;
    }
    if (not this->_parseEvents()) {
      WARN("  Failed to parse events proper");
      return false;
    }
    if (_is_encoded) {
      return true;  //back out and restart the parsing process
    }
    if (not this->_parseMetadata()) {
      WARN("  Failed to parse metadata");
      //Non-fatal if we can't parse metadata, so don't need to return false
    }
    if(!_game_end_found) {
      WARN_CORRUPT("  No game end event found");
      ++_replay.errors;
    }
    if (_replay.errors == 0) {
      DOUT1("  Successfully parsed replay!");
    } else {
      WARN("  Replay parsed with " << _replay.errors << " errors");
    }
    return true;
  }

  bool Parser::_parseHeader() {
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
      WARN_CORRUPT("    0-byte raw data detected");
      ++_replay.errors;
    }
    DOUT1("    Raw portion = " << _length_raw_start << " bytes");
    if (_length_raw_start > _file_size) {
      WARN_CORRUPT("    Raw data size " << +_length_raw_start << " exceeds file size of " << _file_size << " bytes");
      ++_replay.errors;
      _length_raw_start = 0;
    }
    _length_raw = _length_raw_start;
    _bp += 15;
    return true;
  }

  bool Parser::_parseEventDescriptions() {
    DOUT1("  Parsing event descriptions");

    //Next 2 bytes should be 0x35
    if (_rb[_bp] != Event::EV_PAYLOADS) {
      FAIL_CORRUPT("    Expected event 0x" << std::hex
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
          WARN_CORRUPT("    Event " << Event::name[ev_code-Event::EV_PAYLOADS] << " payload size set multiple times");
          ++_replay.errors;
        } else {
          WARN_CORRUPT("    Event " << hex(ev_code) << std::dec << " payload size set multiple times");
          ++_replay.errors;
        }
        return false;
      }
      _payload_sizes[ev_code] = readBE2U(&_rb[_bp+i+1])+1;  //Add one for the event code itself
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
    _bp += ev_bytes;
    _length_raw -= (2+ev_bytes);
    return true;
  }

  bool Parser::_parseEvents() {
    DOUT1("  Parsing events proper");

    if(_length_raw_start == 0) {  //TODO: this is /technically/ recoverable
      _length_raw_start = _file_size - _bp;
      _length_raw = _length_raw_start;
      DOUT1("    Using remaining file size " << +_length_raw << " as raw bytes");
    }

    bool success = true;
    for( ; _length_raw > 0; ) {
      unsigned ev_code = uint8_t(_rb[_bp]);
      unsigned shift   = _payload_sizes[ev_code];
      if (shift > _length_raw) {
        WARN_CORRUPT("    Event byte offset exceeds raw data length");
        ++_replay.errors;
        return true;
      }
      switch(ev_code) { //Determine the event code
        case Event::GAME_START:
          success = _parseGameStart();
          if(_is_encoded) {
            return true; //immediately restart if the file is encoded
          }
          break;
        case Event::PRE_FRAME:   success = _parsePreFrame();   break;
        case Event::POST_FRAME:  success = _parsePostFrame();  break;
        case Event::GAME_END:    success = _parseGameEnd();    break;
        case Event::ITEM_UPDATE: success = _parseItemUpdate(); break;

        case Event::SPLIT_MSG:   success = true;               break;
        case Event::FRAME_START: success = true;               break;
        case Event::BOOKEND:     success = true;               break;

        default:
          DOUT1("    Warning: unknown event code " << hex(ev_code) << " encountered; skipping");
          break;
      }
      if (not success) {
        return false;
      }
      if (_payload_sizes[ev_code] == 0) {
        WARN_CORRUPT("    Uninitialized event " << hex(ev_code) << " encountered");
        ++_replay.errors;
        return true;
      }
      _length_raw    -= shift;
      _bp            += shift;
      DOUT2("    Raw bytes remaining: " << +_length_raw);
    }

    return true;
  }

  bool Parser::_parseGameStart() {
    DOUT1("  Parsing game start event at byte " << +_bp);

    // if this is encoded, we need to decompress our entire read
    //   buffer and retry parsing
    if(_rb[_bp+O_SLP_ENC]) {
      _is_encoded = true;
      DOUT1("    File is encoded, decoding and retrying");
      return true;
    }

    if (_slippi_maj > 0) {
      WARN_CORRUPT("    Duplicate game start event");
      ++_replay.errors;
    }

    //Get Slippi version
    _slippi_maj = uint8_t(_rb[_bp+O_SLP_MAJ]); //Major version
    _slippi_min = uint8_t(_rb[_bp+O_SLP_MIN]); //Minor version
    _slippi_rev = uint8_t(_rb[_bp+O_SLP_REV]); //Build version (4th char unused)
    _replay.slippi_version_raw = readBE4U(&_rb[_bp+O_SLP_MAJ]);

    std::stringstream ss;
    ss << +_slippi_maj << "." << +_slippi_min << "." << +_slippi_rev;
    _slippi_version = ss.str();
    DOUT1("    Slippi Version: " << _slippi_version);

    //Get player info
    for(unsigned p = 0; p < 4; ++p) {
      unsigned i                     = O_PLAYERDATA + 0x24*p; //Beginning of player info block
      std::string ps                 = std::to_string(p+1);

      _replay.player[p].ext_char_id  = uint8_t(_rb[_bp+i+O_PLAYER_ID]);
      if (_replay.player[p].ext_char_id >= CharExt::__LAST) {
        WARN_CORRUPT("    External character ID " << +_replay.player[p].ext_char_id << " is invalid");
        ++_replay.errors;
      }
      _replay.player[p].player_type  = uint8_t(_rb[_bp+i+O_PLAYER_TYPE]);
      _replay.player[p].start_stocks = uint8_t(_rb[_bp+i+O_START_STOCKS]);
      _replay.player[p].color        = uint8_t(_rb[_bp+i+O_COLOR]);
      _replay.player[p].shade        = uint8_t(_rb[_bp+i+O_SHADE]);
      _replay.player[p].handicap     = uint8_t(_rb[_bp+i+O_HANDICAP]);
      _replay.player[p].team_id      = uint8_t(_rb[_bp+i+O_TEAM_ID]);
      _replay.player[p].cpu_level    = uint8_t(_rb[_bp+i+O_CPU_LEVEL]);
      _replay.player[p].offense      = readBE4F(&_rb[_bp+i+O_OFFENSE]);
      _replay.player[p].defense      = readBE4F(&_rb[_bp+i+O_DEFENSE]);
      _replay.player[p].scale        = readBE4F(&_rb[_bp+i+O_SCALE]);
      _replay.player[p].stamina      = uint8_t(_rb[_bp+i+O_PLAYER_BITS]) & 0x01;
      _replay.player[p].silent       = uint8_t(_rb[_bp+i+O_PLAYER_BITS]) & 0x02;
      _replay.player[p].low_gravity  = uint8_t(_rb[_bp+i+O_PLAYER_BITS]) & 0x04;
      _replay.player[p].invisible    = uint8_t(_rb[_bp+i+O_PLAYER_BITS]) & 0x08;
      _replay.player[p].black_stock  = uint8_t(_rb[_bp+i+O_PLAYER_BITS]) & 0x10;
      _replay.player[p].metal        = uint8_t(_rb[_bp+i+O_PLAYER_BITS]) & 0x20;
      _replay.player[p].warp_in      = uint8_t(_rb[_bp+i+O_PLAYER_BITS]) & 0x40;
      _replay.player[p].rumble       = uint8_t(_rb[_bp+i+O_PLAYER_BITS]) & 0x80;

      if(MIN_VERSION(1,0,0)) {
        unsigned m                     = O_DASHBACK   + 0x8*p;
        _replay.player[p].dash_back    = readBE4U(&_rb[_bp+m]);
        _replay.player[p].shield_drop  = readBE4U(&_rb[_bp+m+0x4]);
      }

      //Decode Melee's internal tag as a proper UTF-8 string
      if(MIN_VERSION(1,3,0)) {
        unsigned k                = O_NAMETAG    + 0x10*p;
        _replay.player[p].tag_css = decode_shift_jis(_rb+_bp+k);
        // std::cout << "CSS = " << _replay.player[p].tag_css << std::endl;
      }

      //Decode display name as a proper UTF-8 string
      if(MIN_VERSION(3,9,0)) {
        unsigned k                  = O_DISP_NAME    + 0x1F*p;
        _replay.player[p].disp_name = sj2utf8(std::string(_rb+_bp+k));
        // std::cout << "Name = " << _replay.player[p].disp_name << std::endl;
        unsigned c                  = O_CONN_CODE    + 0x0A*p;
        _replay.player[p].tag_code  = parseConnCode(std::string(_rb+_bp+c));
        // std::cout << "Code = " << _replay.player[p].tag_code << std::endl;
      }

      //Decode Slippi UID
      if(MIN_VERSION(3,11,0)) {
        unsigned k                   = O_SLIPPI_UID   + 0x1D*p;
        _replay.player[p].slippi_uid = std::string(_rb+_bp+k);
        // std::cout << "UID = " << _replay.player[p].slippi_uid << std::endl;
      }
    }

    //Write to replay data structure
    _replay.parser_version = SLIPPC_VERSION;
    _replay.slippi_version = std::string(_slippi_version);
    _replay.game_start_raw = std::string(base64_encode(reinterpret_cast<const unsigned char *>(&_rb[_bp+O_GAMEBITS_1]),312));
    _replay.metadata       = "";

    _replay.sudden_death   = bool(_rb[_bp+O_SUDDEN_DEATH]);
    _replay.teams          = bool(_rb[_bp+O_IS_TEAMS]);
    _replay.items_on       = int8_t(_rb[_bp+O_ITEM_SPAWN]);
    _replay.sd_score       = int8_t(_rb[_bp+O_SD_SCORE]);
    _replay.stage          = readBE2U(&_rb[_bp+O_STAGE]);
    _replay.timer          = readBE4U(&_rb[_bp+O_TIMER])/60;

    _replay.timer_behav    = uint8_t(_rb[_bp+O_GAMEBITS_1]) & 0x03;
    _replay.ui_chars       = (uint8_t(_rb[_bp+O_GAMEBITS_1]) & 0x1C) >> 2;
    _replay.game_mode      = (uint8_t(_rb[_bp+O_GAMEBITS_1]) & 0xE0) >> 5;
    _replay.friendly_fire  = uint8_t(_rb[_bp+O_GAMEBITS_2]) & 0x01;
    _replay.demo_mode      = uint8_t(_rb[_bp+O_GAMEBITS_2]) & 0x02;
    _replay.classic_adv    = uint8_t(_rb[_bp+O_GAMEBITS_2]) & 0x04;
    _replay.hrc_event      = uint8_t(_rb[_bp+O_GAMEBITS_2]) & 0x08;
    _replay.allstar_wait1  = uint8_t(_rb[_bp+O_GAMEBITS_2]) & 0x10;
    _replay.allstar_wait2  = uint8_t(_rb[_bp+O_GAMEBITS_2]) & 0x20;
    _replay.allstar_game1  = uint8_t(_rb[_bp+O_GAMEBITS_2]) & 0x40;
    _replay.allstar_game2  = uint8_t(_rb[_bp+O_GAMEBITS_2]) & 0x80;
    _replay.single_button  = uint8_t(_rb[_bp+O_GAMEBITS_3]) & 0x10;
    _replay.pause_timer    = uint8_t(_rb[_bp+O_GAMEBITS_4]) & 0x01;
    _replay.pause_nohud    = uint8_t(_rb[_bp+O_GAMEBITS_4]) & 0x02;
    _replay.pause_lras     = uint8_t(_rb[_bp+O_GAMEBITS_4]) & 0x04;
    _replay.pause_off      = uint8_t(_rb[_bp+O_GAMEBITS_4]) & 0x08;
    _replay.pause_zretry   = uint8_t(_rb[_bp+O_GAMEBITS_4]) & 0x10;
    _replay.pause_analog   = uint8_t(_rb[_bp+O_GAMEBITS_4]) & 0x40;
    _replay.pause_score    = uint8_t(_rb[_bp+O_GAMEBITS_4]) & 0x80;
    _replay.items1         = uint8_t(_rb[_bp+O_ITEMBITS_1]);
    _replay.items2         = uint8_t(_rb[_bp+O_ITEMBITS_2]);
    _replay.items3         = uint8_t(_rb[_bp+O_ITEMBITS_3]);
    _replay.items4         = uint8_t(_rb[_bp+O_ITEMBITS_4]);
    _replay.items5         = uint8_t(_rb[_bp+O_ITEMBITS_5]);
    if (_replay.stage >= Stage::__LAST) {
      WARN_CORRUPT("    Stage ID " << +_replay.stage << " is invalid");
      ++_replay.errors;
    }
    _replay.seed           = readBE4U(&_rb[_bp+O_RNG_GAME_START]);

    if(MIN_VERSION(1,5,0)) {
      _replay.pal            = bool(_rb[_bp+O_IS_PAL]);
    }

    if(MIN_VERSION(2,0,0)) {
      _replay.frozen_stadium = bool(_rb[_bp+O_PS_FROZEN]);
    }

    if(MIN_VERSION(3,7,0)) {
      _replay.scene_min      = uint8_t(_rb[_bp+O_SCENE_MIN]);
      _replay.scene_maj      = uint8_t(_rb[_bp+O_SCENE_MAJ]);
    }

    if(MIN_VERSION(3,12,0)) {
      _replay.language       = uint8_t(_rb[_bp+O_LANGUAGE]);
    }

    if(MIN_VERSION(3,14,0)) {
      _replay.match_id = std::string(_rb+_bp+O_MATCH_ID);
      _replay.game_number      = readBE4U(&_rb[_bp+O_GAME_NUMBER]);
      _replay.tiebreaker_number= readBE4U(&_rb[_bp+O_TIEBREAKER_NUMBER]);
    }

    _max_frames = getMaxNumFrames();
    _replay.setFrames(_max_frames);
    DOUT1("    Estimated " << _max_frames << " gameplay frames (" << (_replay.frame_count) << " total frames)");
    return true;
  }

  bool Parser::_parsePreFrame() {
    DOUT2("  Parsing pre frame event at byte " << +_bp);
    int32_t fnum = readBE4S(&_rb[_bp+O_FRAME]);
    int32_t f    = fnum-LOAD_FRAME;

    if (fnum < LOAD_FRAME) {
      FAIL_CORRUPT("    Frame index " << fnum << " less than " << +LOAD_FRAME);
      return false;
    }
    if (fnum >= _max_frames) {
      FAIL_CORRUPT("    Frame index " << fnum
        << " greater than max frames computed from reported raw size (" << _max_frames << ")");
      return false;
    }

    uint8_t p    = uint8_t(_rb[_bp+O_PLAYER])+4*uint8_t(_rb[_bp+O_FOLLOWER]); //Includes follower
    if (p > 7 || _replay.player[p].frame == nullptr) {
      FAIL_CORRUPT("    Invalid player index " << +p);
      return false;
    }

    _replay.last_frame                      = fnum;
    _replay.frame_count                     = f+1; //Update the last frame we actually read
    _replay.player[p].frame[f].frame        = fnum;
    _replay.player[p].frame[f].player       = p%4;
    _replay.player[p].frame[f].follower     = (p>3);
    _replay.player[p].frame[f].alive        = 1;
    _replay.player[p].frame[f].seed         = readBE4U(&_rb[_bp+O_RNG_PRE]);
    _replay.player[p].frame[f].action_pre   = readBE2U(&_rb[_bp+O_ACTION_PRE]);
    _replay.player[p].frame[f].pos_x_pre    = readBE4F(&_rb[_bp+O_XPOS_PRE]);
    _replay.player[p].frame[f].pos_y_pre    = readBE4F(&_rb[_bp+O_YPOS_PRE]);
    _replay.player[p].frame[f].face_dir_pre = readBE4F(&_rb[_bp+O_FACING_PRE]);
    _replay.player[p].frame[f].joy_x        = readBE4F(&_rb[_bp+O_JOY_X]);
    _replay.player[p].frame[f].joy_y        = readBE4F(&_rb[_bp+O_JOY_Y]);
    _replay.player[p].frame[f].c_x          = readBE4F(&_rb[_bp+O_CX]);
    _replay.player[p].frame[f].c_y          = readBE4F(&_rb[_bp+O_CY]);
    _replay.player[p].frame[f].trigger      = readBE4F(&_rb[_bp+O_TRIGGER]);
    _replay.player[p].frame[f].buttons      = readBE2U(&_rb[_bp+O_BUTTONS]);
    _replay.player[p].frame[f].phys_l       = readBE4F(&_rb[_bp+O_PHYS_L]);
    _replay.player[p].frame[f].phys_r       = readBE4F(&_rb[_bp+O_PHYS_R]);

    if(MIN_VERSION(1,2,0)) {
      _replay.player[p].frame[f].ucf_x        = uint8_t(_rb[_bp+O_UCF_ANALOG]);
    }

    if(MIN_VERSION(1,4,0)) {
      _replay.player[p].frame[f].percent_pre  = readBE4F(&_rb[_bp+O_DAMAGE_PRE]);
    }

    return true;
  }

  bool Parser::_parsePostFrame() {
    DOUT2("  Parsing post frame event at byte " << +_bp);
    int32_t fnum = readBE4S(&_rb[_bp+O_FRAME]);
    int32_t f    = fnum-LOAD_FRAME;

    if (fnum < LOAD_FRAME) {
      FAIL_CORRUPT("    Frame index " << fnum << " less than " << +LOAD_FRAME);
      return false;
    }
    if (fnum >= _max_frames) {
      FAIL_CORRUPT("    Frame index " << fnum << " greater than max frames computed from reported raw size ("
        << _max_frames << ")");
      return false;
    }

    uint8_t p    = uint8_t(_rb[_bp+O_PLAYER])+4*uint8_t(_rb[_bp+O_FOLLOWER]); //Includes follower
    if (p > 7 || _replay.player[p].frame == nullptr) {
      FAIL_CORRUPT("    Invalid player index " << +p);
      return false;
    }

    _replay.player[p].frame[f].char_id       = uint8_t(_rb[_bp+O_INT_CHAR_ID]);
    if (_replay.player[p].frame[f].char_id >= CharInt::__LAST) {
      WARN_CORRUPT("    Internal character ID " << +_replay.player[p].frame[f].char_id << " is invalid");
      ++_replay.errors;
    }

    _replay.player[p].frame[f].action_post   = readBE2U(&_rb[_bp+O_ACTION_POST]);
    _replay.player[p].frame[f].pos_x_post    = readBE4F(&_rb[_bp+O_XPOS_POST]);
    _replay.player[p].frame[f].pos_y_post    = readBE4F(&_rb[_bp+O_YPOS_POST]);
    _replay.player[p].frame[f].face_dir_post = readBE4F(&_rb[_bp+O_FACING_POST]);
    _replay.player[p].frame[f].percent_post  = readBE4F(&_rb[_bp+O_DAMAGE_POST]);
    _replay.player[p].frame[f].shield        = readBE4F(&_rb[_bp+O_SHIELD]);
    _replay.player[p].frame[f].hit_with      = uint8_t(_rb[_bp+O_LAST_HIT_ID]);
    _replay.player[p].frame[f].combo         = uint8_t(_rb[_bp+O_COMBO]);
    _replay.player[p].frame[f].hurt_by       = uint8_t(_rb[_bp+O_LAST_HIT_BY]);
    _replay.player[p].frame[f].stocks        = uint8_t(_rb[_bp+O_STOCKS]);

    if(MIN_VERSION(0,2,0)) {
      _replay.player[p].frame[f].action_fc     = readBE4F(&_rb[_bp+O_ACTION_FRAMES]);
    }

    if(MIN_VERSION(2,0,0)) {
      _replay.player[p].frame[f].flags_1       = uint8_t(_rb[_bp+O_STATE_BITS_1]);
      _replay.player[p].frame[f].flags_2       = uint8_t(_rb[_bp+O_STATE_BITS_2]);
      _replay.player[p].frame[f].flags_3       = uint8_t(_rb[_bp+O_STATE_BITS_3]);
      _replay.player[p].frame[f].flags_4       = uint8_t(_rb[_bp+O_STATE_BITS_4]);
      _replay.player[p].frame[f].flags_5       = uint8_t(_rb[_bp+O_STATE_BITS_5]);
      _replay.player[p].frame[f].hitstun       = readBE4F(&_rb[_bp+O_HITSTUN]);
      _replay.player[p].frame[f].airborne      = bool(_rb[_bp+O_AIRBORNE]);
      _replay.player[p].frame[f].ground_id     = readBE2U(&_rb[_bp+O_GROUND_ID]);
      _replay.player[p].frame[f].jumps         = uint8_t(_rb[_bp+O_JUMPS]);
      _replay.player[p].frame[f].l_cancel      = uint8_t(_rb[_bp+O_LCANCEL]);
    }

    if(MIN_VERSION(2,1,0)) {
      _replay.player[p].frame[f].hurtbox       = uint8_t(_rb[_bp+O_HURTBOX]);
    }

    if(MIN_VERSION(3,5,0)) {
      _replay.player[p].frame[f].self_air_x    = readBE4F(&_rb[_bp+O_SELF_AIR_X]);
      _replay.player[p].frame[f].self_air_y    = readBE4F(&_rb[_bp+O_SELF_AIR_Y]);
      _replay.player[p].frame[f].attack_x      = readBE4F(&_rb[_bp+O_ATTACK_X]);
      _replay.player[p].frame[f].attack_y      = readBE4F(&_rb[_bp+O_ATTACK_Y]);
      _replay.player[p].frame[f].self_grd_x    = readBE4F(&_rb[_bp+O_SELF_GROUND_X]);
    }

    if(MIN_VERSION(3,8,0)) {
      _replay.player[p].frame[f].hitlag        = readBE4F(&_rb[_bp+O_HITLAG]);
    }

    if(MIN_VERSION(3,11,0)) {
      _replay.player[p].frame[f].anim_index    = readBE4U(&_rb[_bp+O_ANIM_INDEX]);
    }

    return true;
  }

  bool Parser::_parseItemUpdate() {
    DOUT2("  Parsing item frame event at byte " << +_bp);
    int32_t fnum = readBE4S(&_rb[_bp+O_FRAME]);

    if (fnum < LOAD_FRAME) {
      FAIL_CORRUPT("    Frame index " << fnum << " less than " << +LOAD_FRAME);
      return false;
    }
    if (fnum >= _max_frames) {
      FAIL_CORRUPT("    Frame index " << fnum << " greater than max frames computed from reported raw size ("
        << _max_frames << ")");
      return false;
    }

    uint32_t id    = readBE4U(&_rb[_bp+O_ITEM_ID]);

    if (id >= MAX_ITEMS) {
      return true;  //We can't output this many items (TODO: what's with item ID 3039053192???)
    }

    int32_t f = _replay.item[id].num_frames;
    _replay.item[id].spawn_id = id;
    if (_replay.item[id].frame == nullptr) {
      _replay.item[id].frame = new SlippiItemFrame[MAX_ITEM_LIFE];
    }
    if (id >= _replay.num_items) {
      _replay.num_items = id + 1;
    }

    if (f < MAX_ITEM_LIFE) {
      _replay.item[id].num_frames       += 1;
      _replay.item[id].type              = readBE2U(&_rb[_bp+O_ITEM_TYPE]);
      _replay.item[id].frame[f].frame    = fnum;
      _replay.item[id].frame[f].state    = uint8_t(_rb[_bp+O_ITEM_STATE]);
      _replay.item[id].frame[f].face_dir = readBE4F(&_rb[_bp+O_ITEM_FACING]);
      _replay.item[id].frame[f].xvel     = readBE4F(&_rb[_bp+O_ITEM_XVEL]);
      _replay.item[id].frame[f].yvel     = readBE4F(&_rb[_bp+O_ITEM_YVEL]);
      _replay.item[id].frame[f].xpos     = readBE4F(&_rb[_bp+O_ITEM_XPOS]);
      _replay.item[id].frame[f].ypos     = readBE4F(&_rb[_bp+O_ITEM_YPOS]);
      _replay.item[id].frame[f].damage   = readBE2U(&_rb[_bp+O_ITEM_DAMAGE]);
      _replay.item[id].frame[f].expire   = readBE4F(&_rb[_bp+O_ITEM_EXPIRE]);
      if(MIN_VERSION(3,2,0)) {
        _replay.item[id].frame[f].flags_1  = uint8_t(_rb[_bp+O_ITEM_MISC]);
        _replay.item[id].frame[f].flags_2  = uint8_t(_rb[_bp+O_ITEM_MISC+1]);
        _replay.item[id].frame[f].flags_3  = uint8_t(_rb[_bp+O_ITEM_MISC+2]);
        _replay.item[id].frame[f].flags_4  = uint8_t(_rb[_bp+O_ITEM_MISC+3]);
      }
      if(MIN_VERSION(3,6,0)) {
        _replay.item[id].frame[f].owner    = int8_t(_rb[_bp+O_ITEM_OWNER]);
      }
    } else {
      DOUT2("    Item " << +id << " was alive longer than expected ");
    }

    return true;
  }

  bool Parser::_parseGameEnd() {
    DOUT1("  Parsing game end event at byte " << +_bp);
    _game_end_found          = true;
    _replay.end_type         = uint8_t(_rb[_bp+O_END_METHOD]);

    if(MIN_VERSION(2,0,0)) {
      _replay.lras           = int8_t(_rb[_bp+O_LRAS]);
    }

    // Determine game winner
    // TODO: should account for LRAS
    int   winner_stocks = 0;
    float winner_damage = 0;
    for(unsigned p = 0; p < 4; ++p) {
      if (_replay.player[p].player_type == 3) {
        continue;  //If we're not playing, we probably didn't win
      }
      int   end_stocks = _replay.player[p].frame[_replay.frame_count-1].stocks;
      _replay.player[p].end_stocks = end_stocks;
      float end_damage = _replay.player[p].frame[_replay.frame_count-1].percent_post;
      if ((end_stocks > winner_stocks) || (end_stocks == winner_stocks && end_damage < winner_damage)) {
        winner_stocks = end_stocks;
        winner_damage = end_damage;
        _replay.winner_id = p;  //Tentatively set this person as the winner
      }
    }
    return true;
  }

  bool Parser::_parseMetadata() {
    DOUT1("  Parsing metadata");

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
          WARN("    Don't know what's happening; expected key");
          ++_replay.errors;
          return false;
      }
      if (done) {
        break;
      } else if (fail || _bp+i >= _file_size) {
        WARN("    Metadata shorter than expected");
        ++_replay.errors;
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
            WARN("    Found a long string we can't parse yet:"
              << std::endl
              << "  " << ss.str() << std::endl);
            ++_replay.errors;
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
              // check if connect code was already set in game start block
              if (_replay.player[port].tag_code.compare("") == 0) {
                _replay.player[port].tag_code = val;
              }
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
          WARN("    Don't know what's happening; expected value");
          return false;
      }
      if (fail) {
        WARN("    Metadata shorter than expected");
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

  Analysis* Parser::analyze() {
    Analyzer a(_debug);
    return a.analyze(_replay);
  }

  void Parser::_cleanup() {
    _replay.cleanup();
  }

  std::string Parser::asJson(bool delta) {
    return _replay.replayAsJson(delta);
  }

  void Parser::save(const char* outfilename,bool delta) {
    DOUT1("  Saving JSON");
    std::ofstream ofile2;
    ofile2.open(outfilename);
    ofile2 << asJson(delta) << std::endl;
    ofile2.close();
    DOUT1("  Saved to " << outfilename);
  }

}
