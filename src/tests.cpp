#include "tests.h"

const std::string TSLPPATH   = "/xmedia/pretzel/slippc-testing/";
const std::string BCSPATH    = "/xmedia/pretzel/slippc-testing/backward-compat-slp/";
const std::string BCZPATH    = "/xmedia/pretzel/slippc-testing/backward-compat-zlp/";
const std::string TSLPFILE   = "3-9-0-singles-irl-summit12.slp";
const std::string TCMPFILE   = "3-7-0-singles-net.slp";
const std::string TZLPFILE   = "/tmp/zlptest.zlp";
const std::string TUNZLPFILE = "/tmp/zlptest.slp";

const std::string ALLCOMPS[] = {
  // "3-7-0-modded-chars.slp",
  // "1-0-0-ics-pyslippi.slp",
  // "1-7-0-singles-irl-phoenix-blue-2.slp",
  // "1-7-1-pal-fizzi.slp",
  // "2-0-1-doubles-irl.slp",
  // "2-2-0-singles-irl-bighouse9.slp",
  "3-6-0-singles-net.slp",  //Compression unshuffling broken
  // "1-7-1-singles-irl-gang.slp",
  // "2-0-1-singles-irl.slp",
  // "2-0-1-singles-irl-genesis.slp",
  // "2-0-1-singles-irl-mainstage-2019.slp",
  // "2-0-1-singles-irl-shine-2019.slp",
  // "2-0-1-singles-irl-summit8.slp",
  // "2-0-1-singles-irl-summit11.slp",
  // "2-0-1-singles-net.slp",
  // "3-7-0-banned-stage.slp",
  // "3-7-0-items-pyslippi.slp",
  // "3-7-0-modded-chars.slp",
  // "3-7-0-singles-net.slp",
  // "3-7-0-singles-online-summit10.slp"
};

const std::string BACKCOMPS[] = {
  "_Game_20200809T092224",
  "_Game_20200809T092530",
  "_Game_20200809T092719",
  "_Game_20200809T092834",
  "_Game_20200809T092954",
  "_Game_20200809T093104",
  "_Game_20200809T093130",
  "_Game_20200809T093214",
  "_Game_20200809T093244",
  "_Game_20200809T093449",
  "_Game_20210213T120451",
  "_Game_20210213T120836",
  "_Game_20210213T121155",
  "_Game_20210213T121502",
  "_Game_20210213T121751",
  "_Game_20210213T121936",
  "_Game_20210213T122224",
  "_Game_20210213T122510",
  "_Game_20210213T122846",
  "_Game_20210213T123257"
};

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
    << "Usage: slippc-tests [-d <debuglevel]:" << std::endl
    // << "  -i        Parse and analyze <infile> (not very useful on its own)" << std::endl
    // << "  -j        Output <infile> in .json format to <jsonfile> (use \"-\" for stdout)" << std::endl
    // << "  -a        Output an analysis of <infile> in .json format to <analysisfile> (use \"-\" for stdout)" << std::endl
    // << "  -f        When used with -j <jsonfile>, write full frame info (instead of just frame deltas)" << std::endl
    // << "  -x        Compress or decompress a replay" << std::endl
    // << "  -X        Set output file name for compression" << std::endl
    // << std::endl
    // << "Debug options:" << std::endl
    << "  -d        Run tests at level <debuglevel>" << std::endl
    // << "  --raw-enc Output raw encodes with -x (DANGEROUS, debug only)" << std::endl
    // << "  -h        Show this help message" << std::endl
    ;
}

void testTester() {
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
}

int testTestFiles() {
  // TSUITE("Files Available");
  return 0;
}

