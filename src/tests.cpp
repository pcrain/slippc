#include "tests.h"

namespace slip {

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
    << "Usage: slippc-tests [...]:" << std::endl
    // << "  -i        Parse and analyze <infile> (not very useful on its own)" << std::endl
    // << "  -j        Output <infile> in .json format to <jsonfile> (use \"-\" for stdout)" << std::endl
    // << "  -a        Output an analysis of <infile> in .json format to <analysisfile> (use \"-\" for stdout)" << std::endl
    // << "  -f        When used with -j <jsonfile>, write full frame info (instead of just frame deltas)" << std::endl
    // << "  -x        Compress or decompress a replay" << std::endl
    // << "  -X        Set output file name for compression" << std::endl
    // << std::endl
    // << "Debug options:" << std::endl
    // << "  -d        Run at debug level <debuglevel> (show debug output)" << std::endl
    // << "  --raw-enc Output raw encodes with -x (DANGEROUS, debug only)" << std::endl
    // << "  -h        Show this help message" << std::endl
    ;
}

int runtests(int argc, char** argv) {
  if (cmdOptionExists(argv, argv+argc, "-h")) {
    printUsage();
    return 0;
  }

  TSUITE("Basic Pass");
    ASSERT("Pass 1",1==1,
      1 << " != " << 1);
    ASSERT("Pass 2",1!=2,
      1 << " == " << 2);
    ASSERT("Pass 3",2==2,
      2 << " != " << 2);
  TSUITE("Basic Assert");
    ASSERT("Pass",1==1,
      1 << " != " << 1);
    ASSERT("Fail",1==2,
      1 << " != " << 2);
    ASSERT("Fail 2",2!=2,
      2 << " == " << 2);
  TSUITE("Basic Suggest");
    SUGGEST("Suggest Pass",1==1,
      1 << " != " << 1);
    SUGGEST("Suggest Fail",1==2,
      1 << " != " << 2);
  TSUITE("Basic Both");
    SUGGEST("Both Pass",1==1,
      1 << " != " << 1);
    SUGGEST("Both Warn",1==2,
      1 << " != " << 2);
    ASSERT("Both Fail",1==2,
      1 << " != " << 2);
  TSUITE("All Fail");
    ASSERT("All Fail 1",1!=1,
      1 << " == " << 1);
    ASSERT("All Fail 2",2!=2,
      2 << " == " << 2);
  TSUITE("Vars");
    int x = 1, y = 1, z = 2;
    ASSERT("Var Pass 1",x==y,
      x << " == " << y);
    ASSERT("Var Fail 1",x!=y,
      x << " == " << y);
    ASSERT("Var Fail 2",x==z,
      x << " != " << z);
  TSUITE("Errors");
    ASSERT("Error 1",[](){throw std::logic_error("Error Test"); return true;}(),
    "1/0");

  TESTRESULTS();

  // int debug          = 0;
  // char* dlevel       = getCmdOption(   argv, argv+argc, "-d");
  // char* infile       = getCmdOption(   argv, argv+argc, "-i");
  // char* cfile        = getCmdOption(   argv, argv+argc, "-X");
  // char* outfile      = getCmdOption(   argv, argv+argc, "-j");
  // char* analysisfile = getCmdOption(   argv, argv+argc, "-a");
  // bool  nodelta      = cmdOptionExists(argv, argv+argc, "-f");
  // bool  encode       = cmdOptionExists(argv, argv+argc, "-x");
  // bool  rawencode    = cmdOptionExists(argv, argv+argc, "--raw-enc");

  return 0;
}

}

int main(int argc, char** argv) {
  return slip::runtests(argc,argv);
}
