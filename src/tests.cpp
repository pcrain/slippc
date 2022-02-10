#include <algorithm>

#include "util.h"
#include "parser.h"
#include "analyzer.h"
#include "compressor.h"

#ifdef _WIN32
#include <Windows.h> //sleep()
#else
#include <unistd.h> //sleep()
#endif

// ANSI color codes don't work on Windows (I think?)
#ifdef _WIN32
    #define BLK ""
    #define RED ""
    #define GRN ""
    #define YLW ""
    #define BLU ""
    #define MGN ""
    #define CYN ""
    #define WHT ""
    #define BLN ""
#else
    #define BLK "\u001b[30m"
    #define RED "\u001b[31m"
    #define GRN "\u001b[32m"
    #define YLW "\u001b[33m"
    #define BLU "\u001b[34m"
    #define MGN "\u001b[35m"
    #define CYN "\u001b[36m"
    #define WHT "\u001b[37m"
    #define BLN "\u001b[0m"
#endif

typedef struct _test_suite {
  const char* name;
  unsigned p = 0;
  unsigned w = 0;
  unsigned f = 0;
  unsigned t = 0;
  _test_suite* next;
} test_suite;

static unsigned __tests_total  = 0;
static unsigned __tests_passed = 0;
static unsigned __tests_warned = 0;
static unsigned __tests_failed = 0;
static test_suite* suites = NULL, *tail = NULL;
static test_suite* s;

#define TSUITE(tag) \
  std::cout << "----------\nTesting suite " << CYN << (tag) << BLN << std::endl; \
  s = (test_suite*)calloc(1,sizeof(test_suite)); \
  s->name = (tag); \
  if(!tail) {suites     = s;} \
  else      {tail->next = s;} \
  tail = s;

#define TPASS() \
  std::cout << GRN << "\r  pass\n" << BLN; \
  ++__tests_total; \
  ++__tests_passed; \
  ++(tail->t); \
  ++(tail->p);
#define TWARN() \
  std::cout << YLW << "\r  warn\n" << BLN; \
  ++__tests_total; \
  ++__tests_warned; \
  ++(tail->t); \
  ++(tail->w);
#define TFAIL() \
  std::cout << RED << "\r  fail\n" << BLN; \
  ++__tests_total; \
  ++__tests_failed; \
  ++(tail->t); \
  ++(tail->f);

#define ASSERT(name,expr,onfail) \
  std::cout << "       " << (name) << BLN; \
  if ((expr)) { TPASS(); } else { TFAIL(); std::cout << "        " << onfail << std::endl; }

#define SUGGEST(name,expr,onfail) \
  std::cout << "       " << (name) << BLN; \
  if ((expr)) { TPASS(); } else { TWARN(); std::cout << "        " << onfail << std::endl; }

#define TESTRESULTS() \
  std::cout << "----------TEST SUMMARY----------" << std::endl; \
  std::cout << GRN << "  " << __tests_passed << "/" << __tests_total << " tests passed" << " (" << ((100.0*__tests_passed)/(__tests_passed+__tests_failed+__tests_warned))<< "%)" << BLN; \
  if (__tests_failed) { \
    std::cout << " with " << RED << __tests_failed << " failure" << (__tests_failed==1 ? "" : "s") << BLN; \
    if (__tests_warned) { \
      std::cout << " and " << YLW << __tests_warned << " warning" << (__tests_warned==1 ? "" : "s") << BLN; \
    } \
  } else if (__tests_warned) { \
    std::cout << " with " << YLW << __tests_warned << " warning" << (__tests_warned==1 ? "" : "s") << BLN; \
  } \
  std::cout << std::endl; \
  for(test_suite* s = suites; s; ) { \
    printf("    %s%30s%s: %3u test%s passed (%3.0f%%)",s->f ? RED : s->w ? YLW : GRN,s->name,BLN,s->p,s->p==1 ? " " : "s",(100.0f*s->p/s->t)); \
    if (s->f) { \
      printf(" with %s%u failure%s%s",RED,s->f,s->f==1 ? "" : "s",BLN);\
      if (s->w) { \
        printf(" and %s%u warning%s%s",YLW,s->w,s->w==1 ? "" : "s",BLN);\
      }\
    } else if (s->w) { \
      printf(" with %s%u warning%s%s",YLW,s->w,s->w==1 ? "" : "s",BLN);\
    }\
    printf("\n"); \
    struct _test_suite* next = s->next; \
    free(s); \
    s = next; \
  } \
  std::cout << "--------END TEST SUMMARY--------" << std::endl; \

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
    SUGGEST("All Fail 1",1!=1,
      1 << " == " << 1);
    SUGGEST("All Fail 2",2!=2,
      2 << " == " << 2);
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
