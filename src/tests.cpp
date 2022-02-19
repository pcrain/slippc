#include "tests.h"

// file containing test replays
static const std::string TESTDIR       = "test-replays";
// normal replays
static const std::string STANDARDDIR   = "standard";
// replays that cannot compress
static const std::string CORRUPTDIR    = "corrupt";
// replays that were compressed with old versions of compressor
static const std::string BACKCOMPATDIR = "zlp-compat-1";

// known file 1
static const std::string TSLPFILE      = "3-9-0-singles-irl-summit12.slp.xz";
// known file 2
static const std::string TCMPFILE      = "3-7-0-singles-net.slp.xz";
// temporary slp file
static const std::string TZLPFILE      = "zlptest.zlp";
// temporary zlp file
static const std::string TUNZLPFILE    = "zlptest.slp";

static const std::string tmpzlp        = (PATH(TESTDIR) / PATH(TZLPFILE)).string();
static const std::string tmpunzlp      = (PATH(TESTDIR) / PATH(TUNZLPFILE)).string();

typedef std::filesystem::directory_iterator f_iter;
typedef std::filesystem::directory_entry    f_entry;

static int _debug = 0;

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
    << "Usage: slippc-tests [-t <testlevel] [-d <debuglevel]:" << std::endl
    // << "  -i        Parse and analyze <infile> (not very useful on its own)" << std::endl
    // << "  -j        Output <infile> in .json format to <jsonfile> (use \"-\" for stdout)" << std::endl
    // << "  -a        Output an analysis of <infile> in .json format to <analysisfile> (use \"-\" for stdout)" << std::endl
    // << "  -f        When used with -j <jsonfile>, write full frame info (instead of just frame deltas)" << std::endl
    // << "  -x        Compress or decompress a replay" << std::endl
    // << "  -X        Set output file name for compression" << std::endl
    // << std::endl
    // << "Debug options:" << std::endl
    << "  -t        Run tests at level <testlevel>" << std::endl
    << "  -d        Run debug at level <debuglevel>" << std::endl
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

int sanityCheck(slip::Parser *p, std::string name, std::string path) {
  ASSERT(name+" Exists",access( path.c_str(), F_OK ) == 0,
        name << " does not exist");
  BAILONFAIL(1);
  ASSERT(name+" Parses",p->load(path.c_str()),
    name << " does not parse");
    BAILONFAIL(1);

  const SlippiReplay* r = p->replay();
  ASSERT("  First frame is -123",r->first_frame == -123,
    name+" first frame is " << r->first_frame);
  int playing = -1;
  for(unsigned i = 0; i < 8; ++i) {
    int pt = r->player[i].player_type;
    ASSERT("  Port "+std::to_string(i)+" player type < 4",pt < 4,
      "Port "<<i<<" player type is "<<pt);
    if(pt < 3) {
      playing = i;
    }
  }
  ASSERT("  Game has at least 1 player",playing >= 0,
    "Game has no players");
  BAILONFAIL(1);

  for(unsigned pnum = 0; pnum < 8; ++pnum) {
    if (r->player[pnum].player_type == 3) {
      continue;
    }
    unsigned flag_char = 0, flag_jumps = 0, flag_dmg = 0, flag_shield = 0,
      flag_lcancel = 0, flag_hurt = 0, flag_stocks = 0, flag_stocks_inc = 0;
    unsigned char cid = r->player[pnum].frame[0].char_id;
    bool sheik = ((cid == 7) || (cid == 19));
    for(unsigned f = 1; f < r->frame_count; ++f) {
      SlippiFrame sf = r->player[pnum].frame[f];
      if (sf.action_post > Action::Sleep) {  //if we're not dead
        if (sf.char_id != cid) {
          if(!(sheik && (sf.char_id == 7 || sf.char_id == 19))) {
            ++flag_char;
          }
        }
      }
      if (sf.jumps > 6) {
        ++flag_jumps;
      }
      if ((sf.percent_pre > 1000) || (sf.percent_pre < 0)) {
        ++flag_dmg;
      }
      if (sf.shield >= 61) {
        ++flag_shield;
      }
      if (sf.l_cancel > 2) {
        ++flag_lcancel;
      }
      if (sf.hurt_by > 8) {
        ++flag_hurt;
      }
      if (sf.stocks > 99) {
        ++flag_stocks;
      }
    }
    std::string ps = std::to_string(pnum+1);
    ASSERT("  Port " + ps + " character is consistent throughout game",flag_char==0,
      ps << "'s character changes throughout the game");
    ASSERT("  Port " + ps + " character never has more than 6 jumps",flag_jumps==0,
      ps << " has more than 6 jumps");
    ASSERT("  Port " + ps + " damage is always between 0 and 1000",flag_dmg==0,
      ps << " has weird damage");
    ASSERT("  Port " + ps + " shield is always <= 60",flag_shield==0,
      ps << " has more than 60 shield");
    ASSERT("  Port " + ps + " l cancel status is always <= 2",flag_lcancel==0,
      ps << "'s l cancel status is more than 2");
    ASSERT("  Port " + ps + " hurt by player id always < 8",flag_hurt==0,
      ps << "'s hurt by player >= 8");
    ASSERT("  Port " + ps + " stocks always <= 99",flag_stocks==0,
      ps << "'s stocks > 99");
  }

  return 0;
}