int testKnownFiles() {
  slip::Parser *p;

  TSUITE("Known File Parsing");
    ASSERT("File Exists",access( (TSLPPATH+TSLPFILE).c_str(), F_OK ) == 0,
      "File does not exist");
    BAILONFAIL(1);
    std::string test_md5 = md5file(TSLPPATH+TSLPFILE);
    ASSERT("MD5 of file is 8ba0485603d5d99cdfd8cead63ba6c1f",test_md5.compare("8ba0485603d5d99cdfd8cead63ba6c1f") == 0,
      "MD5 of file is " << test_md5);
    BAILONFAIL(1);
    p = new slip::Parser(0);
    ASSERT("File Parses",p->load((TSLPPATH+TSLPFILE).c_str()),
      "File does not parse");
    BAILONFAIL(1);
    SlippiReplay* r = p->replay();
    ASSERT("File version is 3.9.0",r->slippi_version.compare("3.9.0") == 0,
      "File version is " << r->slippi_version);
    ASSERT("First frame is -123",r->first_frame == -123,
      "First frame is " << r->first_frame);
    ASSERT("Frame count is 13662",r->frame_count == 13662,
      "Frame count is " << r->frame_count);
    ASSERT("Seed is 3969935363",r->seed == 3969935363,
      "Seed is " << r->seed);
    ASSERT("Played on Nintendont",r->played_on.compare("nintendont") == 0,
      "Game played on " << r->played_on);
    ASSERT("Played on date 2021-12-13T10:55:05",r->start_time.compare("2021-12-13T10:55:05") == 0,
      "Game played on date " << r->start_time);
    ASSERT("Port 1 is empty",r->player[0].player_type == 3,
      "Port 1 is not empty");
    ASSERT("Port 2 is empty",r->player[1].player_type == 3,
      "Port 2 is not empty");
    ASSERT("Port 3 played Fox",r->player[2].ext_char_id == 2,
      "Port 3 played " << r->player[2].ext_char_id << " (" << CharExt::name[r->player[2].ext_char_id] << ")");
    ASSERT("Port 4 played Marth",r->player[3].ext_char_id == 9,
      "Port 4 played " << r->player[3].ext_char_id << " (" << CharExt::name[r->player[3].ext_char_id] << ")");
    ASSERT("Game played on Dream Land",r->stage == 28,
      "Game played on" << r->stage << " (" << Stage::name[r->stage] << ")");
    ASSERT("Port 3's damage on frame 2345 = 9.4%",NEAR(r->player[2].frame[2345].percent_post,9.4f),
      "Port 3's damage on frame 2345 = " << r->player[2].frame[2345].percent_post);
    ASSERT("Port 4's damage on frame 2345 = 117.46%",NEAR(r->player[3].frame[2345].percent_post,117.46f),
      "Port 4's damage on frame 2345 = " << r->player[3].frame[2345].percent_post);
    ASSERT("Port 3's joystick x on frame 4444 = -0.975",NEAR(r->player[2].frame[4444].joy_x,-0.975f),
      "Port 3's joystick x on frame 4444 = " << r->player[2].frame[4444].joy_x);
    ASSERT("Port 4's joystick y on frame 4444 = 0",NEAR(r->player[3].frame[4444].joy_y,0.0f),
      "Port 4's joystick y on frame 4444 = " << r->player[3].frame[4444].joy_y);
    ASSERT("Port 3's y pos on frame 4444 = 0.0089",NEAR(r->player[2].frame[4444].pos_y_post,0.0089f),
      "Port 3's y pos on frame 4444 = " << r->player[2].frame[4444].pos_y_post);
    ASSERT("Port 4's x pos on frame 4444 = 0.0089",NEAR(r->player[3].frame[4444].pos_x_post,-22.0653),
      "Port 4's x pos on frame 4444 = " << r->player[3].frame[4444].pos_x_post);
    ASSERT("Port 3 has 3 stocks on frame 4444",r->player[2].frame[4444].stocks == 3,
      "Port 3 has " << r->player[2].frame[4444].stocks << " stocks on frame 4444");
    ASSERT("Port 4 has 3 stocks on frame 4444",r->player[3].frame[4444].stocks == 3,
      "Port 4 has " << r->player[3].frame[4444].stocks << " stocks on frame 4444");
    ASSERT("Port 3 is in action 'Turn' on frame 5000",r->player[2].frame[5000].action_post == 18,
      "Port 3 is in action " << r->player[2].frame[5000].action_post << " = " << Action::name[r->player[2].frame[5000].action_post]);
    ASSERT("Port 3 is in action 'DamageFlyLw' on frame 6000",r->player[2].frame[6000].action_post == 89,
      "Port 3 is in action " << r->player[2].frame[6000].action_post << " = " << Action::name[r->player[2].frame[6000].action_post]);
    ASSERT("Port 3 is in action 'DamageFlyRoll' on frame 7000",r->player[2].frame[7000].action_post == 91,
      "Port 3 is in action " << r->player[2].frame[7000].action_post << " = " << Action::name[r->player[2].frame[7000].action_post]);
    ASSERT("Port 3 is in action 'Catch' on frame 8000",r->player[2].frame[8000].action_post == 212,
      "Port 3 is in action " << r->player[2].frame[8000].action_post << " = " << Action::name[r->player[2].frame[8000].action_post]);
    ASSERT("Port 3 is in action 'AttackAirN' on frame 9000",r->player[2].frame[9000].action_post == 65,
      "Port 3 is in action " << r->player[2].frame[9000].action_post << " = " << Action::name[r->player[2].frame[9000].action_post]);
    ASSERT("Port 3 is in action 'EscapeN' on frame 10000",r->player[2].frame[10000].action_post == 350,
      "Port 3 is in action " << r->player[2].frame[10000].action_post << " = " << Action::name[r->player[2].frame[10000].action_post]);
    ASSERT("Port 4 is in action 'Fall' on frame 5000",r->player[3].frame[5000].action_post == 29,
      "Port 4 is in action " << r->player[3].frame[5000].action_post << " = " << Action::name[r->player[3].frame[5000].action_post]);
    ASSERT("Port 4 is in action 'AttackAirF' on frame 6000",r->player[3].frame[6000].action_post == 66,
      "Port 4 is in action " << r->player[3].frame[6000].action_post << " = " << Action::name[r->player[3].frame[6000].action_post]);
    ASSERT("Port 4 is in action 'KneeBend' on frame 7000",r->player[3].frame[7000].action_post == 24,
      "Port 4 is in action " << r->player[3].frame[7000].action_post << " = " << Action::name[r->player[3].frame[7000].action_post]);
    ASSERT("Port 4 is in action 'JumpF' on frame 8000",r->player[3].frame[8000].action_post == 25,
      "Port 4 is in action " << r->player[3].frame[8000].action_post << " = " << Action::name[r->player[3].frame[8000].action_post]);
    ASSERT("Port 4 is in action 'GuardSetOff' on frame 9000",r->player[3].frame[9000].action_post == 181,
      "Port 4 is in action " << r->player[3].frame[9000].action_post << " = " << Action::name[r->player[3].frame[9000].action_post]);
    ASSERT("Port 4 is in action 'JumpF' on frame 10000",r->player[3].frame[10000].action_post == 25,
      "Port 4 is in action " << r->player[3].frame[10000].action_post << " = " << Action::name[r->player[3].frame[10000].action_post]);
    delete p;

  TSUITE("Known File Compression");
    //Removing existing temporary zlp and slp files
    if (fileExists(TZLPFILE.c_str())) {
      remove(TZLPFILE.c_str());
    }
    if (fileExists(TUNZLPFILE.c_str())) {
      remove(TUNZLPFILE.c_str());
    }
    ASSERT("Uncompressed File Exists",access( (TSLPPATH+TCMPFILE).c_str(), F_OK ) == 0,
      "File does not exist");
    BAILONFAIL(1);
    std::string test_md5_2 = md5file(TSLPPATH+TCMPFILE);
    ASSERT("MD5 of uncompressed file is 7ea1aa5b49f87ab77a66bd8541810d50",test_md5_2.compare("7ea1aa5b49f87ab77a66bd8541810d50") == 0,
      "MD5 of file is " << test_md5_2);
    BAILONFAIL(1);
    slip::Compressor *c = new slip::Compressor(0);
    c->setOutputFilename(TZLPFILE.c_str());
    bool loaded = c->loadFromFile((TSLPPATH+TCMPFILE).c_str());
    ASSERTNOERR("Compressor Loads File",loaded,
      "Compressor failed to load known file");
    BAILONFAIL(1);
    ASSERT("Compressor Validates File",c->validate(),
      "Compressor failed to validate known file");
    BAILONFAIL(1);
    c->saveToFile(false);
    ASSERT("Compressed File is Generated",access( TZLPFILE.c_str(), F_OK ) == 0,
      "Compressor failed to save compressed output file");
    BAILONFAIL(1);
    delete c;

    std::string test_md5_z = md5file(TZLPFILE.c_str());
    SUGGEST("MD5 of compressed file is 18f1fbc76e6e40edb185590c0fc9c0d4",test_md5_z.compare("18f1fbc76e6e40edb185590c0fc9c0d4") == 0,
      "MD5 of file is " << test_md5_z << ", compression algorithm may have changed");

    c = new slip::Compressor(0);
    ASSERT("Compressor Loads Compressed File",c->loadFromFile(TZLPFILE.c_str()),
      "Compressor failed to load compressed known file");
    BAILONFAIL(1);
    ASSERT("Compressor Validates Compressed File",c->validate(),
      "Compressor failed to validate compressed known file");
    BAILONFAIL(1);
    c->saveToFile(false);
    ASSERT("In-place decompressed File is Generated",access( TUNZLPFILE.c_str(), F_OK ) == 0,
      "Compressor failed to save decompressed output file");
    BAILONFAIL(1);
    delete c;

    std::string test_md5_3 = md5file(TUNZLPFILE.c_str());
    ASSERT("MD5 of restored file is 7ea1aa5b49f87ab77a66bd8541810d50",test_md5_3.compare("7ea1aa5b49f87ab77a66bd8541810d50") == 0,
      "MD5 of restored file is " << test_md5_3);

  return 0;
}

