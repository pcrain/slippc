#include "util.h"
#include "parser.h"

const char* TESTREPLAY = "/home/pretzel/workspace/slippi-replay-parser/Game_20190712T200124.slp";
const char* TESTOUT = "/home/pretzel/workspace/slippi-replay-parser/out.json";

int main(int argc, char** argv) {
  slip::Parser p;
  p.load(TESTREPLAY);
  p.summary();
  p.save(TESTOUT);
  return 0;
}