int testCorruptFiles() {
  TSUITE("Corrupt File Sanity Checks");
    int errors = 0;
    for (const f_entry & entry : f_iter(PATH(TESTDIR) / PATH(CORRUPTDIR))) {
      std::string path = entry.path().string();
      std::string name = entry.path().stem().string();
    // for(unsigned i = 0; i < ncomps; ++i) {
      slip::Parser *p = new slip::Parser(_debug);
      if(sanityCheck(p,name,path) != 0) {
        ++errors;
      }
      delete p;
      slip::Compressor *c = new slip::Compressor(_debug);
      bool loaded = c->loadFromFile(path.c_str());
      ASSERTNOERR("Compressor refuses to compress "+name,!loaded,
        "Compressor attempted to compress corrupt file " << name);
      delete c;
    };
  return 0;
}

int testConsistencySanity() {
  TSUITE("Parser Sanity Checks");
    int errors = 0;
    for (const f_entry & entry : f_iter(PATH(TESTDIR) / PATH(STANDARDDIR))) {
      std::string path = entry.path().string();
      std::string name = entry.path().stem().string();
    // for(unsigned i = 0; i < ncomps; ++i) {
      slip::Parser *p = new slip::Parser(_debug);
      if(sanityCheck(p,name,path) != 0) {
        ++errors;
      }
      delete p;
    };
  return 0;
}


