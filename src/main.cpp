#include "util.h"
#include "parser.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "Usage: slippc <infile> [<outfile>]" << std::endl;
    return 1;
  }
  std::string outfile;
  if (argc == 2) {
    outfile = std::string(argv[1])+".json";
  } else {
    outfile = std::string(argv[2]);
  }
  slip::Parser p;
  p.load(argv[1]);
  p.summary();
  p.save(outfile.c_str());
  return 0;
}
