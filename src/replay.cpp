#include "replay.h"

//JSON Output shortcuts
#define JFLT(i,k,n) SPACE[ILEV*(i)] << "\"" << (k) << "\" : " << float(n)
#define JINT(i,k,n) SPACE[ILEV*(i)] << "\"" << (k) << "\" : " << int32_t(n)
#define JUIN(i,k,n) SPACE[ILEV*(i)] << "\"" << (k) << "\" : " << uint32_t(n)
#define JSTR(i,k,s) SPACE[ILEV*(i)] << "\"" << (k) << "\" : \"" << (s) << "\""
//Logic for outputting a line only if it changed since last frame (or if we're in full output mode)
#define CHANGED(field) (not delta) || (f == 0) || (s.player[p].frame[f].field != s.player[p].frame[f-1].field)
#define ICHANGED(field) (not delta) || (f == 0) || (s.item[i].frame[f].field != s.item[i].frame[f-1].field)
//Logic for outputting a comma or not depending on whether we're the first element in a JSON object
#define JEND(a) ((a++ == 0) ? "\n" : ",\n")

namespace slip {

void SlippiReplay::setFrames(int32_t max_frames) {
  this->last_frame  = max_frames;
  this->frame_count = max_frames-this->first_frame;
  for(unsigned i = 0; i < 4; ++i) {
    if (this->player[i].player_type != 3) {
      this->player[i].frame = new SlippiFrame[this->frame_count];
      if (this->player[i].ext_char_id == CharExt::CLIMBER) { //Extra player for Ice Climbers
        this->player[i+4].frame = new SlippiFrame[this->frame_count];
      }
    }
  }
}

void SlippiReplay::cleanup() {
  for(unsigned i = 0; i < 4; ++i) {
    if (this->player[i].player_type != 3) {
      delete [] this->player[i].frame;
      if (this->player[i].ext_char_id == CharExt::CLIMBER) { //Extra player for Ice Climbers
        delete [] this->player[i+4].frame;
      }
    }
  }
  for(unsigned i = 0; i < MAX_ITEMS; ++i) {
    if (this->item[i].frame != nullptr) {
      delete [] this->item[i].frame;
    }
  }
}

std::string SlippiReplay::replayAsJson(bool delta) {
  SlippiReplay s = (*this);

  uint8_t _slippi_maj = (s.slippi_version_raw >> 24) & 0xff;
  uint8_t _slippi_min = (s.slippi_version_raw >> 16) & 0xff;
  uint8_t _slippi_rev = (s.slippi_version_raw >>  8) & 0xff;

  std::stringstream ss;
  ss << "{" << std::endl;

  ss << JSTR(0,"original_file" , escape_json(s.original_file))  << ",\n";
  ss << JSTR(0,"slippi_version", s.slippi_version)              << ",\n";
  ss << JSTR(0,"parser_version", s.parser_version)              << ",\n";
  ss << JUIN(0,"errors",         s.errors)                      << ",\n";
  ss << JSTR(0,"game_start_raw", s.game_start_raw)              << ",\n";
  ss << JSTR(0,"start_time"    , s.start_time)                  << ",\n";
  ss << JINT(0,"frame_count"   , s.frame_count)                 << ",\n";
  ss << JSTR(0,"played_on"     , s.played_on)                   << ",\n";
  ss << JINT(0,"winner_id"     , s.winner_id)                   << ",\n";
  ss << JUIN(0,"timer"         , s.timer)                       << ",\n";
  ss << JUIN(0,"teams"         , s.teams)                       << ",\n";
  ss << JUIN(0,"stage"         , s.stage)                       << ",\n";
  ss << JUIN(0,"seed"          , s.seed)                        << ",\n";
  ss << JINT(0,"items_on"      , s.items_on)                    << ",\n";
  ss << JUIN(0,"end_type"      , s.end_type)                    << ",\n";
  ss << JINT(0,"lras"          , s.lras)                        << ",\n";
  if(MIN_VERSION(1,5,0)) {
    ss << JUIN(0,"pal"           , s.pal)            << ",\n";
  }
  if(MIN_VERSION(2,0,0)) {
    ss << JUIN(0,"frozen_stadium", s.frozen_stadium) << ",\n";
  }
  if(MIN_VERSION(3,7,0)) {
    ss << JUIN(0,"scene_min"     , s.scene_min)      << ",\n";
    ss << JUIN(0,"scene_maj"     , s.scene_maj)      << ",\n";
  }
  if(MIN_VERSION(3,12,0)) {
    ss << JUIN(0,"language"      , s.language)       << ",\n";
  }
  if(MIN_VERSION(3,14,0)) {
    ss << JSTR(0,"match_id"          , s.match_id)          << ",\n";
    ss << JUIN(0,"game_number"       , s.game_number)       << ",\n";
    ss << JUIN(0,"tiebreaker_number" , s.tiebreaker_number) << ",\n";
  }
  ss << JINT(0,"first_frame"   , s.first_frame)    << ",\n";
  ss << JINT(0,"last_frame"    , s.last_frame)     << ",\n";
  ss << JUIN(0,"sudden_death"  , s.sudden_death)   << ",\n";
  ss << JINT(0,"sd_score"      , s.sd_score)       << ",\n";
  ss << JUIN(0,"timer_behav"   , s.timer_behav)   << ",\n";
  ss << JUIN(0,"ui_chars"      , s.ui_chars)      << ",\n";
  ss << JUIN(0,"game_mode"     , s.game_mode)     << ",\n";
  ss << JUIN(0,"friendly_fire" , s.friendly_fire) << ",\n";
  ss << JUIN(0,"demo_mode"     , s.demo_mode)     << ",\n";
  ss << JUIN(0,"classic_adv"   , s.classic_adv)   << ",\n";
  ss << JUIN(0,"hrc_event"     , s.hrc_event)     << ",\n";
  ss << JUIN(0,"allstar_wait1" , s.allstar_wait1) << ",\n";
  ss << JUIN(0,"allstar_wait2" , s.allstar_wait2) << ",\n";
  ss << JUIN(0,"allstar_game1" , s.allstar_game1) << ",\n";
  ss << JUIN(0,"allstar_game2" , s.allstar_game2) << ",\n";
  ss << JUIN(0,"single_button" , s.single_button) << ",\n";
  ss << JUIN(0,"pause_timer"   , s.pause_timer)   << ",\n";
  ss << JUIN(0,"pause_nohud"   , s.pause_nohud)   << ",\n";
  ss << JUIN(0,"pause_lras"    , s.pause_lras)    << ",\n";
  ss << JUIN(0,"pause_off"     , s.pause_off)     << ",\n";
  ss << JUIN(0,"pause_zretry"  , s.pause_zretry)  << ",\n";
  ss << JUIN(0,"pause_analog"  , s.pause_analog)  << ",\n";
  ss << JUIN(0,"pause_score"   , s.pause_score)   << ",\n";
  ss << JUIN(0,"items1"        , s.items1)        << ",\n";
  ss << JUIN(0,"items2"        , s.items2)        << ",\n";
  ss << JUIN(0,"items3"        , s.items3)        << ",\n";
  ss << JUIN(0,"items4"        , s.items4)        << ",\n";
  ss << JUIN(0,"items5"        , s.items5)        << ",\n";
  ss << "\"metadata\" : " << s.metadata << "\n},\n";

  ss << "\"players\" : [\n";
  for(unsigned p = 0; p < 8; ++p) {
    unsigned pp = (p % 4);
    if(p > 3 && s.player[pp].ext_char_id != CharExt::CLIMBER) { //If we're not Ice climbers
      if (p == 7) {
        ss << SPACE[ILEV] << "{}\n";
      } else {
        ss << SPACE[ILEV] << "{},\n";
      }
      continue;
    }

    ss << SPACE[ILEV] << "{\n";
    ss << JUIN(1,"player_id"   ,pp)                                   << ",\n";
    ss << JUIN(1,"is_follower" ,p > 3)                                << ",\n";
    ss << JUIN(1,"ext_char_id" ,s.player[pp].ext_char_id)             << ",\n";
    ss << JUIN(1,"player_type" ,s.player[pp].player_type)             << ",\n";
    ss << JUIN(1,"start_stocks",s.player[pp].start_stocks)            << ",\n";
    ss << JUIN(1,"end_stocks"  ,s.player[pp].end_stocks)              << ",\n";
    ss << JUIN(1,"color"       ,s.player[pp].color)                   << ",\n";
    ss << JUIN(1,"team_id"     ,s.player[pp].team_id)                 << ",\n";
    ss << JUIN(1,"cpu_level"   ,s.player[pp].cpu_level)               << ",\n";
    ss << JUIN(1,"dash_back"   ,s.player[pp].dash_back)               << ",\n";
    ss << JUIN(1,"shield_drop" ,s.player[pp].shield_drop)             << ",\n";
    ss << JUIN(1,"shade"       ,s.player[pp].shade)                   << ",\n";
    ss << JUIN(1,"handicap"    ,s.player[pp].handicap)                << ",\n";
    ss << JUIN(1,"offense"     ,s.player[pp].offense)                 << ",\n";
    ss << JUIN(1,"defense"     ,s.player[pp].defense)                 << ",\n";
    ss << JUIN(1,"scale"       ,s.player[pp].scale)                   << ",\n";
    ss << JUIN(1,"stamina"     ,s.player[pp].stamina)                 << ",\n";
    ss << JUIN(1,"silent"      ,s.player[pp].silent)                  << ",\n";
    ss << JUIN(1,"low_gravity" ,s.player[pp].low_gravity)             << ",\n";
    ss << JUIN(1,"invisible"   ,s.player[pp].invisible)               << ",\n";
    ss << JUIN(1,"black_stock" ,s.player[pp].black_stock)             << ",\n";
    ss << JUIN(1,"metal"       ,s.player[pp].metal)                   << ",\n";
    ss << JUIN(1,"warp_in"     ,s.player[pp].warp_in)                 << ",\n";
    ss << JUIN(1,"rumble"      ,s.player[pp].rumble)                  << ",\n";
    ss << JSTR(1,"tag_css"     ,escape_json(s.player[pp].tag_css))    << ",\n";
    ss << JSTR(1,"tag_code"    ,escape_json(s.player[pp].tag_code))   << ",\n";
    ss << JSTR(1,"tag_player"  ,escape_json(s.player[pp].tag))        << ",\n";
    ss << JSTR(1,"disp_name"   ,escape_json(s.player[pp].disp_name))  << ",\n";
    ss << JSTR(1,"slippi_uid"  ,escape_json(s.player[pp].slippi_uid)) << ",\n";

    if (s.player[p].player_type == 3) {
      ss << SPACE[ILEV] << "\"frames\" : []\n";
    } else {
      ss << SPACE[ILEV] << "\"frames\" : [\n";
      for(unsigned f = 0; f < s.frame_count; ++f) {
        ss << SPACE[ILEV*2] << "{";

        int a = 0; //True for only the first thing output per line
        if (CHANGED(follower))
          ss << JEND(a) << JUIN(2,"follower"      ,s.player[p].frame[f].follower);
        if (CHANGED(seed))
          ss << JEND(a) << JUIN(2,"seed"          ,s.player[p].frame[f].seed);
        if (CHANGED(action_pre))
          ss << JEND(a) << JUIN(2,"action_pre"    ,s.player[p].frame[f].action_pre);
        if (CHANGED(pos_x_pre))
          ss << JEND(a) << JFLT(2,"pos_x_pre"     ,s.player[p].frame[f].pos_x_pre);
        if (CHANGED(pos_y_pre))
          ss << JEND(a) << JFLT(2,"pos_y_pre"     ,s.player[p].frame[f].pos_y_pre);
        if (CHANGED(face_dir_pre))
          ss << JEND(a) << JFLT(2,"face_dir_pre"  ,s.player[p].frame[f].face_dir_pre);
        if (CHANGED(joy_x))
          ss << JEND(a) << JFLT(2,"joy_x"         ,s.player[p].frame[f].joy_x);
        if (CHANGED(joy_y))
          ss << JEND(a) << JFLT(2,"joy_y"         ,s.player[p].frame[f].joy_y);
        if (CHANGED(c_x))
          ss << JEND(a) << JFLT(2,"c_x"           ,s.player[p].frame[f].c_x);
        if (CHANGED(c_y))
          ss << JEND(a) << JFLT(2,"c_y"           ,s.player[p].frame[f].c_y);
        if (CHANGED(trigger))
          ss << JEND(a) << JFLT(2,"trigger"       ,s.player[p].frame[f].trigger);
        if (CHANGED(buttons))
          ss << JEND(a) << JUIN(2,"buttons"       ,s.player[p].frame[f].buttons);
        if (CHANGED(phys_l))
          ss << JEND(a) << JFLT(2,"phys_l"        ,s.player[p].frame[f].phys_l);
        if (CHANGED(phys_r))
          ss << JEND(a) << JFLT(2,"phys_r"        ,s.player[p].frame[f].phys_r);
        if (CHANGED(ucf_x))
          ss << JEND(a) << JUIN(2,"ucf_x"         ,s.player[p].frame[f].ucf_x);
        if (CHANGED(percent_pre))
          ss << JEND(a) << JFLT(2,"percent_pre"   ,s.player[p].frame[f].percent_pre);
        if (CHANGED(char_id))
          ss << JEND(a) << JUIN(2,"char_id"       ,s.player[p].frame[f].char_id);
        if (CHANGED(action_post))
          ss << JEND(a) << JUIN(2,"action_post"   ,s.player[p].frame[f].action_post);
        if (CHANGED(pos_x_post))
          ss << JEND(a) << JFLT(2,"pos_x_post"    ,s.player[p].frame[f].pos_x_post);
        if (CHANGED(pos_y_post))
          ss << JEND(a) << JFLT(2,"pos_y_post"    ,s.player[p].frame[f].pos_y_post);
        if (CHANGED(face_dir_post))
          ss << JEND(a) << JFLT(2,"face_dir_post" ,s.player[p].frame[f].face_dir_post);
        if (CHANGED(percent_post))
          ss << JEND(a) << JFLT(2,"percent_post"  ,s.player[p].frame[f].percent_post);
        if (CHANGED(shield))
          ss << JEND(a) << JFLT(2,"shield"        ,s.player[p].frame[f].shield);
        if (CHANGED(hit_with))
          ss << JEND(a) << JUIN(2,"hit_with"      ,s.player[p].frame[f].hit_with);
        if (CHANGED(combo))
          ss << JEND(a) << JUIN(2,"combo"         ,s.player[p].frame[f].combo);
        if (CHANGED(hurt_by))
          ss << JEND(a) << JUIN(2,"hurt_by"       ,s.player[p].frame[f].hurt_by);
        if (CHANGED(stocks))
          ss << JEND(a) << JUIN(2,"stocks"        ,s.player[p].frame[f].stocks);
        if (CHANGED(action_fc))
          ss << JEND(a) << JFLT(2,"action_fc"     ,s.player[p].frame[f].action_fc);

        if(MIN_VERSION(2,0,0)) {
          if (CHANGED(flags_1))
            ss << JEND(a) << JUIN(2,"flags_1"       ,s.player[p].frame[f].flags_1);
          if (CHANGED(flags_2))
            ss << JEND(a) << JUIN(2,"flags_2"       ,s.player[p].frame[f].flags_2);
          if (CHANGED(flags_3))
            ss << JEND(a) << JUIN(2,"flags_3"       ,s.player[p].frame[f].flags_3);
          if (CHANGED(flags_4))
            ss << JEND(a) << JUIN(2,"flags_4"       ,s.player[p].frame[f].flags_4);
          if (CHANGED(flags_5))
            ss << JEND(a) << JUIN(2,"flags_5"       ,s.player[p].frame[f].flags_5);
          if (CHANGED(hitstun))
            ss << JEND(a) << JUIN(2,"hitstun"       ,s.player[p].frame[f].hitstun);
          if (CHANGED(airborne))
            ss << JEND(a) << JUIN(2,"airborne"      ,s.player[p].frame[f].airborne);
          if (CHANGED(ground_id))
            ss << JEND(a) << JUIN(2,"ground_id"     ,s.player[p].frame[f].ground_id);
          if (CHANGED(jumps))
            ss << JEND(a) << JUIN(2,"jumps"         ,s.player[p].frame[f].jumps);
          if (CHANGED(l_cancel))
            ss << JEND(a) << JUIN(2,"l_cancel"      ,s.player[p].frame[f].l_cancel);
          if (CHANGED(alive))
            ss << JEND(a) << JINT(2,"alive"         ,s.player[p].frame[f].alive);
        }

        if(MIN_VERSION(2,1,0)) {
          if (CHANGED(hurtbox))
            ss << JEND(a) << JUIN(2,"hurtbox"       ,s.player[p].frame[f].hurtbox);
        }

        if(MIN_VERSION(3,5,0)) {
          if (CHANGED(self_air_x))
            ss << JEND(a) << JFLT(2,"self_air_x"    ,s.player[p].frame[f].self_air_x);
          if (CHANGED(self_air_y))
            ss << JEND(a) << JFLT(2,"self_air_y"    ,s.player[p].frame[f].self_air_y);
          if (CHANGED(attack_x))
            ss << JEND(a) << JFLT(2,"attack_x"      ,s.player[p].frame[f].attack_x);
          if (CHANGED(attack_y))
            ss << JEND(a) << JFLT(2,"attack_y"      ,s.player[p].frame[f].attack_y);
          if (CHANGED(self_grd_x))
            ss << JEND(a) << JFLT(2,"self_grd_x"    ,s.player[p].frame[f].self_grd_x);
        }

        if(MIN_VERSION(3,8,0)) {
          if (CHANGED(hitlag))
            ss << JEND(a) << JFLT(2,"hitlag"        ,s.player[p].frame[f].hitlag);
        }

        if(MIN_VERSION(3,11,0)) {
          if (CHANGED(anim_index))
            ss << JEND(a) << JUIN(2,"anim_index"    ,s.player[p].frame[f].anim_index);
        }

        if (f < s.frame_count-1) {
          ss << "\n" << SPACE[ILEV*2] << "},\n";
        } else {
          ss << "\n" << SPACE[ILEV*2] << "}\n";
        }
      }
      ss << SPACE[ILEV*2] << "]\n";
    }
    if (p == 7) {
      ss << SPACE[ILEV] << "}\n";
    } else {
      ss << SPACE[ILEV] << "},\n";
    }
  }
  if (MAX_VERSION(3,0,0)) {
    ss << "]\n";
  } else {
    ss << "],\n";
    ss << "\"items\" : [\n";
    for(unsigned i = 0; i < MAX_ITEMS; ++i) {
      if (s.item[i].spawn_id > MAX_ITEMS) {
        break;
      }
      ss << SPACE[ILEV] << "{\n";
      ss << JUIN(1,"spawn_id" ,s.item[i].spawn_id)           << ",\n";
      ss << JUIN(1,"item_type",s.item[i].type)               << ",\n";
      ss << SPACE[ILEV] << "\"frames\" : [\n";

      for(unsigned f = 0; f < s.item[i].num_frames; ++f) {
        ss << SPACE[ILEV*2] << "{";
        int a = 0; //True for only the first thing output per line

        ss << JEND(a) << JUIN(2,"frame"      ,s.item[i].frame[f].frame);
        if (ICHANGED(state))
          ss << JEND(a) << JUIN(2,"state"      ,s.item[i].frame[f].state);
        if (ICHANGED(face_dir))
          ss << JEND(a) << JFLT(2,"face_dir"   ,s.item[i].frame[f].face_dir);
        if (ICHANGED(xvel))
          ss << JEND(a) << JFLT(2,"xvel"       ,s.item[i].frame[f].xvel);
        if (ICHANGED(yvel))
          ss << JEND(a) << JFLT(2,"yvel"       ,s.item[i].frame[f].yvel);
        if (ICHANGED(xpos))
          ss << JEND(a) << JFLT(2,"xpos"       ,s.item[i].frame[f].xpos);
        if (ICHANGED(ypos))
          ss << JEND(a) << JFLT(2,"ypos"       ,s.item[i].frame[f].ypos);
        if (ICHANGED(damage))
          ss << JEND(a) << JUIN(2,"damage"     ,s.item[i].frame[f].damage);
        if (ICHANGED(expire))
          ss << JEND(a) << JFLT(2,"expire"     ,s.item[i].frame[f].expire);

        if(MIN_VERSION(3,2,0)) {
          if (ICHANGED(flags_1))
            ss << JEND(a) << JUIN(2,"flags_1"     ,s.item[i].frame[f].flags_1);
          if (ICHANGED(flags_2))
            ss << JEND(a) << JUIN(2,"flags_2"     ,s.item[i].frame[f].flags_2);
          if (ICHANGED(flags_3))
            ss << JEND(a) << JUIN(2,"flags_3"     ,s.item[i].frame[f].flags_3);
          if (ICHANGED(flags_4))
            ss << JEND(a) << JUIN(2,"flags_4"     ,s.item[i].frame[f].flags_4);
          if(MIN_VERSION(3,6,0)) {
            if (ICHANGED(owner))
              ss << JEND(a) << JINT(2,"owner"      ,s.item[i].frame[f].owner);
          }
        }

        if (f+1 == s.item[i].num_frames) {
          ss << "\n" << SPACE[ILEV*2] << "}\n";
        } else {
          ss << "\n" << SPACE[ILEV*2] << "},\n";
        }

      }

      if (s.item[i+1].spawn_id > MAX_ITEMS) {
        ss << SPACE[ILEV] << "]}\n";
      } else {
        ss << SPACE[ILEV] << "]},\n";
      }
    }
    ss << "]\n";
  }

  ss << "}" << std::endl;
  return ss.str();
}

}
