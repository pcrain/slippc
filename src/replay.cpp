#include "replay.h"

const std::string SPACE[10] = {
  "",
  " ",
  "  ",
  "   ",
  "    ",
  "     ",
  "      ",
  "       ",
  "        ",
  "         ",
};

#define JFLT(i,k,n) SPACE[(i)] << "\"" << (k) << "\" : " << float(n)
#define JINT(i,k,n) SPACE[(i)] << "\"" << (k) << "\" : " << int32_t(n)
#define JUIN(i,k,n) SPACE[(i)] << "\"" << (k) << "\" : " << uint32_t(n)
#define JSTR(i,k,s) SPACE[(i)] << "\"" << (k) << "\" : \"" << (s) << "\""
#define CHANGED(field) (not delta) || (f == 0) || (s.player[p].frame[f].field != s.player[p].frame[f-1].field)

namespace slip {

void setFrames(SlippiReplay &s, int32_t nframes) {
  s.frame_count = nframes;
  for(unsigned i = 0; i < 4; ++i) {
    if (s.player[i].player_type != 3) {
      s.player[i].frame = new SlippiFrame[nframes+123];
      if (s.player[i].ext_char_id == 0x0E) { //Ice climbers
        s.player[i+4].frame = new SlippiFrame[nframes+123];
      }
    }
  }
}

void cleanup(SlippiReplay s) {
  for(unsigned i = 0; i < 4; ++i) {
    delete s.player[i].frame;
  }
}

std::string replayAsJson(SlippiReplay s, bool delta) {
  int32_t total_frames = s.frame_count + 123 + 1;
  std::stringstream ss;
  ss << "{" << std::endl;

  ss << JSTR(0,"slippi_version", s.slippi_version) << ",\n";
  ss << JSTR(0,"parser_version", "0.0.1")          << ",\n";
  ss << JSTR(0,"game_start_raw", s.game_start_raw) << ",\n";
  ss << JUIN(0,"teams"         , s.teams)          << ",\n";
  ss << JUIN(0,"stage"         , s.stage)          << ",\n";
  ss << JUIN(0,"seed"          , s.seed)           << ",\n";
  ss << JUIN(0,"pal"           , s.pal)            << ",\n";
  ss << JUIN(0,"frozen"        , s.frozen)         << ",\n";
  ss << JUIN(0,"end_type"      , s.end_type)       << ",\n";
  ss << JINT(0,"lras"          , s.lras)           << ",\n";
  ss << JINT(0,"first_frame"   , -123)             << ",\n";
  ss << JINT(0,"last_frame"    , s.frame_count)  << ",\n";
  ss << "\"metadata\" : " << s.metadata << "\n},\n";

  ss << "\"players\" : [\n";
  for(unsigned p = 0; p < 8; ++p) {
    unsigned pp = (p % 4);
    if(p > 3 && s.player[pp].ext_char_id != 0x0E) {  //If we're not Ice climbers
      if (p == 7) {
        ss << "  {}\n";
      } else {
        ss << "  {},\n";
      }
      continue;
    }

    ss << "  {\n";
    ss << JUIN(4,"player_id"   ,pp)                            << ",\n";
    ss << JUIN(4,"is_follower" ,p > 3)                         << ",\n";
    ss << JUIN(4,"ext_char_id" ,s.player[pp].ext_char_id)      << ",\n";
    ss << JUIN(4,"player_type" ,s.player[pp].player_type)      << ",\n";
    ss << JUIN(4,"start_stocks",s.player[pp].start_stocks)     << ",\n";
    ss << JUIN(4,"color"       ,s.player[pp].color)            << ",\n";
    ss << JUIN(4,"team_id"     ,s.player[pp].team_id)          << ",\n";
    ss << JUIN(4,"dash_back"   ,s.player[pp].dash_back)        << ",\n";
    ss << JUIN(4,"shield_drop" ,s.player[pp].shield_drop)      << ",\n";
    ss << JSTR(4,"tag"         ,escape_json(s.player[pp].tag)) << ",\n";

    if (s.player[p].player_type == 3) {
      ss << "    \"frames\" : []\n";
    } else {
      ss << "    \"frames\" : [\n";
      for(unsigned f = 0; f < total_frames; ++f) {
        ss << "      {\n";

        ss << JINT(6,"frame"           ,int(f)-123)                        << ",\n";
        if (CHANGED(follower))
          ss << JUIN(6,"follower"      ,s.player[p].frame[f].follower)     << ",\n";
        if (CHANGED(seed))
          ss << JUIN(6,"seed"          ,s.player[p].frame[f].seed)         << ",\n";
        if (CHANGED(action_pre))
          ss << JUIN(6,"action_pre"    ,s.player[p].frame[f].action_pre)   << ",\n";
        if (CHANGED(pos_x_pre))
          ss << JFLT(6,"pos_x_pre"     ,s.player[p].frame[f].pos_x_pre)    << ",\n";
        if (CHANGED(pos_y_pre))
          ss << JFLT(6,"pos_y_pre"     ,s.player[p].frame[f].pos_y_pre)    << ",\n";
        if (CHANGED(face_dir_pre))
          ss << JFLT(6,"face_dir_pre"  ,s.player[p].frame[f].face_dir_pre) << ",\n";
        if (CHANGED(joy_x))
          ss << JFLT(6,"joy_x"         ,s.player[p].frame[f].joy_x)        << ",\n";
        if (CHANGED(joy_y))
          ss << JFLT(6,"joy_y"         ,s.player[p].frame[f].joy_y)        << ",\n";
        if (CHANGED(c_x))
          ss << JFLT(6,"c_x"           ,s.player[p].frame[f].c_x)          << ",\n";
        if (CHANGED(c_y))
          ss << JFLT(6,"c_y"           ,s.player[p].frame[f].c_y)          << ",\n";
        if (CHANGED(trigger))
          ss << JFLT(6,"trigger"       ,s.player[p].frame[f].trigger)      << ",\n";
        if (CHANGED(buttons))
          ss << JUIN(6,"buttons"       ,s.player[p].frame[f].buttons)      << ",\n";
        if (CHANGED(phys_l))
          ss << JFLT(6,"phys_l"        ,s.player[p].frame[f].phys_l)       << ",\n";
        if (CHANGED(phys_r))
          ss << JFLT(6,"phys_r"        ,s.player[p].frame[f].phys_r)       << ",\n";
        if (CHANGED(ucf_x))
          ss << JUIN(6,"ucf_x"         ,s.player[p].frame[f].ucf_x)        << ",\n";
        if (CHANGED(percent_pre))
          ss << JFLT(6,"percent_pre"   ,s.player[p].frame[f].percent_pre)  << ",\n";
        if (CHANGED(char_id))
          ss << JUIN(6,"char_id"       ,s.player[p].frame[f].char_id)      << ",\n";
        if (CHANGED(action_post))
          ss << JUIN(6,"action_post"   ,s.player[p].frame[f].action_post)  << ",\n";
        if (CHANGED(pos_x_post))
          ss << JFLT(6,"pos_x_post"    ,s.player[p].frame[f].pos_x_post)   << ",\n";
        if (CHANGED(pos_y_post))
          ss << JFLT(6,"pos_y_post"    ,s.player[p].frame[f].pos_y_post)   << ",\n";
        if (CHANGED(face_dir_post))
          ss << JFLT(6,"face_dir_post" ,s.player[p].frame[f].face_dir_post)<< ",\n";
        if (CHANGED(percent_post))
          ss << JFLT(6,"percent_post"  ,s.player[p].frame[f].percent_post) << ",\n";
        if (CHANGED(shield))
          ss << JFLT(6,"shield"        ,s.player[p].frame[f].shield)       << ",\n";
        if (CHANGED(hit_with))
          ss << JUIN(6,"hit_with"      ,s.player[p].frame[f].hit_with)     << ",\n";
        if (CHANGED(combo))
          ss << JUIN(6,"combo"         ,s.player[p].frame[f].combo)        << ",\n";
        if (CHANGED(hurt_by))
          ss << JUIN(6,"hurt_by"       ,s.player[p].frame[f].hurt_by)      << ",\n";
        if (CHANGED(stocks))
          ss << JUIN(6,"stocks"        ,s.player[p].frame[f].stocks)       << ",\n";
        if (CHANGED(action_fc))
          ss << JFLT(6,"action_fc"     ,s.player[p].frame[f].action_fc)    << ",\n";
        if (CHANGED(flags_1))
          ss << JUIN(6,"flags_1"       ,s.player[p].frame[f].flags_1)      << ",\n";
        if (CHANGED(flags_2))
          ss << JUIN(6,"flags_2"       ,s.player[p].frame[f].flags_2)      << ",\n";
        if (CHANGED(flags_3))
          ss << JUIN(6,"flags_3"       ,s.player[p].frame[f].flags_3)      << ",\n";
        if (CHANGED(flags_4))
          ss << JUIN(6,"flags_4"       ,s.player[p].frame[f].flags_4)      << ",\n";
        if (CHANGED(flags_5))
          ss << JUIN(6,"flags_5"       ,s.player[p].frame[f].flags_5)      << ",\n";
        if (CHANGED(hitstun))
          ss << JUIN(6,"hitstun"       ,s.player[p].frame[f].hitstun)      << ",\n";
        if (CHANGED(airborne))
          ss << JUIN(6,"airborne"      ,s.player[p].frame[f].airborne)     << ",\n";
        if (CHANGED(ground_id))
          ss << JUIN(6,"ground_id"     ,s.player[p].frame[f].ground_id)    << ",\n";
        if (CHANGED(jumps))
          ss << JUIN(6,"jumps"         ,s.player[p].frame[f].jumps)        << ",\n";
        if (CHANGED(l_cancel))
          ss << JUIN(6,"l_cancel"      ,s.player[p].frame[f].l_cancel)     << ",\n";

        //Putting this last for a consistent non-comma end
        ss << JINT(6,"alive"         ,s.player[p].frame[f].alive)        << "\n";

        if (f < total_frames-1) {
          ss << "      },\n";
        } else {
          ss << "      }\n";
        }
      }
      ss << "    ]\n";
    }
    if (p == 7) {
      ss << "  }\n";
    } else {
      ss << "  },\n";
    }
  }
  ss << "]\n";

  ss << "}" << std::endl;
  return ss.str();
}

}
