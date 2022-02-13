#include "analysis.h"

//JSON Output shortcuts
#define JFLT(i,k,n) SPACE[ILEV*(i)] << "\"" << (k) << "\" : " << float(n)
#define JINT(i,k,n) SPACE[ILEV*(i)] << "\"" << (k) << "\" : " << int32_t(n)
#define JUIN(i,k,n) SPACE[ILEV*(i)] << "\"" << (k) << "\" : " << uint32_t(n)
#define JSTR(i,k,s) SPACE[ILEV*(i)] << "\"" << (k) << "\" : \"" << (s) << "\""
#define JEND(a) ((a++ == 0) ? "\n" : ",\n")

namespace slip {

std::string Analysis::asJson() {
  std::stringstream ss;
  ss << "{" << std::endl;

  ss << JSTR(0,"original_file",    escape_json(original_file))  << ",\n";
  ss << JSTR(0,"slippi_version",   slippi_version)              << ",\n";
  ss << JSTR(0,"parser_version",   parser_version)              << ",\n";
  ss << JSTR(0,"analyzer_version", analyzer_version)            << ",\n";
  ss << JUIN(0,"parse_errors",     parse_errors)                << ",\n";
  ss << JSTR(0,"game_time",        game_time)                   << ",\n";
  ss << JUIN(0,"stage_id",         stage_id)                    << ",\n";
  ss << JSTR(0,"stage_name",       stage_name)                  << ",\n";
  ss << JUIN(0,"game_length",      game_length)                 << ",\n";
  ss << JUIN(0,"winner_port",      winner_port)                 << ",\n";
  ss << JUIN(0,"start_minutes",    timer)                       << ",\n";
  ss << JUIN(0,"end_type",         end_type)                    << ",\n";
  ss << JINT(0,"lras",             lras_player)                 << ",\n";

  ss << "\"players\" : [\n";
  for(unsigned p = 0; p < 2; ++p) {
    ss << SPACE[ILEV] << "{\n";

    ss << JUIN(1,"port",                   ap[p].port)                      << ",\n";
    ss << JSTR(1,"tag_player",             escape_json(ap[p].tag_player))   << ",\n";
    ss << JSTR(1,"tag_css",                escape_json(ap[p].tag_css))      << ",\n";
    ss << JSTR(1,"tag_code",               escape_json(ap[p].tag_code))      << ",\n";
    ss << JUIN(1,"char_id",                ap[p].char_id)                   << ",\n";
    ss << JSTR(1,"char_name",              ap[p].char_name)                 << ",\n";
    ss << JUIN(1,"player_type" ,           ap[p].player_type)               << ",\n";
    ss << JUIN(1,"cpu_level" ,             ap[p].cpu_level)                 << ",\n";
    ss << JUIN(1,"color"       ,           ap[p].color)                     << ",\n";
    ss << JUIN(1,"team_id"     ,           ap[p].team_id)                   << ",\n";
    ss << JUIN(1,"start_stocks",           ap[p].start_stocks)              << ",\n";
    ss << JUIN(1,"end_stocks",             ap[p].end_stocks)                << ",\n";
    ss << JUIN(1,"end_pct",                ap[p].end_pct)                   << ",\n";

    ss << JUIN(1,"airdodges",              ap[p].airdodges)                 << ",\n";
    ss << JUIN(1,"spotdodges",             ap[p].spotdodges)                << ",\n";
    ss << JUIN(1,"rolls",                  ap[p].rolls)                     << ",\n";
    ss << JUIN(1,"dashdances",             ap[p].dashdances)                << ",\n";
    ss << JUIN(1,"l_cancels_hit",          ap[p].l_cancels_hit)             << ",\n";
    ss << JUIN(1,"l_cancels_missed",       ap[p].l_cancels_missed)          << ",\n";
    ss << JUIN(1,"techs",                  ap[p].techs)                     << ",\n";
    ss << JUIN(1,"walltechs",              ap[p].walltechs)                 << ",\n";
    ss << JUIN(1,"walljumps",              ap[p].walljumps)                 << ",\n";
    ss << JUIN(1,"walltechjumps",          ap[p].walltechjumps)             << ",\n";
    ss << JUIN(1,"missed_techs",           ap[p].missed_techs)              << ",\n";
    ss << JUIN(1,"ledge_grabs",            ap[p].ledge_grabs)               << ",\n";
    ss << JUIN(1,"air_frames",             ap[p].air_frames)                << ",\n";
    ss << JUIN(1,"wavedashes",             ap[p].wavedashes)                << ",\n";
    ss << JUIN(1,"wavelands",              ap[p].wavelands)                 << ",\n";
    ss << JUIN(1,"neutral_wins",           ap[p].neutral_wins)              << ",\n";
    ss << JUIN(1,"pokes",                  ap[p].pokes)                     << ",\n";
    ss << JUIN(1,"counters",               ap[p].counters)                  << ",\n";
    ss << JUIN(1,"powershields",           ap[p].powershields)              << ",\n";
    ss << JUIN(1,"shield_breaks",          ap[p].shield_breaks)             << ",\n";
    ss << JUIN(1,"grabs",                  ap[p].grabs)                     << ",\n";
    ss << JUIN(1,"grab_escapes",           ap[p].grab_escapes)              << ",\n";
    ss << JUIN(1,"taunts",                 ap[p].taunts)                    << ",\n";
    ss << JUIN(1,"meteor_cancels",         ap[p].meteor_cancels)            << ",\n";
    ss << JFLT(1,"damage_dealt",           ap[p].damage_dealt)              << ",\n";
    ss << JUIN(1,"hits_blocked",           ap[p].hits_blocked)              << ",\n";
    ss << JUIN(1,"shield_stabs",           ap[p].shield_stabs)              << ",\n";
    ss << JUIN(1,"edge_cancel_aerials",    ap[p].edge_cancel_aerials)       << ",\n";
    ss << JUIN(1,"edge_cancel_specials",   ap[p].edge_cancel_specials)      << ",\n";
    ss << JUIN(1,"teeter_cancel_aerials",  ap[p].teeter_cancel_aerials)     << ",\n";
    ss << JUIN(1,"teeter_cancel_specials", ap[p].teeter_cancel_specials)    << ",\n";
    ss << JUIN(1,"phantom_hits",           ap[p].phantom_hits)              << ",\n";
    ss << JUIN(1,"no_impact_lands",        ap[p].no_impact_lands)           << ",\n";
    ss << JUIN(1,"shield_drops",           ap[p].shield_drops)              << ",\n";
    ss << JUIN(1,"pivots",                 ap[p].pivots)                    << ",\n";
    ss << JUIN(1,"reverse_edgeguards",     ap[p].reverse_edgeguards)        << ",\n";
    ss << JUIN(1,"self_destructs",         ap[p].self_destructs)            << ",\n";
    ss << JUIN(1,"stage_spikes",           ap[p].stage_spikes)              << ",\n";
    ss << JUIN(1,"short_hops",             ap[p].short_hops)                << ",\n";
    ss << JUIN(1,"full_hops",              ap[p].full_hops)                 << ",\n";
    ss << JUIN(1,"shield_time",            ap[p].shield_time)               << ",\n";
    ss << JFLT(1,"shield_damage",          ap[p].shield_damage)             << ",\n";
    ss << JFLT(1,"shield_lowest",          ap[p].shield_lowest)             << ",\n";
    ss << JUIN(1,"total_openings",         ap[p].total_openings)            << ",\n";
    ss << JFLT(1,"mean_kill_openings",     ap[p].mean_kill_openings)        << ",\n";
    ss << JFLT(1,"mean_kill_percent",      ap[p].mean_kill_percent)         << ",\n";
    ss << JFLT(1,"mean_opening_percent",   ap[p].mean_opening_percent)      << ",\n";
    ss << JUIN(1,"galint_ledgedashes",     ap[p].galint_ledgedashes)        << ",\n";
    ss << JFLT(1,"mean_galint",            ap[p].mean_galint)               << ",\n";
    ss << JUIN(1,"max_galint",             ap[p].max_galint)                << ",\n";
    ss << JUIN(1,"button_count",           ap[p].button_count)              << ",\n";
    ss << JUIN(1,"cstick_count",           ap[p].cstick_count)              << ",\n";
    ss << JUIN(1,"astick_count",           ap[p].astick_count)              << ",\n";
    ss << JFLT(1,"actions_per_min",        ap[p].apm)                       << ",\n";
    ss << JUIN(1,"state_changes",          ap[p].state_changes)             << ",\n";
    ss << JFLT(1,"states_per_min",         ap[p].aspm)                      << ",\n";
    ss << JUIN(1,"shieldstun_times",       ap[p].shieldstun_times)          << ",\n";
    ss << JUIN(1,"shieldstun_act_frames",  ap[p].shieldstun_act_frames)     << ",\n";
    ss << JUIN(1,"hitstun_times",          ap[p].hitstun_times)             << ",\n";
    ss << JUIN(1,"hitstun_act_frames",     ap[p].hitstun_act_frames)        << ",\n";
    ss << JUIN(1,"wait_times",             ap[p].wait_times)                << ",\n";
    ss << JUIN(1,"wait_act_frames",        ap[p].wait_act_frames)           << ",\n";
    ss << JUIN(1,"used_norm_moves",        ap[p].used_norm_moves)           << ",\n";
    ss << JUIN(1,"used_spec_moves",        ap[p].used_spec_moves)           << ",\n";
    ss << JUIN(1,"used_misc_moves",        ap[p].used_misc_moves)           << ",\n";
    ss << JUIN(1,"used_grabs",             ap[p].used_grabs)                << ",\n";
    ss << JUIN(1,"used_pummels",           ap[p].used_pummels)              << ",\n";
    ss << JUIN(1,"used_throws",            ap[p].used_throws)               << ",\n";
    ss << JUIN(1,"total_moves_used",       ap[p].total_moves_used)          << ",\n";
    ss << JUIN(1,"total_moves_landed",     ap[p].total_moves_landed)        << ",\n";
    ss << JFLT(1,"move_accuracy",          ap[p].move_accuracy)             << ",\n";
    ss << JFLT(1,"actionability",          ap[p].actionability)             << ",\n";
    ss << JFLT(1,"neutral_wins_per_min",   ap[p].neutral_wins_per_min)      << ",\n";
    ss << JFLT(1,"mean_death_percent",     ap[p].mean_death_percent)        << ",\n";

    ss << SPACE[ILEV] << "\"interaction_frames\" : {\n";
    for(unsigned d = Dynamic::__LAST-1; d > 0; --d) {
      ss << JUIN(2,Dynamic::name[d], ap[p].dyn_counts[d]) << ((d == 1) ? "\n" : ",\n");
    }
    ss << SPACE[ILEV] << "},\n";

    ss << SPACE[ILEV] << "\"interaction_damage\" : {\n";
    for(unsigned d = Dynamic::__LAST-1; d > 0; --d) {
      ss << JFLT(2,Dynamic::name[d], ap[p].dyn_damage[d]) << ((d == 1) ? "\n" : ",\n");
    }
    ss << SPACE[ILEV] << "},\n";

    ss << SPACE[ILEV] << "\"moves_landed\" : {\n";
    unsigned _total_moves = 0;
    for(unsigned d = 0; d < Move::BUBBLE; ++d) {
      if ((ap[p].move_counts[d]) > 0) {
        ss << JUIN(2,Move::name[d], ap[p].move_counts[d]) << ",\n";
        _total_moves += ap[p].move_counts[d];
      }
    }
    ss << JUIN(2,"_total", _total_moves) << "\n";
    ss << SPACE[ILEV] << "},\n";

    ss << SPACE[ILEV] << "\"attacks\" : [\n";
    for(unsigned i = 0; ap[p].attacks[i].frame > 0; ++i) {
      ss << SPACE[2*ILEV] << "{" << std::endl;
      ss << JUIN(2,"move_id",         ap[p].attacks[i].move_id)                        << ",\n";
      ss << JSTR(2,"move_name",       Move::shortname[ap[p].attacks[i].move_id])       << ",\n";
      ss << JUIN(2,"cancel_type",     ap[p].attacks[i].cancel_type)                    << ",\n";
      ss << JSTR(2,"cancel_name",     Cancel::shortname[ap[p].attacks[i].cancel_type]) << ",\n";
      ss << JUIN(2,"punish_id",       ap[p].attacks[i].punish_id)                      << ",\n";
      ss << JUIN(2,"hit_id",          ap[p].attacks[i].hit_id)                         << ",\n";
      ss << JUIN(2,"game_frame",      ap[p].attacks[i].frame)                          << ",\n";
      ss << JUIN(2,"anim_frame",      ap[p].attacks[i].anim_frame)                     << ",\n";
      ss << JFLT(2,"damage",          ap[p].attacks[i].damage)                         << ",\n";
      ss << JSTR(2,"opening",         Dynamic::name[ap[p].attacks[i].opening])         << ",\n";
      ss << JSTR(2,"kill_dir",        Dir::name[ap[p].attacks[i].kill_dir])            << "\n";
      ss << SPACE[2*ILEV] << "}" << ((ap[p].attacks[i+1].frame > 0) ? ",\n" : "\n");
    }
    ss << SPACE[ILEV] << "],\n";

    ss << SPACE[ILEV] << "\"punishes\" : [\n";
    for(unsigned i = 0; ap[p].punishes[i].num_moves > 0; ++i) {
      ss << SPACE[2*ILEV] << "{" << std::endl;
      ss << JUIN(2,"start_frame",     ap[p].punishes[i].start_frame)                   << ",\n";
      ss << JUIN(2,"end_frame",       ap[p].punishes[i].end_frame)                     << ",\n";
      ss << JFLT(2,"start_pct",       ap[p].punishes[i].start_pct)                     << ",\n";
      ss << JFLT(2,"end_pct",         ap[p].punishes[i].end_pct)                       << ",\n";
      ss << JUIN(2,"stocks",          ap[p].punishes[i].stocks)                       << ",\n";
      ss << JUIN(2,"num_moves",       ap[p].punishes[i].num_moves)                     << ",\n";
      ss << JUIN(2,"last_move_id",    ap[p].punishes[i].last_move_id)                  << ",\n";
      ss << JSTR(2,"last_move_name",  Move::shortname[ap[p].punishes[i].last_move_id]) << ",\n";
      ss << JSTR(2,"opening",         "UNUSED")                                        << ",\n";
      ss << JSTR(2,"kill_dir",        Dir::name[ap[p].punishes[i].kill_dir])           << "\n";
      ss << SPACE[2*ILEV] << "}" << ((ap[p].punishes[i+1].num_moves > 0) ? ",\n" : "\n");
    }
    ss << SPACE[ILEV] << "]\n";

    ss << SPACE[ILEV] << "}" << ( p== 0 ? ",\n" : "\n");
  }
  ss << "]\n";
  ss << "}\n";
  return ss.str();
}

void Analysis::save(const char* outfilename) {
  std::ofstream fout;
  fout.open(outfilename);
  std::string j = asJson();
  fout << j << std::endl;
  fout.close();
}

}
