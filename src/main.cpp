#include <algorithm>

#include "util.h"
#include "parser.h"
#include "analyzer.h"

// https://stackoverflow.com/questions/865668/how-to-parse-command-line-arguments-in-c
char* getCmdOption(char ** begin, char ** end, const std::string & option) {
  char ** itr = std::find(begin, end, option);
  if (itr != end && ++itr != end) {
    return *itr;
  }
  return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option) {
  return std::find(begin, end, option) != end;
}

void printUsage() {
  std::cout
    << "Usage: slippc -i <infile> [-o <outfile>] [-a <analysisfile>] [-f] [-d] [-h]:" << std::endl
    << "  -i     Parse and analyze <infile>" << std::endl
    << "  -o     Output the input replay in .json format to <outfile>" << std::endl
    << "  -a     Output an analysis of the input replay to <analysisfile>" << std::endl
    << "  -f     When used with -o <outfile>, write full frame info (c.f. frame deltas)" << std::endl
    << "  -d     Run in debug mode (show debug output)" << std::endl
    << "  -h     Show this help message" << std::endl
    ;
}

int main(int argc, char** argv) {
  if (cmdOptionExists(argv, argv+argc, "-h")) {
    printUsage();
    return 0;
  }
  bool debug = cmdOptionExists(argv, argv+argc, "-d");
  char * infile = getCmdOption(argv, argv + argc, "-i");
  if (not infile) {
    printUsage();
    return -1;
  }
  bool delta = not cmdOptionExists(argv, argv+argc, "-f");

  slip::Parser *p = new slip::Parser(debug);
  if (not p->load(infile)) {
    return 2;
  }

  char * outfile = getCmdOption(argv, argv + argc, "-o");
  if (outfile) {
    p->save(outfile,delta);
  }

  char * analysisfile = getCmdOption(argv, argv + argc, "-a");
  if (analysisfile) {
    slip::Analysis *a = p->analyze();
    a->save(analysisfile);
  }

  delete p;
  return 0;
}
