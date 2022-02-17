#include <algorithm>

#include "util.h"
#include "parser.h"
#include "analyzer.h"
#include "compressor.h"

// #define GUI_ENABLED 1  //debug, normally enable this from the makefile

#if GUI_ENABLED == 1
  #include "portable-file-dialogs.h"
#endif

namespace slip {

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
    << "  -i        Parse and analyze <infile> (not very useful on its own)" << std::endl
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

int parseSingleFile(const cmdoptions &c, const int debug) {
  if(c.cfile || c.encode) {
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

  slip::Parser p(debug);
  if (not p.load(c.infile)) {
    std::cerr << "Could not load input; exiting" << std::endl;
    return 2;
  }

  if (c.outfile) {
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
  }

  if (c.analysisfile) {
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
  }

  if (debug) {
    std::cerr << "Cleaning up" << std::endl;
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
    std::cerr << "No output file selected" << std::endl;
    printUsage();
    return -1;
  }

  return parseSingleFile(c,c.debug);
}

}

int main(int argc, char** argv) {
  return slip::run(argc,argv);
}
