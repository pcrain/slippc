#include <algorithm>

#include "util.h"
#include "parser.h"
#include "analyzer.h"
#include "compressor.h"

#if GUI_ENABLED == 1
  #include "portable-file-dialogs.h"
#endif

namespace slip {

typedef std::vector<std::__cxx11::basic_string<char> > str_vec;

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
    << "  --raw-enc    Output raw encodes with -x (DANGEROUS, debug only)" << std::endl
    << "  --dump-gecko Dump gecko codes to <inputfilename>.dat" << std::endl
    << "  -h           Show this help message" << std::endl
    ;
}

int run(int argc, char** argv) {
  if (cmdOptionExists(argv, argv+argc, "-h")) {
    printUsage();
    return 0;
  }

  int debug          = 0;
  char* dlevel       = getCmdOption(   argv, argv+argc, "-d");
  char* infile       = getCmdOption(   argv, argv+argc, "-i");
  char* cfile        = getCmdOption(   argv, argv+argc, "-X");
  char* outfile      = getCmdOption(   argv, argv+argc, "-j");
  char* analysisfile = getCmdOption(   argv, argv+argc, "-a");
  bool  nodelta      = cmdOptionExists(argv, argv+argc, "-f");
  bool  encode       = cmdOptionExists(argv, argv+argc, "-x");
  bool  rawencode    = cmdOptionExists(argv, argv+argc, "--raw-enc");
  bool  dumpgecko    = cmdOptionExists(argv, argv+argc, "--dump-gecko");

  if (dlevel) {
    if (dlevel[0] >= '0' && dlevel[0] <= '9') {
      debug = dlevel[0]-'0';
    } else {
      std::cerr << "Warning: invalid debug level" << std::endl;
    }
  }
  if (debug) {
    std::cout << "Running at debug level " << +debug << std::endl;
  }

  if (not infile) { //Running in GUI dialog mode
    std::string save, saveext;
    #if GUI_ENABLED == 1
      debug = 1;  //Force some debug output in GUI mode
      str_vec file_res = pfd::open_file(
        "Select an input File", ".", {"Slippi Files", "*.slp *.zlp"}).result();
      if(file_res.empty()) {
        printUsage();
        return -1;
      }
      stringtoChars(file_res[0],&infile);

      std::string inbase = getFileBase(file_res[0]);
      std::string inext  = getFileExt(file_res[0]);

      if (inext.compare("slp") == 0) {
        if (askYesNo("Compress?","Output compressed file (yes) or JSON (no)?")) {
          std::cout << "GUI mode, compressed output" << std::endl;
          save = pfd::save_file("Select an Output file", inbase+".zlp", { "Zlippi Files", "*.zlp"}).result();
          stringtoChars(save,&cfile);
        } else if (askYesNo("Analysis?", "Output analysis JSON (yes) or regular JSON (no)?")) {
          std::cout << "GUI mode, analysis output" << std::endl;
          save = pfd::save_file("Select an Output file", inbase+".json", { "JSON Files", "*.json"}).result();
          stringtoChars(save,&analysisfile);
        } else {
          std::cout << "GUI mode, JSON output" << std::endl;
          save = pfd::save_file("Select an Output file", inbase+".json", { "JSON Files", "*.json"}).result();
          stringtoChars(save,&outfile);
        }
      } else if (inext.compare("zlp") == 0) {
        std::cout << "GUI mode, decompressed output" << std::endl;
        save = pfd::save_file("Select an Output file", inbase+".slp", { "Slippi Files", "*.slp"}).result();
        stringtoChars(save,&cfile);
      }
    #endif
    if(save.empty()) {
      std::cerr << "No output file selected" << std::endl;
      printUsage();
      return -1;
    }
  }

  if(cfile || encode) {
    slip::Compressor *c = new slip::Compressor(debug);

    if (cfile) {
      if (!(c->setOutputFilename(cfile))) {
        std::cerr << "File " << cfile << " already exists or is invalid" << std::endl;
        return 4;
      }
    }

    if (dumpgecko) {
      std::cout << "setting gecko output filename" << std::endl;
      c->setGeckoOutputFilename(infile);
    }

    std::cerr << "Encoding / decoding replay" << std::endl;
    if (not c->loadFromFile(infile)) {
      std::cerr << "Failed to encode input; exiting" << std::endl;
      delete c;
      return 2;
    }
    if (rawencode) {
        std::cerr << "Skipping validation" << std::endl;
    } else {
      std::cerr << "Validating encoding" << std::endl;
      if (!c->validate()) {
        std::cerr << "Validation failed; exiting" << std::endl;
        return 3;
      }
    }
    std::cerr << "Saving encoded / decoded replay" << std::endl;
    c->saveToFile(rawencode);
    std::cerr << "Saved!" << std::endl;
    return 0;
  }

  slip::Parser *p = new slip::Parser(debug);
  if (not p->load(infile)) {
    std::cerr << "Could not load input; exiting" << std::endl;
    delete p;
    return 2;
  }

  if (outfile) {
    if (outfile[0] == '-' && outfile[1] == '\0') {
      if (debug) {
        std::cerr << "Writing Slippi JSON data to stdout" << std::endl;
      }
      std::cout << p->asJson(!nodelta) << std::endl;
    } else {
      if (debug) {
        std::cout << "Saving Slippi JSON data to file" << std::endl;
      }
      p->save(outfile,!nodelta);
    }
  }

  if (analysisfile) {
    slip::Analysis *a  = p->analyze();
    if (a->success) {
      if (analysisfile[0] == '-' && analysisfile[1] == '\0') {
        if (debug) {
          std::cout << "Writing analysis to stdout" << std::endl;
        }
        std::cout << a->asJson() << std::endl;
      } else {
        if (debug) {
          std::cout << "Saving analysis to file" << std::endl;
        }
        a->save(analysisfile);
      }
    }
    if (debug) {
      std::cout << "Deleting analysis" << std::endl;
    }

    delete a;
  }

  if (debug) {
    std::cout << "Cleaning up" << std::endl;
  }
  delete p;
  return 0;
}

}

int main(int argc, char** argv) {
  return slip::run(argc,argv);
}
