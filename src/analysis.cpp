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

  ss << JSTR(0,"slippi_version",   slippi_version)  << ",\n";
  ss << JSTR(0,"parser_version",   parser_version)  << ",\n";
  ss << JSTR(0,"analyzer_version", analyzer_version)<< ",\n";
  ss << JSTR(0,"game_time",        game_time)       << ",\n";
  ss << JUIN(0,"stage_id",         stage_id)        << ",\n";
  ss << JSTR(0,"stage_name",       stage_name)      << ",\n";
  ss << JUIN(0,"game_length",      game_length)     << ",\n";

  ss << "\"players\" : [\n";
  for(unsigned p = 0; p < 2; ++p) {
    ss << SPACE[ILEV] << "{\n";

    ss << JUIN(1,"port",                 ap[p].port)                    << ",\n";
    ss << JSTR(1,"tag_player",           escape_json(ap[p].tag_player)) << ",\n";
    ss << JSTR(1,"tag_css",              escape_json(ap[p].tag_css))    << ",\n";
    ss << JUIN(1,"char_id",              ap[p].char_id)                 << ",\n";
    ss << JSTR(1,"char_name",            ap[p].char_name)               << ",\n";
    ss << JUIN(1,"player_type" ,         ap[p].player_type)             << ",\n";
    ss << JUIN(1,"color"       ,         ap[p].color)                   << ",\n";
    ss << JUIN(1,"team_id"     ,         ap[p].team_id)                 << ",\n";
    ss << JUIN(1,"start_stocks",         ap[p].start_stocks)            << ",\n";
    ss << JUIN(1,"end_stocks",           ap[p].end_stocks)              << ",\n";
    ss << JUIN(1,"end_pct",              ap[p].end_pct)                 << ",\n";

    ss << JUIN(1,"airdodges",            ap[p].airdodges)               << ",\n";
    ss << JUIN(1,"spotdodges",           ap[p].spotdodges)              << ",\n";
    ss << JUIN(1,"rolls",                ap[p].rolls)                   << ",\n";
    ss << JUIN(1,"dashdances",           ap[p].dashdances)              << ",\n";
    ss << JUIN(1,"l_cancels_hit",        ap[p].l_cancels_hit)           << ",\n";
    ss << JUIN(1,"l_cancels_missed",     ap[p].l_cancels_missed)        << ",\n";
    ss << JUIN(1,"techs",                ap[p].techs)                   << ",\n";
    ss << JUIN(1,"walltechs",            ap[p].walltechs)               << ",\n";
    ss << JUIN(1,"walljumps",            ap[p].walljumps)               << ",\n";
    ss << JUIN(1,"walltechjumps",        ap[p].walltechjumps)           << ",\n";
    ss << JUIN(1,"missed_techs",         ap[p].missed_techs)            << ",\n";
    ss << JUIN(1,"ledge_grabs",          ap[p].ledge_grabs)             << ",\n";
    ss << JUIN(1,"air_frames",           ap[p].air_frames)              << ",\n";
    ss << JUIN(1,"wavedashes",           ap[p].wavedashes)              << ",\n";
    ss << JUIN(1,"wavelands",            ap[p].wavelands)               << ",\n";
    ss << JUIN(1,"neutral_wins",         ap[p].neutral_wins)            << ",\n";
    ss << JUIN(1,"pokes",                ap[p].pokes)                   << ",\n";
    ss << JUIN(1,"counters",             ap[p].counters)                << ",\n";
    ss << JUIN(1,"powershields",         ap[p].powershields)            << ",\n";
    ss << JUIN(1,"shield_breaks",        ap[p].shield_breaks)           << ",\n";
    ss << JUIN(1,"grabs",                ap[p].grabs)                   << ",\n";
    ss << JUIN(1,"grab_escapes",         ap[p].grab_escapes)            << ",\n";
    ss << JUIN(1,"taunts",               ap[p].taunts)                  << ",\n";
    ss << JUIN(1,"meteor_cancels",       ap[p].meteor_cancels)          << ",\n";
    ss << JFLT(1,"damage_dealt",         ap[p].damage_dealt)            << ",\n";
    ss << JUIN(1,"hits_blocked",         ap[p].hits_blocked)            << ",\n";
    ss << JUIN(1,"shield_stabs",         ap[p].shield_stabs)            << ",\n";
    ss << JUIN(1,"edge_cancel_aerials",  ap[p].edge_cancel_aerials)     << ",\n";
    ss << JUIN(1,"edge_cancel_specials", ap[p].edge_cancel_specials)    << ",\n";
    ss << JUIN(1,"phantom_hits",         ap[p].phantom_hits)            << ",\n";
    ss << JUIN(1,"no_impact_lands",      ap[p].no_impact_lands)         << ",\n";
    ss << JUIN(1,"shield_drops",         ap[p].shield_drops)            << ",\n";
    ss << JUIN(1,"pivots",               ap[p].pivots)                  << ",\n";
    ss << JUIN(1,"reverse_edgeguards",   ap[p].reverse_edgeguards)      << ",\n";
    ss << JUIN(1,"self_destructs",       ap[p].self_destructs)          << ",\n";
    ss << JUIN(1,"stage_spikes",         ap[p].stage_spikes)            << ",\n";

    ss << SPACE[ILEV] << "\"interactions\" : {\n";
    for(unsigned d = Dynamic::__LAST-1; d > 0; --d) {
      ss << JUIN(2,Dynamic::name[d], ap[p].dyn_counts[d]) << ((d == 1) ? "\n" : ",\n");
    }
    ss << SPACE[ILEV] << "},\n";

    ss << SPACE[ILEV] << "\"moves_landed\" : {\n";
    unsigned _total_moves = 0;
    for(unsigned d = 0; d < Move::__LAST; ++d) {
      if ((ap[p].move_counts[d]) > 0) {
        ss << JUIN(2,Move::name[d], ap[p].move_counts[d]) << ",\n";
        _total_moves += ap[p].move_counts[d];
      }
    }
    ss << JUIN(2,"_total", _total_moves) << "\n";
    ss << SPACE[ILEV] << "},\n";

    ss << SPACE[ILEV] << "\"attacks\" : [\n";
    // move_id
    // anim_frame
    // punish_id
    // frame
    // damage
    // opening
    // kill_dir
    for(unsigned i = 0; ap[p].attacks[i].frame > 0; ++i) {
      ss << SPACE[2*ILEV] << "{" << std::endl;
      ss << JUIN(2,"move_id",         ap[p].attacks[i].move_id)                  << ",\n";
      ss << JSTR(2,"move_name",       Move::shortname[ap[p].attacks[i].move_id]) << ",\n";
      ss << JUIN(2,"punish_id",       ap[p].attacks[i].punish_id)                << ",\n";
      ss << JUIN(2,"hit_id",          ap[p].attacks[i].hit_id)                   << ",\n";
      ss << JUIN(2,"game_frame",      ap[p].attacks[i].frame)                    << ",\n";
      ss << JUIN(2,"anim_frame",      ap[p].attacks[i].anim_frame)               << ",\n";
      ss << JFLT(2,"damage",          ap[p].attacks[i].damage)                   << ",\n";
      ss << JSTR(2,"opening",         Dynamic::name[ap[p].attacks[i].opening])   << ",\n";
      ss << JSTR(2,"kill_dir",        Dir::name[ap[p].attacks[i].kill_dir])      << "\n";
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
