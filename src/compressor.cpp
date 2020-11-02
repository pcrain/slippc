#include "compressor.h"

//Debug output convenience macros
#define DOUT1(s) if (_debug >= 1) { std::cout << s; }
#define DOUT2(s) if (_debug >= 2) { std::cout << s; }
#define FAIL(e) std::cerr << "ERROR: " << e << std::endl
#define FAIL_CORRUPT(e) std::cerr << "ERROR: " << e << "; replay may be corrupt" << std::endl

// static inline void buildMap(uint16_t v1) {
//   printf("Mapping %u to %u\n",v1,_mapped_c);
//   ACTION_MAP[v1]        = _mapped_c;
//   ACTION_REV[_mapped_c] = v1;
//   ++_mapped_c;
// }

namespace slip {

  Compressor::Compressor(int debug_level) {
    _debug = debug_level;
    _bp    = 0;

    // //Initilize action map
    // buildMap(Action::AttackAirB);
    // buildMap(Action::LandingFallSpecial);
    // buildMap(Action::DamageFlyHi);
    // buildMap(Action::Dash);
    // buildMap(Action::AttackAirF);
    // buildMap(Action::AttackAirLw);
    // buildMap(Action::JumpF);
    // buildMap(Action::KneeBend);
    // buildMap(Action::Wait);
    // buildMap(Action::Fall);
    // buildMap(Action::JumpAerialF);
    // buildMap(Action::ItemHammerWait);
    // buildMap(Action::DamageFlyTop);
    // buildMap(Action::AttackAirHi);
    // buildMap(Action::Wait1);
    // buildMap(Action::Rebirth);
    // buildMap(Action::DamageFlyN);
    // buildMap(Action::Landing);
    // buildMap(Action::LandingAirLw);
    // buildMap(Action::Turn);
    // buildMap(Action::LandingAirB);
    // buildMap(Action::AttackAirN);
    // buildMap(Action::DownBoundU);
    // buildMap(Action::JumpB);
    // buildMap(Action::DeadDown);
    // buildMap(Action::EscapeAir);
    // buildMap(Action::DamageFlyRoll);
    // buildMap(Action::DamageAir3);
    // buildMap(Action::GuardOn);
    // buildMap(Action::AttackLw4);
    // buildMap(Action::EscapeB);
    // buildMap(Action::Guard);
    // buildMap(Action::AttackHi3);
    // buildMap(Action::LandingAirF);
    // buildMap(Action::DamageFlyLw);
    // buildMap(Action::CliffCatch);
    // buildMap(Action::Catch);
    // buildMap(Action::SquatWait);
    // buildMap(Action::ThrowLw);
    // buildMap(Action::CliffWait);
    // buildMap(Action::GuardSetOff);
    // buildMap(Action::GuardDamage);
    // buildMap(Action::DamageHi3);
    // buildMap(Action::DamageFall);
    // buildMap(Action::AttackS4S);
    // buildMap(Action::JumpAerialB);
    // buildMap(Action::AttackLw3);
    // buildMap(Action::GuardOff);
    // buildMap(Action::DownFowardD);
    // buildMap(Action::PassiveStandB);
    // buildMap(Action::AttackS4Hold);
    // buildMap(Action::DamageAir2);
    // buildMap(Action::Squat);
    // buildMap(Action::Wait4);
    // buildMap(Action::SlipEscapeF);
    // buildMap(Action::LandingAirN);
    // buildMap(Action::Run);
    // buildMap(Action::Wait3);
    // buildMap(Action::DownBackU);
    // buildMap(Action::Pass);
    // buildMap(Action::ThrownLw);
    // buildMap(Action::WalkSlow);
    // buildMap(Action::Wait2);
    // buildMap(Action::EscapeF);
    // buildMap(Action::DeadLeft);
    // buildMap(Action::ItemBlind);
    // buildMap(Action::DeadRight);
    // buildMap(Action::EntryEnd);
    // buildMap(Action::LandingAirHi);
    // buildMap(Action::DownStandD);
    // buildMap(Action::EntryStart);
    // buildMap(Action::DamageN3);
    // buildMap(Action::AttackHi4);
    // buildMap(Action::CliffAttackQuick);
    // buildMap(Action::ThrowB);
    // buildMap(Action::DamageN2);
    // buildMap(Action::DownBoundD);
    // buildMap(Action::WalkMiddle);
    // buildMap(Action::SlipEscapeB);
    // buildMap(Action::AttackS3S);
    // buildMap(Action::HeavyWalk1);
    // buildMap(Action::FallSpecial);
    // buildMap(Action::DownFowardU);
    // buildMap(Action::DownBackD);
    // buildMap(Action::PassiveWallJump);
    // buildMap(Action::DownStandU);
    // buildMap(Action::FuraSleepLoop);
    // buildMap(Action::Escape);
    // buildMap(Action::WaitItem);
    // buildMap(Action::SlipStand);
    // buildMap(Action::GuardReflect);
    // buildMap(Action::EscapeN);
    // buildMap(Action::ThrownB);
    // buildMap(Action::CaptureWaitKirby);
    // buildMap(Action::WallDamage);
    // buildMap(Action::Ottotto);
    // buildMap(Action::DownDamageD);
    // buildMap(Action::SquatRv);
    // buildMap(Action::Passive);
    // buildMap(Action::ItemHammerMove);
    // buildMap(Action::CaptureCaptain);
    // buildMap(Action::Attack12);
    // buildMap(Action::ThrownKirbyStar);
    // buildMap(Action::FuraSleepStart);
    // buildMap(Action::AttackS3Lw);
    // buildMap(Action::RebirthWait);
    // buildMap(Action::CatchWait);
    // buildMap(Action::Entry);
    // buildMap(Action::Zitabata);
    // buildMap(Action::WalkFast);
    // buildMap(Action::CaptureKirby);
    // buildMap(Action::CatchPull);
    // buildMap(Action::DownWaitU);
    // buildMap(Action::CaptureWaitLw);
    // buildMap(Action::CaptureWaitHi);
    // buildMap(Action::Attack11);
    // buildMap(Action::DamageLw3);
    // buildMap(Action::CapturePulledLw);
    // buildMap(Action::OttottoWait);
    // buildMap(Action::ThrownKirby);
    // buildMap(Action::CapturePulledHi);
    // buildMap(Action::ThrownKoopaEndF);
    // buildMap(Action::YoshiEgg);
    // buildMap(Action::WarpStarJump);
    // buildMap(Action::WarpStarFall);
    // buildMap(Action::TurnRun);
    // buildMap(Action::ThrownMewtwoAir);
    // buildMap(Action::ThrownMewtwo);
    // buildMap(Action::ThrownMasterhand);
    // buildMap(Action::ThrownLwWomen);
    // buildMap(Action::ThrownKoopaF);
    // buildMap(Action::ThrownKoopaEndB);
    // buildMap(Action::ThrownKoopaB);
    // buildMap(Action::ThrownKoopaAirF);
    // buildMap(Action::ThrownKoopaAirEndF);
    // buildMap(Action::ThrownKoopaAirEndB);
    // buildMap(Action::ThrownKoopaAirB);
    // buildMap(Action::ThrownKirbySpitSShot);
    // buildMap(Action::ThrownKirbyDrinkSShot);
    // buildMap(Action::ThrownHi);
    // buildMap(Action::ThrownFLw);
    // buildMap(Action::ThrownFHi);
    // buildMap(Action::ThrownFF);
    // buildMap(Action::ThrownFB);
    // buildMap(Action::ThrownF);
    // buildMap(Action::ThrownCrazyhand);
    // buildMap(Action::ThrownCopyStar);
    // buildMap(Action::ThrowHi);
    // buildMap(Action::ThrowF);
    // buildMap(Action::SwordSwingDash);
    // buildMap(Action::SwordSwing4);
    // buildMap(Action::SwordSwing3);
    // buildMap(Action::SwordSwing1);
    // buildMap(Action::StopWall);
    // buildMap(Action::StopCeil);
    // buildMap(Action::StarRodSwingDash);
    // buildMap(Action::StarRodSwing4);
    // buildMap(Action::StarRodSwing3);
    // buildMap(Action::StarRodSwing1);
    // buildMap(Action::SquatWaitItem);
    // buildMap(Action::SquatWait2);
    // buildMap(Action::SquatWait1);
    // buildMap(Action::SlipWait);
    // buildMap(Action::SlipTurn);
    // buildMap(Action::SlipDown);
    // buildMap(Action::SlipDash);
    // buildMap(Action::SlipAttack);
    // buildMap(Action::Slip);
    // buildMap(Action::Sleep);
    // buildMap(Action::ShoulderedWalkSlow);
    // buildMap(Action::ShoulderedWalkMiddle);
    // buildMap(Action::ShoulderedWalkFast);
    // buildMap(Action::ShoulderedWait);
    // buildMap(Action::ShoulderedTurn);
    // buildMap(Action::ShieldBreakStandU);
    // buildMap(Action::ShieldBreakStandD);
    // buildMap(Action::ShieldBreakFly);
    // buildMap(Action::ShieldBreakFall);
    // buildMap(Action::ShieldBreakDownU);
    // buildMap(Action::ShieldBreakDownD);
    // buildMap(Action::RunDirect);
    // buildMap(Action::RunBrake);
    // buildMap(Action::ReboundStop);
    // buildMap(Action::Rebound);
    // buildMap(Action::PassiveWall);
    // buildMap(Action::PassiveStandF);
    // buildMap(Action::PassiveCeil);
    // buildMap(Action::ParasolSwingDash);
    // buildMap(Action::ParasolSwing4);
    // buildMap(Action::ParasolSwing3);
    // buildMap(Action::ParasolSwing1);
    // buildMap(Action::MissFoot);
    // buildMap(Action::LipStickSwingDash);
    // buildMap(Action::LipStickSwing4);
    // buildMap(Action::LipStickSwing3);
    // buildMap(Action::LipStickSwing1);
    // buildMap(Action::LightThrowLw4);
    // buildMap(Action::LightThrowLw);
    // buildMap(Action::LightThrowHi4);
    // buildMap(Action::LightThrowHi);
    // buildMap(Action::LightThrowF4);
    // buildMap(Action::LightThrowF);
    // buildMap(Action::LightThrowDrop);
    // buildMap(Action::LightThrowDash);
    // buildMap(Action::LightThrowB4);
    // buildMap(Action::LightThrowB);
    // buildMap(Action::LightThrowAirLw4);
    // buildMap(Action::LightThrowAirLw);
    // buildMap(Action::LightThrowAirHi4);
    // buildMap(Action::LightThrowAirHi);
    // buildMap(Action::LightThrowAirF4);
    // buildMap(Action::LightThrowAirF);
    // buildMap(Action::LightThrowAirB4);
    // buildMap(Action::LightThrowAirB);
    // buildMap(Action::LightGet);
    // buildMap(Action::LiftWalk2);
    // buildMap(Action::LiftWalk1);
    // buildMap(Action::LiftWait);
    // buildMap(Action::LiftTurn);
    // buildMap(Action::LGunShootEmpty);
    // buildMap(Action::LGunShootAirEmpty);
    // buildMap(Action::LGunShootAir);
    // buildMap(Action::LGunShoot);
    // buildMap(Action::KirbyYoshiEgg);
    // buildMap(Action::KinokoSmallStartAir);
    // buildMap(Action::KinokoSmallStart);
    // buildMap(Action::KinokoSmallEndAir);
    // buildMap(Action::KinokoSmallEnd);
    // buildMap(Action::KinokoGiantStartAir);
    // buildMap(Action::KinokoGiantStart);
    // buildMap(Action::KinokoGiantEndAir);
    // buildMap(Action::KinokoGiantEnd);
    // buildMap(Action::ItemScrewAir);
    // buildMap(Action::ItemScrew);
    // buildMap(Action::ItemScopeStartEmpty);
    // buildMap(Action::ItemScopeStart);
    // buildMap(Action::ItemScopeRapidEmpty);
    // buildMap(Action::ItemScopeRapid);
    // buildMap(Action::ItemScopeFireEmpty);
    // buildMap(Action::ItemScopeFire);
    // buildMap(Action::ItemScopeEndEmpty);
    // buildMap(Action::ItemScopeEnd);
    // buildMap(Action::ItemScopeAirStartEmpty);
    // buildMap(Action::ItemScopeAirStart);
    // buildMap(Action::ItemScopeAirRapidEmpty);
    // buildMap(Action::ItemScopeAirRapid);
    // buildMap(Action::ItemScopeAirFireEmpty);
    // buildMap(Action::ItemScopeAirFire);
    // buildMap(Action::ItemScopeAirEndEmpty);
    // buildMap(Action::ItemScopeAirEnd);
    // buildMap(Action::ItemParasolOpen);
    // buildMap(Action::ItemParasolFallSpecial);
    // buildMap(Action::ItemParasolFall);
    // buildMap(Action::ItemParasolDamageFall);
    // buildMap(Action::HeavyWalk2);
    // buildMap(Action::HeavyThrowLw4);
    // buildMap(Action::HeavyThrowLw);
    // buildMap(Action::HeavyThrowHi4);
    // buildMap(Action::HeavyThrowHi);
    // buildMap(Action::HeavyThrowF4);
    // buildMap(Action::HeavyThrowF);
    // buildMap(Action::HeavyThrowB4);
    // buildMap(Action::HeavyThrowB);
    // buildMap(Action::HeavyGet);
    // buildMap(Action::HarisenSwingDash);
    // buildMap(Action::HarisenSwing4);
    // buildMap(Action::HarisenSwing3);
    // buildMap(Action::HarisenSwing1);
    // buildMap(Action::HammerWalk);
    // buildMap(Action::HammerWait);
    // buildMap(Action::HammerTurn);
    // buildMap(Action::HammerLanding);
    // buildMap(Action::HammerKneeBend);
    // buildMap(Action::HammerJump);
    // buildMap(Action::HammerFall);
    // buildMap(Action::FuraSleepEnd);
    // buildMap(Action::FuraFura);
    // buildMap(Action::FlyReflectWall);
    // buildMap(Action::FlyReflectCeil);
    // buildMap(Action::FireFlowerShootAir);
    // buildMap(Action::FireFlowerShoot);
    // buildMap(Action::FallSpecialF);
    // buildMap(Action::FallSpecialB);
    // buildMap(Action::FallF);
    // buildMap(Action::FallB);
    // buildMap(Action::FallAerialF);
    // buildMap(Action::FallAerialB);
    // buildMap(Action::FallAerial);
    // buildMap(Action::DownWaitD);
    // buildMap(Action::DownSpotU);
    // buildMap(Action::DownSpotD);
    // buildMap(Action::DownReflect);
    // buildMap(Action::DownDamageU);
    // buildMap(Action::DownAttackU);
    // buildMap(Action::DownAttackD);
    // buildMap(Action::DeadUpStarIce);
    // buildMap(Action::DeadUpStar);
    // buildMap(Action::DeadUpFallIce);
    // buildMap(Action::DeadUpFallHitCameraIce);
    // buildMap(Action::DeadUpFallHitCameraFlat);
    // buildMap(Action::DeadUpFallHitCamera);
    // buildMap(Action::DeadUp);
    // buildMap(Action::DamageSongWait);
    // buildMap(Action::DamageSongRv);
    // buildMap(Action::DamageSong);
    // buildMap(Action::DamageScrewAir);
    // buildMap(Action::DamageScrew);
    // buildMap(Action::DamageN1);
    // buildMap(Action::DamageLw2);
    // buildMap(Action::DamageLw1);
    // buildMap(Action::DamageIceJump);
    // buildMap(Action::DamageIce);
    // buildMap(Action::DamageHi2);
    // buildMap(Action::DamageHi1);
    // buildMap(Action::DamageElec);
    // buildMap(Action::DamageBind);
    // buildMap(Action::DamageAir1);
    // buildMap(Action::CliffWait2);
    // buildMap(Action::CliffWait1);
    // buildMap(Action::CliffJumpSlow2);
    // buildMap(Action::CliffJumpSlow1);
    // buildMap(Action::CliffJumpQuick2);
    // buildMap(Action::CliffJumpQuick1);
    // buildMap(Action::CliffEscapeSlow);
    // buildMap(Action::CliffEscapeQuick);
    // buildMap(Action::CliffClimbSlow);
    // buildMap(Action::CliffClimbQuick);
    // buildMap(Action::CliffAttackSlow);
    // buildMap(Action::CatchDashPull);
    // buildMap(Action::CatchDash);
    // buildMap(Action::CatchCut);
    // buildMap(Action::CatchAttack);
    // buildMap(Action::CaptureYoshi);
    // buildMap(Action::CapturewaitMasterhand);
    // buildMap(Action::CaptureWaitKoopaAir);
    // buildMap(Action::CaptureWaitKoopa);
    // buildMap(Action::CapturewaitCrazyhand);
    // buildMap(Action::CaptureNeck);
    // buildMap(Action::CaptureMewtwoAir);
    // buildMap(Action::CaptureMewtwo);
    // buildMap(Action::CaptureMasterhand);
    // buildMap(Action::CaptureLikelike);
    // buildMap(Action::CaptureLeadead);
    // buildMap(Action::CaptureKoopaHit);
    // buildMap(Action::CaptureKoopaAirHit);
    // buildMap(Action::CaptureKoopaAir);
    // buildMap(Action::CaptureKoopa);
    // buildMap(Action::CaptureKirbyYoshi);
    // buildMap(Action::CaptureJump);
    // buildMap(Action::CaptureFoot);
    // buildMap(Action::CapturedamageMasterhand);
    // buildMap(Action::CaptureDamageLw);
    // buildMap(Action::CaptureDamageKoopaAir);
    // buildMap(Action::CaptureDamageKoopa);
    // buildMap(Action::CaptureDamageHi);
    // buildMap(Action::CapturedamageCrazyhand);
    // buildMap(Action::CaptureCut);
    // buildMap(Action::CaptureCrazyhand);
    // buildMap(Action::BuryWait);
    // buildMap(Action::BuryJump);
    // buildMap(Action::Bury);
    // buildMap(Action::BatSwingDash);
    // buildMap(Action::BatSwing4);
    // buildMap(Action::BatSwing3);
    // buildMap(Action::BatSwing1);
    // buildMap(Action::BarrelWait);
    // buildMap(Action::BarrelCannonWait);
    // buildMap(Action::AttackS4LwS);
    // buildMap(Action::AttackS4Lw);
    // buildMap(Action::AttackS4HiS);
    // buildMap(Action::AttackS4Hi);
    // buildMap(Action::AttackS3LwS);
    // buildMap(Action::AttackS3HiS);
    // buildMap(Action::AttackS3Hi);
    // buildMap(Action::AttackDash);
    // buildMap(Action::Attack13);
    // buildMap(Action::Attack100Start);
    // buildMap(Action::Attack100Loop);
    // buildMap(Action::Attack100End);
    // buildMap(Action::AppealS);
    // buildMap(Action::AppealR);
    // buildMap(Action::AppealL);
    // buildMap(Action::_NONE1);

    // //Map remaining action states
    // for(unsigned i = 0; i < Action::__LAST; ++i) {
    //   if (ACTION_MAP.find(i) == ACTION_MAP.end()) {
    //     buildMap(i);
    //   }
    // }
  }