int testKnownFiles() {
  std::string known1 = (PATH(TESTDIR) / PATH(STANDARDDIR) / PATH(TSLPFILE)).string();
  std::string known2 = (PATH(TESTDIR) / PATH(STANDARDDIR) / PATH(TCMPFILE)).string();
  slip::Parser *p;

  TSUITE("Known File Parsing");
    ASSERT("File Exists",access( known1.c_str(), F_OK ) == 0,
      "File " << known1 << " does not exist");
    BAILONFAIL(1);
    std::string test_md5 = md5compressed(known1);
    ASSERT("MD5 of file is 8ba0485603d5d99cdfd8cead63ba6c1f",test_md5.compare("8ba0485603d5d99cdfd8cead63ba6c1f") == 0,
      "MD5 of file is " << test_md5);
    BAILONFAIL(1);
    p = new slip::Parser(_debug);
    ASSERT("File Parses",p->load((known1).c_str()),
      "File does not parse");
    BAILONFAIL(1);
    const SlippiReplay* r = p->replay();
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
    if (fileExists(tmpzlp.c_str())) {
      remove(tmpzlp.c_str());
    }
    if (fileExists(tmpunzlp.c_str())) {
      remove(tmpunzlp.c_str());
    }
    ASSERT("Uncompressed File Exists",access( known2.c_str(), F_OK ) == 0,
      "File does not exist");
    BAILONFAIL(1);
    std::string test_md5_2 = md5compressed(known2);
    ASSERT("MD5 of uncompressed file is 7ea1aa5b49f87ab77a66bd8541810d50",test_md5_2.compare("7ea1aa5b49f87ab77a66bd8541810d50") == 0,
      "MD5 of file is " << test_md5_2);
    BAILONFAIL(1);
    slip::Compressor *c = new slip::Compressor(_debug);
    c->setOutputFilename(tmpzlp.c_str());
    bool loaded = c->loadFromFile(known2.c_str());
    ASSERTNOERR("Compressor Loads File",loaded,
      "Compressor failed to load known file");
    BAILONFAIL(1);
    ASSERT("Compressor Validates File",c->validate(),
      "Compressor failed to validate known file");
    BAILONFAIL(1);
    c->saveToFile(false);
    ASSERT("Compressed File is Generated",access( tmpzlp.c_str(), F_OK ) == 0,
      "Compressor failed to save compressed output file");
    BAILONFAIL(1);
    delete c;

    std::string test_md5_z = md5file(tmpzlp.c_str());
    SUGGEST("MD5 of compressed file is 8af440882c30c4ac29b935ae3d588e03",test_md5_z.compare("8af440882c30c4ac29b935ae3d588e03") == 0,
      "MD5 of file is " << test_md5_z << ", compression algorithm may have changed");

    c = new slip::Compressor(_debug);
    ASSERT("Compressor Loads Compressed File",c->loadFromFile(tmpzlp.c_str()),
      "Compressor failed to load compressed known file");
    BAILONFAIL(1);
    ASSERT("Compressor Validates Compressed File",c->validate(),
      "Compressor failed to validate compressed known file");
    BAILONFAIL(1);
    c->saveToFile(false);
    ASSERT("In-place decompressed File is Generated",access( tmpunzlp.c_str(), F_OK ) == 0,
      "Compressor failed to save decompressed output file");
    BAILONFAIL(1);
    delete c;

    std::string test_md5_3 = md5file(tmpunzlp.c_str());
    ASSERT("MD5 of restored file is 7ea1aa5b49f87ab77a66bd8541810d50",test_md5_3.compare("7ea1aa5b49f87ab77a66bd8541810d50") == 0,
      "MD5 of restored file is " << test_md5_3);

  return 0;
}

int testCompressionBackcompat() {
  TSUITE("Backwards Compatible Decompression");
    for (const f_entry & entry : f_iter(PATH(TESTDIR) / PATH(BACKCOMPATDIR))) {
      std::string path        = entry.path().string();
      std::string name        = entry.path().stem().string();
      // md5 checksum is first 32 characters of filename
      std::string test_md5_o  = name.substr(0,32);
      // base name is rest of filename
      name                    = name.substr(33,name.length()-33);

      //Removing existing temporary zlp and slp files
      if (fileExists(tmpunzlp.c_str())) {
        remove(tmpunzlp.c_str());
      }

      ASSERT(name+".zlp Exists",access( path.c_str(), F_OK ) == 0,
        name << " does not exist");
      BAILONFAIL(1);

      slip::Compressor *c = new slip::Compressor(_debug);
      c->setOutputFilename(tmpunzlp.c_str());
      bool loaded = c->loadFromFile(path.c_str());
      ASSERTNOERR("Decompressor Loads "+name,loaded,
        "Decompressor failed to load " << name);
      BAILONFAIL(1);
      ASSERT("Compressor Validates "+name,c->validate(),
        "Decompressor failed to validate " << name);
      BAILONFAIL(1);
      c->saveToFile(false);
      ASSERT("Compressed File for "+name+" is Generated",access( tmpunzlp.c_str(), F_OK ) == 0,
        "Decompressor failed to save compressed output file for " << name);
      BAILONFAIL(1);
      delete c;

      std::string test_md5_r = md5file(tmpunzlp.c_str());
      ASSERT("MD5 of restored file for "+name+" is "+test_md5_o,test_md5_r.compare(test_md5_o) == 0,
        "MD5 of restored file for " << name << " is " << test_md5_r);
    }
    return 0;
}

