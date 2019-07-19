#include "util.h"
#include "parser.h"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cout << "Usage: slippc <infile> <outfile>" << std::endl;
    return 1;
  }
  slip::Parser p;
  p.load(argv[1]);
  p.summary();
  p.save(argv[2]);
  return 0;
}
