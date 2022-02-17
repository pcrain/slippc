#include <algorithm>
#include <sys/stat.h>
#include <filesystem>

#include "util.h"
#include "parser.h"
#include "analyzer.h"
#include "compressor.h"

// #define GUI_ENABLED 1  //debug, normally enable this from the makefile

#if GUI_ENABLED == 1
  #include "portable-file-dialogs.h"
#endif

namespace slip {

#define PATH std::filesystem::path

typedef std::filesystem::directory_iterator f_iter;
typedef std::filesystem::directory_entry    f_entry;
#if GUI_ENABLED == 1
  typedef std::vector<std::__cxx11::basic_string<char> > str_vec;
#endif

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

#if GUI_ENABLED == 1
  bool askYesNo(std::string title, std::string question) {
    return pfd::message(title, question, pfd::choice::yes_no, pfd::icon::question).result()==pfd::button::yes;
  }
#endif

void printUsage() {
  std::cout
    << "Usage: slippc -i <infile> [-x | -X <zlpfle>] [-j <jsonfile>] [-a <analysisfile>] [-f] [-d <debuglevel>] [-h]:" << std::endl
    << "  -i        Parse and analyze <infile> (can be a whole directory)" << std::endl
    << "  -j        Output <infile> in .json format to <jsonfile> (use \"-\" for stdout)" << std::endl
    << "  -a        Output an analysis of <infile> in .json format to <analysisfile> (use \"-\" for stdout)" << std::endl
    << "  -f        When used with -j <jsonfile>, write full frame info (instead of just frame deltas)" << std::endl
    << "  -x        Compress or decompress a replay" << std::endl
    << "  -X        Set output file name for compression" << std::endl
    << std::endl
    << "Debug options:" << std::endl
    << "  -d           Run at debug level <debuglevel> (show debug output)" << std::endl
    << "  --skip-save  Skip saving compressed replay, validate only" << std::endl
    << "  --raw-enc    Output raw encodes with -x (DANGEROUS, debug only)" << std::endl
    << "  --dump-gecko Dump gecko codes to <inputfilename>.dat" << std::endl
    << "  -h           Show this help message" << std::endl
    ;
}

typedef struct _cmdoptions {
  char* dlevel       = nullptr;
  char* infile       = nullptr;
  char* cfile        = nullptr;
  char* outfile      = nullptr;
  char* analysisfile = nullptr;
  bool  nodelta      = false;
  bool  encode       = false;
  bool  rawencode    = false;
  bool  skipsave     = false;
  bool  dumpgecko    = false;
  int   debug        = 0;
} cmdoptions;

cmdoptions getCommandLineOptions(int argc, char** argv) {
  _cmdoptions c;
  c.dlevel       = getCmdOption(   argv, argv+argc, "-d");
  c.infile       = getCmdOption(   argv, argv+argc, "-i");
  c.cfile        = getCmdOption(   argv, argv+argc, "-X");
  c.outfile      = getCmdOption(   argv, argv+argc, "-j");
  c.analysisfile = getCmdOption(   argv, argv+argc, "-a");
  c.nodelta      = cmdOptionExists(argv, argv+argc, "-f");
  c.encode       = cmdOptionExists(argv, argv+argc, "-x");
  c.rawencode    = cmdOptionExists(argv, argv+argc, "--raw-enc");
  c.skipsave     = cmdOptionExists(argv, argv+argc, "--skip-save");
  c.dumpgecko    = cmdOptionExists(argv, argv+argc, "--dump-gecko");

  if (c.dlevel) {
    if (c.dlevel[0] >= '0' && c.dlevel[0] <= '9') {
      c.debug = c.dlevel[0]-'0';
    } else {
      std::cerr << "Warning: invalid debug level" << std::endl;
    }
  }
  if (c.debug) {
    std::cerr << "Running at debug level " << +c.debug << std::endl;
  }

  return c;
}

#if GUI_ENABLED == 1
  void getGUIOptions(cmdoptions &c) {
    if (_debug < 1) {
      _debug = 1;  //force some debug output
    }
    std::string save, saveext;
    str_vec file_res = pfd::open_file(
      "Select an input File", ".", {"Slippi Files", "*.slp *.zlp"}).result();
    if(file_res.empty()) {
      printUsage();
      return "";
    }
    stringtoChars(file_res[0],&c.infile);

    std::string inbase = getFileBase(file_res[0]);
    std::string inext  = getFileExt(file_res[0]);

    if (inext.compare("slp") == 0) {
      if (askYesNo("Compress?","Output compressed file (yes) or JSON (no)?")) {
        std::cerr << "GUI mode, compressed output" << std::endl;
        save = pfd::save_file("Select an Output file", inbase+".zlp", { "Zlippi Files", "*.zlp"}).result();
        stringtoChars(save,&c.cfile);
      } else if (askYesNo("Analysis?", "Output analysis JSON (yes) or regular JSON (no)?")) {
        std::cerr << "GUI mode, analysis output" << std::endl;
        save = pfd::save_file("Select an Output file", inbase+".json", { "JSON Files", "*.json"}).result();
        stringtoChars(save,&c.analysisfile);
      } else {
        std::cerr << "GUI mode, JSON output" << std::endl;
        save = pfd::save_file("Select an Output file", inbase+".json", { "JSON Files", "*.json"}).result();
        stringtoChars(save,&c.outfile);
      }
    } else if (inext.compare("zlp") == 0) {
      std::cerr << "GUI mode, decompressed output" << std::endl;
      save = pfd::save_file("Select an Output file", inbase+".slp", { "Slippi Files", "*.slp"}).result();
      stringtoChars(save,&c.cfile);
    }
  }
#endif

inline void copyCommandOptions(const cmdoptions &c, cmdoptions &c2) {
  memcpy(&c2,&c,sizeof(cmdoptions));
}

inline void cleanupCommandOptions(cmdoptions &c) {
  if(c.cfile) {
    delete[] c.cfile;
  }
  if(c.outfile) {
    delete[] c.outfile;
  }
  if(c.analysisfile) {
    delete[] c.analysisfile;
  }
}

int handleCompression(const cmdoptions &c, const int debug) {
  slip::Compressor cmp(debug);

  if (c.cfile) {
    if (!(cmp.setOutputFilename(c.cfile))) {
      std::cerr << "File " << c.cfile << " already exists or is invalid" << std::endl;
      return 4;
    }
  }

  if (c.dumpgecko) {
    std::cerr << "setting gecko output filename" << std::endl;
    cmp.setGeckoOutputFilename(c.infile);
  }

  std::cerr << "Encoding / decoding replay" << std::endl;
  if (not cmp.loadFromFile(c.infile)) {
    std::cerr << "Failed to encode input; exiting" << std::endl;
    return 2;
  }

  std::cerr << "Validating encoding" << std::endl;
  if (!cmp.validate()) {
    std::cerr << "Validation failed; exiting" << std::endl;
    return 3;
  }

  if (c.skipsave) {
    std::cerr << "Skipping saving" << std::endl;
  } else {
    std::cerr << "Saving encoded / decoded replay" << std::endl;
    cmp.saveToFile(c.rawencode);
  }

  std::cerr << "Saved!" << std::endl;
  return 0;
}

int handleAnalysis(const cmdoptions &c, const int debug, slip::Parser &p) {
  slip::Analysis *a  = p.analyze();

  if (a->success) {
    if (c.analysisfile[0] == '-' && c.analysisfile[1] == '\0') {
      if (debug) {
        std::cerr << "Writing analysis to stdout" << std::endl;
      }
      std::cout << a->asJson() << std::endl;
    } else {
      if (debug) {
        std::cerr << "Saving analysis to file" << std::endl;
      }
      a->save(c.analysisfile);
    }
  }
  if (debug) {
    std::cerr << "Deleting analysis" << std::endl;
  }

  delete a;
  return 0;
}

int handleJson(const cmdoptions &c, const int debug, slip::Parser &p) {
  if (c.outfile[0] == '-' && c.outfile[1] == '\0') {
    if (debug) {
      std::cerr << "Writing Slippi JSON data to stdout" << std::endl;
    }
    std::cout << p.asJson(!c.nodelta) << std::endl;
  } else {
    if (debug) {
      std::cerr << "Saving Slippi JSON data to file" << std::endl;
    }
    p.save(c.outfile,!c.nodelta);
  }
  return 0;
}

int handleSingleFile(const cmdoptions &c, const int debug) {
  int retc = 0;  //return value from compression phase
  int reta = 0;  //return value from analysis phase
  int retj = 0;  //return value from jsonoutput phase

  if (c.cfile || c.encode) {
    retc = handleCompression(c,debug);
  }

  if (c.outfile || c.analysisfile) {
    slip::Parser p(debug);
    if (not p.load(c.infile)) {
      std::cerr << "Could not load input; exiting" << std::endl;
      return 2;
    }

    if (c.outfile) {
      retj = handleJson(c,debug,p);
    }

    if (c.analysisfile) {
      reta = handleAnalysis(c,debug,p);
    }
  }

  if (debug) {
    std::cerr << "Cleaning up" << std::endl;
  }
  return retc+reta+retj;
}

int handleDirectory(const cmdoptions &c, const int debug) {
  // verify all of our input and output directories are valid (not files + proper write permissions)
  if (!(c.cfile || c.outfile || c.analysisfile)) {
    std::cerr << "No output directories specified with -j, -a, or -X" << std::endl;
    return -2;
  }
  if (c.outfile && (!makeDirectoryIfNotExists(c.outfile))) {
    std::cerr << "JSON output directory '" << c.outfile << "' is not a valid directory" << std::endl;
    return -2;
  }
  if (c.analysisfile && (!makeDirectoryIfNotExists(c.analysisfile))) {
    std::cerr << "Analysis output directory '" << c.analysisfile << "' is not a valid directory" << std::endl;
    return -2;
  }
  if (c.cfile && (!makeDirectoryIfNotExists(c.cfile))) {
    std::cerr << "Compression output directory '" << c.cfile << "' is not a valid directory" << std::endl;
    return -2;
  }

  // find all slippi files in a directory
  for (const f_entry & entry : f_iter(std::string(c.infile))) {
    std::string base  = entry.path().filename();
    std::string noext = entry.path().stem();
    if (getFileExt(base).compare("slp") == 0) {
      cmdoptions c2;
      copyCommandOptions(c,c2);
      stringtoChars((entry.path()).string(),&(c2.infile));
      if(c2.cfile) {
        stringtoChars((PATH(c.cfile) / PATH(noext+".zlp")).string(),&(c2.cfile));
      }
      if(c2.outfile) {
        stringtoChars((PATH(c.outfile) / PATH(base+".json")).string(),&(c2.outfile));
      }
      if(c2.analysisfile) {
        stringtoChars((PATH(c.analysisfile) / PATH(noext+"-analysis.json")).string(),&(c2.analysisfile));
      }
      // std::cout << "  -i "   << c2.infile << std::endl;
      // std::cout << "    -X " << c2.cfile << std::endl;
      // std::cout << "    -j " << c2.outfile << std::endl;
      // std::cout << "    -a " << c2.analysisfile << std::endl;
      int ret = handleSingleFile(c2,debug);
      if (ret != 0) {
        std::cerr << "Encountered errors handling input file " << RED << c2.infile << BLN << std::endl;
      }
      cleanupCommandOptions(c2);
    }
  }
  return 0;
}

int run(int argc, char** argv) {
  if (cmdOptionExists(argv, argv+argc, "-h")) {
    printUsage();
    return 0;
  }

  cmdoptions c = getCommandLineOptions(argc,argv);

  #if GUI_ENABLED == 1
    if (not c.infile) { //if we don't have an input file, open file selector
      getGUIOptions(c);
    }
  #endif

  if (not c.infile) { //if we still don't have an input file, exit
    std::cerr << "No input selected" << std::endl;
    printUsage();
    return -1;
  }

  if(isDirectory(c.infile)) {
    return handleDirectory(c,c.debug);
  }
  return handleSingleFile(c,c.debug);
}

}

int main(int argc, char** argv) {
  return slip::run(argc,argv);
}