int testCompressionBackcompat() {
  TSUITE("Backwards Compatible Decompression");
    const size_t ncomps = std::extent<decltype(BACKCOMPS)>::value;
    for(unsigned i = 0; i < ncomps; ++i) {
      //Removing existing temporary zlp and slp files
      if (fileExists(TUNZLPFILE.c_str())) {
        remove(TUNZLPFILE.c_str());
      }

      std::string sf = BCSPATH+BACKCOMPS[i]+".slp";
      std::string zf = BCZPATH+BACKCOMPS[i]+".zlp";
      ASSERT(BACKCOMPS[i]+".slp Exists",access( (sf).c_str(), F_OK ) == 0,
        sf << " does not exist");
      BAILONFAIL(1);
      ASSERT(BACKCOMPS[i]+".zlp Exists",access( (zf).c_str(), F_OK ) == 0,
        zf << " does not exist");
      BAILONFAIL(1);
      std::string test_md5_o = md5file(sf);

      slip::Compressor *c = new slip::Compressor(0);
      c->setOutputFilename(TUNZLPFILE.c_str());
      bool loaded = c->loadFromFile(zf.c_str());
      ASSERTNOERR("Compressor Loads "+BACKCOMPS[i],loaded,
        "Compressor failed to load " << BACKCOMPS[i]);
      BAILONFAIL(1);
      ASSERT("Compressor Validates "+BACKCOMPS[i],c->validate(),
        "Compressor failed to validate " << BACKCOMPS[i]);
      BAILONFAIL(1);
      c->saveToFile(false);
      ASSERT("Compressed File for "+BACKCOMPS[i]+" is Generated",access( TUNZLPFILE.c_str(), F_OK ) == 0,
        "Compressor failed to save compressed output file for " << ALLCOMPS[i]);
      BAILONFAIL(1);
      delete c;

      std::string test_md5_r = md5file(TUNZLPFILE.c_str());
      ASSERT("MD5 of restored file for "+BACKCOMPS[i]+" matches original = "+test_md5_o,test_md5_r.compare(test_md5_o) == 0,
        "MD5 of restored file for " << BACKCOMPS[i] << " is " << test_md5_r);
    }
    return 0;
}