int testCompressionVersions() {
  slip::Compressor *c;
  TSUITE("All Version Compression");
    for (const f_entry & entry : f_iter(PATH(TESTDIR) / PATH(STANDARDDIR))) {
      std::string path = entry.path().string();
      std::string name = entry.path().stem().string();
      //Removing existing temporary zlp and slp files
      if (fileExists(tmpzlp.c_str())) {
        remove(tmpzlp.c_str());
      }
      if (fileExists(tmpunzlp.c_str())) {
        remove(tmpunzlp.c_str());
      }
      ASSERT(name+" Exists",access( path.c_str(), F_OK ) == 0,
        name << " does not exist");
      NEXTONFAIL();
      std::string test_md5_o = md5compressed(path);

      c = new slip::Compressor(_debug);
      c->setOutputFilename(tmpzlp.c_str());
      bool loaded = c->loadFromFile((path).c_str());
      ASSERTNOERR("Compressor Loads "+name,loaded,
        "Compressor failed to load " << name);
      NEXTONFAIL();
      ASSERT("Compressor Validates "+name,c->validate(),
        "Compressor failed to validate " << name);
      NEXTONFAIL();
      c->saveToFile(false);
      ASSERT("Compressed File for "+name+" is Generated",access( tmpzlp.c_str(), F_OK ) == 0,
        "Compressor failed to save compressed output file for " << name);
      NEXTONFAIL();
      delete c;

      c = new slip::Compressor(_debug);
      ASSERT("Compressor Loads Compressed "+name,c->loadFromFile(tmpzlp.c_str()),
        "Compressor failed to load compressed " << name);
      BAILONFAIL(1);
      ASSERT("Compressor Validates Compressed "+name,c->validate(),
        "Compressor failed to validate compressed " << name);
      BAILONFAIL(1);
      c->saveToFile(false);
      ASSERT("In-place Decompressed File is Generated for "+name,access( tmpunzlp.c_str(), F_OK ) == 0,
        "Compressor failed to save decompressed output file for " << name);
      BAILONFAIL(1);
      delete c;

      std::string test_md5_r = md5file(tmpunzlp.c_str());
      ASSERT("MD5 of restored file for "+name+" is "+test_md5_o,test_md5_r.compare(test_md5_o) == 0,
        "MD5 of restored file for " << name << " is " << test_md5_r);
    }
  return 0;
}

int runtests(int argc, char** argv) {
  if (cmdOptionExists(argv, argv+argc, "-h")) {
    printUsage();
    return 0;
  }

  char* dlevel       = getCmdOption(   argv, argv+argc, "-d");
  char* tlevel       = getCmdOption(   argv, argv+argc, "-t");
  if (dlevel) {
    if (dlevel[0] >= '0' && dlevel[0] <= '9') {
      _debug = dlevel[0]-'0';
    } else {
      std::cerr << "Warning: invalid debug level" << std::endl;
    }
  }
  int testlevel = 0;
  if (tlevel) {
    if (tlevel[0] >= '0' && tlevel[0] <= '9') {
      testlevel = tlevel[0]-'0';
    } else {
      std::cerr << "Warning: invalid test level" << std::endl;
    }
  }

  testTestFiles();
  testKnownFiles();
  testCorruptFiles();
  testCompressionBackcompat();
  testConsistencySanity();
  if(testlevel >= 1) {
    testCompressionVersions();
  }
  TESTRESULTS();

  return 0;
}

}

int main(int argc, char** argv) {
  return slip::runtests(argc,argv);
}