  Compressor::~Compressor() {
    if (_rb != nullptr) { delete [] _rb; }
    if (_wb != nullptr) { delete [] _wb; }
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
    _bp         += ev_bytes;
    _length_raw -= (2+ev_bytes);
    return true;
  }

  bool Compressor::_parseEvents() {
    DOUT1("Parsing events proper" << std::endl);

    bool success = true;
    for( ; _length_raw > 0; ) {
      unsigned ev_code = uint8_t(_rb[_bp]);

      //Compress event codes
      if (_slippi_maj > 0) {  //If we've parsed the game start event
        const int MIN_EVENT_ENCODE_RANGE = 0x10; //Do not encode events of this value or higher
        const int MAX_EVENT_ENCODE_RANGE = 0x30; //Do not encode events of this value or lower
        const int DLT_EVENT_ENCODE_RANGE = 0x36; //Encode events by shifting them down this much
        if (_is_encoded) {
          if (ev_code < MIN_EVENT_ENCODE_RANGE) {
            _wb[_bp] += DLT_EVENT_ENCODE_RANGE;
            ev_code = _wb[_bp];
          }
        } else {
          if (ev_code > MAX_EVENT_ENCODE_RANGE) {
            _wb[_bp] -= DLT_EVENT_ENCODE_RANGE;
          }
        }
      }

      unsigned shift   = _payload_sizes[ev_code]+1; //Add one byte for event code
      if (shift > _length_raw) {
        FAIL_CORRUPT("Event byte offset exceeds raw data length");
        return false;
      }
      switch(ev_code) { //Determine the event code
        case Event::GAME_START:  success = _parseGameStart();  break;
        case Event::PRE_FRAME:   success = _parsePreFrame();   break;
        case Event::POST_FRAME:  success = _parsePostFrame();  break;
        case Event::ITEM_UPDATE: success = _parseItemUpdate(); break;
        case Event::FRAME_START: success = _parseFrameStart(); break;
        case Event::BOOKEND:     success = _parseBookend();    break;
        case Event::GAME_END:    success = true;               break;
        case Event::SPLIT_MSG:   success = true;               break;
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

    //Get Slippi version
    _slippi_maj = uint8_t(_rb[_bp+0x1]); //Major version
    _slippi_min = uint8_t(_rb[_bp+0x2]); //Minor version
    _slippi_rev = uint8_t(_rb[_bp+0x3]); //Build version (4th char unused)
    _is_encoded = uint8_t(_rb[_bp+0x4]); //Whether the file is encoded
    if (_slippi_maj == 0) {
      FAIL("Replays from Slippi 0.x.x are not supported");
      return false;
    }

    //Flip encoding status for output buffer
    _wb[_bp+0x4] ^= 1;

    //Print slippi version
    std::stringstream ss;
    ss << +_slippi_maj << "." << +_slippi_min << "." << +_slippi_rev;
    DOUT1("Slippi Version: " << ss.str() << ", " << (_is_encoded ? "encoded" : "raw") << std::endl);

    //Get starting RNG state
    _rng       = readBE4U(&_rb[_bp+0x13D]);
    _rng_start = _rng;
    DOUT1("Starting RNG: " << _rng << std::endl);

    return true;
  }

  bool Compressor::_parseItemUpdate() {
    //Encodings so far
      //0x00 - 0x00 | Command Byte         | Encoded with offset -0x36
      //0x01 - 0x04 | Frame Number         | Predictive encoding (last frame + 1)
      //0x05 - 0x06 | Type ID              | XORed with last item-slot value
      //0x07 - 0x07 | State                | XORed with last item-slot value
      //0x08 - 0x0B | Facing Direction     | XORed with last item-slot value
      //0x0C - 0x0F | X Velocity           | Value retrieved from float map
      //0x10 - 0x13 | Y Velocity           | Value retrieved from float map
      //0x14 - 0x17 | X Position           | Predictive encoding (2-frame delta)
      //0x18 - 0x1B | Y Position           | Predictive encoding (2-frame delta)
      //0x1C - 0x1D | Damage Taken         | XORed with last item-slot value
      //0x1E - 0x21 | Expiration Timer     | Predictive encoding (2-frame delta)
      //0x22 - 0x25 | Spawn ID             | No encoding
      //0x26 - 0x29 | Misc Values          | XORed with last item-slot value
      //0x2A - 0x2A | Owner                | XORed with last item-slot value

    DOUT2("  Compressing item event at byte " << +_bp << std::endl);
    //Encode frame number, predicting the last frame number + 1
    predictFrame(readBE4S(&_rb[_bp+0x1]), 0x01);

    //Get a storage slot for the item
    uint8_t slot = readBE4U(&_rb[_bp+0x22]) % 16;

    //Add item velocities to float map
    buildFloatMap(0x0C);
    buildFloatMap(0x10);

    //Predict item positions based on acceleration
    predictVelocItem(slot,0x14);
    predictVelocItem(slot,0x18);
    // predictAccelItem(slot,0x14);
    // predictAccelItem(slot,0x18);

    //Predict item expiration based on acceleration
    predictVelocItem(slot,0x1E);

    //XOR all of the remaining data for the item
    xorEncodeRange(0x05,0x0C,_x_item[slot]);
    xorEncodeRange(0x1C,0x1E,_x_item[slot]);
    xorEncodeRange(0x26,0x2B,_x_item[slot]);

    if (_debug == 0) { return true; }

    return true;
  }

  bool Compressor::_parseFrameStart() {
    //Encodings so far
      //0x00 - 0x00 | Command Byte         | Encoded with offset -0x36
      //0x01 - 0x04 | Frame Number         | Predictive encoding (last frame + 1), second bit flipped if raw RNG
      //0x05 - 0x08 | RNG Seed             | Predictive encoding (# of RNG rolls, or raw)

    //Encode frame number, predicting the last frame number +1
    predictFrame(readBE4S(&_rb[_bp+0x1]), 0x01);

    //Predict RNG value from real (not delta encoded) frame number
    predictRNG(0x01,0x05);

    return true;
  }

  bool Compressor::_parseBookend() {
    //Encodings so far
      //0x00 - 0x00 | Command Byte         | Encoded with offset -0x36
      //0x01 - 0x04 | Frame Number         | Predictive encoding (last frame + 1)
      //0x05 - 0x08 | Rollback Frame       | Predictive encoding (last frame + 1)

    //Encode actual frame number, predicting and updating laststartframe
    predictFrame(readBE4S(&_rb[_bp+0x1]), 0x01, true);

    if ( (100*_slippi_maj + _slippi_min) >= 307) {
      //Encode rollback frame number, predicting laststartframe without update
      predictFrame(readBE4S(&_rb[_bp+0x5]), 0x5, false);
    }

    return true;
  }

  bool Compressor::_parsePreFrame() {
    //Encodings so far
      //0x00 - 0x00 | Command Byte         | Encoded with offset -0x36
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
      //0x31 - 0x32 | Physical Buttons     | No encoding (already sparse)
      //0x33 - 0x36 | Physical L           | Value retrieved from float map
      //0x37 - 0x3A | Physical R           | Value retrieved from float map
      //0x3B - 0x3B | UCF X analog         | No encoding
      //0x3C - 0x3F | Damage               | No encoding (already sparse)

    DOUT2("  Compressing pre frame event at byte " << +_bp << std::endl);

    //Get player index
    uint8_t p = uint8_t(_rb[_bp+0x5])+4*uint8_t(_rb[_bp+0x6]); //Includes follower
    if (p > 7) {
      FAIL_CORRUPT("Invalid player index " << +p);
      return false;
    }

    //Encode frame number, predicting the last frame number +1
    predictFrame(readBE4S(&_rb[_bp+0x01]), 0x01);

    //Predict RNG value from real (not delta encoded) frame number
    predictRNG(0x01,0x07);

    //Carry over action state from last post-frame
    writeBE2U(readBE2U(&_rb[_bp+0x0B]) ^ readBE2U(&_x_post_frame[p][0x08]),&_wb[_bp+0x0B]);
    memcpy(&_x_pre_frame[p][0x0B],_is_encoded ? &_wb[_bp+0x0B] : &_rb[_bp+0x0B],2);

    //Carry over x position from last post-frame
    writeBE4U(readBE4U(&_rb[_bp+0x0D]) ^ readBE4U(&_x_post_frame[p][0x0A]),&_wb[_bp+0x0D]);
    memcpy(&_x_pre_frame[p][0x0D],_is_encoded ? &_wb[_bp+0x0D] : &_rb[_bp+0x0D],4);

    //Carry over y position from last post-frame
    writeBE4U(readBE4U(&_rb[_bp+0x11]) ^ readBE4U(&_x_post_frame[p][0x0E]),&_wb[_bp+0x11]);
    memcpy(&_x_pre_frame[p][0x11],_is_encoded ? &_wb[_bp+0x11] : &_rb[_bp+0x11],4);

    //Carry over facing direction from last post-frame
    writeBE4U(readBE4U(&_rb[_bp+0x15]) ^ readBE4U(&_x_post_frame[p][0x12]),&_wb[_bp+0x15]);
    memcpy(&_x_pre_frame[p][0x15],_is_encoded ? &_wb[_bp+0x15] : &_rb[_bp+0x15],4);

    // std::cout << "Xdelta: " << (readBE2U(&_rb[_bp+0x0B]) ^ readBE2U(&_x_post_frame[p][0x08])) << std::endl;

    //Map out stray float values to integers
    buildFloatMap(0x19);
    buildFloatMap(0x1D);
    buildFloatMap(0x21);
    buildFloatMap(0x25);
    buildFloatMap(0x29);
    buildFloatMap(0x33);
    buildFloatMap(0x37);

    if (_debug == 0) { return true; }
    //Below here is still in testing mode

    // xorEncodeRange(0x3B,0x3C,_x_pre_frame[p]);
    // xorEncodeRange(0x3C,0x40,_x_pre_frame[p]);

    // analogFloatToInt(0x19,560);
    // analogFloatToInt(0x1D,560);
    // analogFloatToInt(0x21,560);
    // analogFloatToInt(0x25,560);
    // analogFloatToInt(0x29,140);
    // analogFloatToInt(0x33,140);
    // analogFloatToInt(0x37,140);

    return true;
  }

  bool Compressor::_parsePostFrame() {
    //Encodings so far
      //0x00 - 0x00 | Command Byte         | Encoded with offset -0x36
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

    DOUT2("  Compressing post frame event at byte " << +_bp << std::endl);
    union { float f; uint32_t u; } float_true, float_pred, float_temp;
    int32_t fnum = readBE4S(&_rb[_bp+0x1]);
    int32_t f    = fnum-LOAD_FRAME;

    //Get player index
    uint8_t p = uint8_t(_rb[_bp+0x5])+4*uint8_t(_rb[_bp+0x6]); //Includes follower
    if (p > 7) {
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

    // float pos1 = readBE4F(&_x_post_frame[  p][0x0A]);
    // float pos2 = readBE4F(&_x_post_frame_2[p][0x0A]);
    // float pos3 = readBE4F(&_x_post_frame_3[p][0x0A]);
    // float vel1 = pos1-pos2;
    // float vel2 = pos2-pos3;
    // float acc1 = vel1-vel2;
    // if (p == 0) {
    //   std::cout
    //     << "XTru:   " << readBE4F(&_rb[_bp+0x0A])
    //     << "     XPos: " << pos1
    //     << "     XVel: " << vel1
    //     << "     XAcc: " << acc1
    //     << std::endl;
    // }

    //Map action states
    // if (_debug > 0) {
    //   uint16_t as = readBE2U(&_rb[_bp+0x08]);
    //   if (as < Action::__LAST) {
    //     uint16_t ms = _is_encoded ? ACTION_REV[as] : ACTION_MAP[as];
    //     writeBE2U(ms,&_wb[_bp+0x08]);
    //   }
    // }

    //Predict x position based on velocity and acceleration
    predictAccelPost(p,0x0A);
    //Predict y position based on velocity and acceleration
    predictAccelPost(p,0x0E);

    //Copy action state to post-frame
    memcpy(&_x_post_frame[p][0x08],_is_encoded ? &_wb[_bp+0x08] : &_rb[_bp+0x08],2);
    //Copy x position to post-frame
    memcpy(&_x_post_frame[p][0x0A],_is_encoded ? &_wb[_bp+0x0A] : &_rb[_bp+0x0A],4);
    //Copy y position to post-frame
    memcpy(&_x_post_frame[p][0x0E],_is_encoded ? &_wb[_bp+0x0E] : &_rb[_bp+0x0E],4);
    //Copy facing direction to post-frame
    memcpy(&_x_post_frame[p][0x12],_is_encoded ? &_wb[_bp+0x12] : &_rb[_bp+0x12],4);
    //Copy damage to post-frame
    memcpy(&_x_post_frame[p][0x16],_is_encoded ? &_wb[_bp+0x16] : &_rb[_bp+0x16],4);

    // uint16_t as = readBE2U(&_rb[_bp+0x08]);
    // if (_as_counter.find(as) == _as_counter.end()) {
    //   _as_counter[as] = 0;
    // }
    // ++_as_counter[as];

    //Predict shield decay as velocity
    predictVelocPost(p,0x1A);

    //Compress single byte values with XOR encoding
    xorEncodeRange(0x1E,0x22,_x_post_frame[p]);

    //Predict this frame's action state counter from the last 2 frames' counters
    predictVelocPost(p,0x22);

    //XOR encode state bit flags
    xorEncodeRange(0x26,0x2B,_x_post_frame[p]);

    //Predict this frame's hitstun counter from the last 2 frames' counters
    predictVelocPost(p,0x2B);

    if (_slippi_maj >= 3 && _slippi_min >= 5) {
      //Predict velocity of various speeds based on previous frames' positions
      predictVelocPost(p,0x35);
      predictVelocPost(p,0x39);
      predictVelocPost(p,0x3D);
      predictVelocPost(p,0x41);
      predictVelocPost(p,0x45);
    }

    if (_debug == 0) { return true; }
    //Below here is still in testing mode

    return true;
  }

  void Compressor::save(const char* outfilename) {
    std::ofstream ofile2;
    ofile2.open(outfilename, std::ios::binary | std::ios::out);
    if (_debug >= 2) {
      printBuffers();
    }
    ofile2.write(_wb,sizeof(char)*_file_size);
    ofile2.close();
  }

}