int testCompressionVersions() {
  TSUITE("All Version Compression");
    const size_t ncomps = std::extent<decltype(ALLCOMPS)>::value;
    for(unsigned i = 0; i < ncomps; ++i) {
      //Removing existing temporary zlp and slp files
      if (fileExists(TZLPFILE.c_str())) {
        remove(TZLPFILE.c_str());
      }
      if (fileExists(TUNZLPFILE.c_str())) {
        remove(TUNZLPFILE.c_str());
      }
      ASSERT(ALLCOMPS[i]+" Exists",access( (TSLPPATH+ALLCOMPS[i]).c_str(), F_OK ) == 0,
        ALLCOMPS[i] << " does not exist");
      NEXTONFAIL();
      std::string test_md5_o = md5file(TSLPPATH+ALLCOMPS[i]);
      slip::Compressor *c = new slip::Compressor(0);
      c->setOutputFilename(TZLPFILE.c_str());
      bool loaded = c->loadFromFile((TSLPPATH+ALLCOMPS[i]).c_str());
      ASSERTNOERR("Compressor Loads "+ALLCOMPS[i],loaded,
        "Compressor failed to load " << ALLCOMPS[i]);
      NEXTONFAIL();
      ASSERT("Compressor Validates "+ALLCOMPS[i],c->validate(),
        "Compressor failed to validate " << ALLCOMPS[i]);
      NEXTONFAIL();
      c->saveToFile(false);
      ASSERT("Compressed File for "+ALLCOMPS[i]+" is Generated",access( TZLPFILE.c_str(), F_OK ) == 0,
        "Compressor failed to save compressed output file for " << ALLCOMPS[i]);
      NEXTONFAIL();
      delete c;

      c = new slip::Compressor(0);
      ASSERT("Compressor Loads Compressed "+ALLCOMPS[i],c->loadFromFile(TZLPFILE.c_str()),
        "Compressor failed to load compressed " << ALLCOMPS[i]);
      BAILONFAIL(1);
      ASSERT("Compressor Validates Compressed "+ALLCOMPS[i],c->validate(),
        "Compressor failed to validate compressed " << ALLCOMPS[i]);
      BAILONFAIL(1);
      c->saveToFile(false);
      ASSERT("In-place Decompressed File is Generated for "+ALLCOMPS[i],access( TUNZLPFILE.c_str(), F_OK ) == 0,
        "Compressor failed to save decompressed output file for " << ALLCOMPS[i]);
      BAILONFAIL(1);
      delete c;

      std::string test_md5_r = md5file(TUNZLPFILE.c_str());
      ASSERT("MD5 of restored file for "+ALLCOMPS[i]+" matches original = "+test_md5_o,test_md5_r.compare(test_md5_o) == 0,
        "MD5 of restored file for " << ALLCOMPS[i] << " is " << test_md5_r);
    }
  return 0;
}

int runtests(int argc, char** argv) {
  if (cmdOptionExists(argv, argv+argc, "-h")) {
    printUsage();
    return 0;
  }

  int debug          = 0;
  char* dlevel       = getCmdOption(   argv, argv+argc, "-d");
  if (dlevel) {
    if (dlevel[0] >= '0' && dlevel[0] <= '9') {
      debug = dlevel[0]-'0';
    } else {
      std::cerr << "Warning: invalid debug level" << std::endl;
    }
  }

  // testTestFiles();
  // testKnownFiles();
  // testCompressionBackcompat();
  if(debug >= 1) {
    testCompressionVersions();
  }
  TESTRESULTS();

  // int debug          = 0;
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
