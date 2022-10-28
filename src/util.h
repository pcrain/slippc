#ifndef UTIL_H_
#define UTIL_H_

#if defined(__GNUC__) || defined(__GNUG__)
#define swap32 __builtin_bswap32
#define swap16 __builtin_bswap16
#elif defined(_MSC_VER)
#define swap32 _byteswap_ulong
#define swap16 _byteswap_ushort
#include <intrin.h>
#endif

#include <string.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <codecvt>
#include <algorithm> //std::find
#include <sys/stat.h> //std::find
#include <filesystem>

#include "lzma.h"
#include "picohash.h"
#include "shiftjis.h"

#define BYTE8(b1,b2,b3,b4,b5,b6,b7,b8) (*((uint64_t*)(uint8_t[]){b1,b2,b3,b4,b5,b6,b7,b8}))
#define BYTE4(b1,b2,b3,b4)             (*((uint32_t*)(uint8_t[]){b1,b2,b3,b4}))
#define BYTE2(b1,b2)                   (*((uint16_t*)(uint8_t[]){b1,b2}))

const uint64_t SLP_HEADER  = BYTE8(0x7b,0x55,0x03,0x72,0x61,0x77,0x5b,0x24); // {U.raw[$
const uint32_t LZMA_HEADER = BYTE4(0xfd,0x37,0x7a,0x58);

const unsigned N_HEADER_BYTES      =  15; //Header is always 15 bytes
const unsigned MIN_EV_PAYLOAD_SIZE =  14; //Payloads, game start, pre frame, post frame, game end always defined
const unsigned MIN_GAME_START_SIZE = 353; //Minimum size for game start event (necessary for all replays)
const unsigned MIN_REPLAY_LENGTH   = N_HEADER_BYTES + MIN_EV_PAYLOAD_SIZE + MIN_GAME_START_SIZE;

// Version convenience macros
#define MIN_VERSION(maj,min,rev) (_slippi_maj > (maj)) || (_slippi_maj == (maj) && ( (_slippi_min > (min)) || (_slippi_min == (min) && _slippi_rev >= (rev)) ))
#define MAX_VERSION(maj,min,rev) (_slippi_maj < (maj)) || (_slippi_maj == (maj) && ( (_slippi_min < (min)) || (_slippi_min == (min) && _slippi_rev < (rev)) ))
#define GET_VERSION() (std::to_string(int(_slippi_maj))+"."+std::to_string(int(_slippi_min))+"."+std::to_string(int(_slippi_rev)))
#define ENCODE_VERSION_MIN(min) (_encode_ver == 0 || _encode_ver >= min)  //also allows unencoded files

// Convenience pseudo-typedefs
#define PATH std::filesystem::path

// Variable checking whether error log has been initialized
static bool errlog_init = false;

//Debug output convenience macros
#define DOUT1(s) if (_debug >= 1) { std::cerr << "  " << BLU << "DEBUG 1: " << BLN << s << std::endl; }
#define DOUT2(s) if (_debug >= 2) { std::cerr << "  " << BLU << "DEBUG 2: " << BLN << s << std::endl; }
#define DOUT3(s) if (_debug >= 3) { std::cerr << "  " << BLU << "DEBUG 3: " << BLN << s << std::endl; }
#define INFO(e)                     std::cerr << "  " << GRN << "   INFO: " << BLN << e << std::endl;
#define WARN(e)                     std::cerr << "  " << YLW << "WARNING: " << BLN << e << std::endl;
#define FAIL(e)                     std::cerr << "  " << RED << "  ERROR: " << BLN << e << std::endl;
#define YIKES(e)                    std::cerr << "  " << CRT << "  YIKES: " << BLN << e << std::endl;
#define WARN_CORRUPT(e)             std::cerr << "  " << YLW << "WARNING: " << BLN << e << "; replay may be corrupt"   << std::endl;
#define FAIL_CORRUPT(e)             std::cerr << "  " << RED << "  ERROR: " << BLN << e << "; cannot continue parsing" << std::endl;
#define _LOG(e) \
  std::cerr << "  " << MGN << "    LOG: " << BLN << e << std::endl;
#define LOG(e,f) {\
  std::cerr << "  " << MGN << "    LOG: " << BLN << e << std::endl; \
  std::ofstream log(f, std::ios_base::app | std::ios_base::out); \
  log << "  [" << timestamp() << "] " << e << std::endl; \
  log.close(); };
#define ERRLOG(path,e) \
  PATH errorpath = (path / "_errors.txt"); \
  if (!errlog_init) { \
    errlog_init = true; \
    _LOG("Logging errors to " << MGN << errorpath << BLN); \
    LOG("Log opened",errorpath); \
  } \
  LOG(e,errorpath);

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
    #define CRT ""
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
    #define CRT "\u001b[41m"
    #define BLN "\u001b[0m"
#endif

namespace slip {

//Indent Level
#define ILEV 1

//Strings for various indentation amounts
const std::string SPACE[10] = {
  "",
  " ",
  "  ",
  "   ",
  "    ",
  "     ",
  "      ",
  "       ",
  "        ",
  "         ",
};

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

//Hex Output stuff

struct HexCharStruct {
  unsigned char c;
  HexCharStruct(unsigned char _c) : c(_c) { }
};

inline std::ostream& operator<<(std::ostream& o, const HexCharStruct& hs) {
  return (o << "0x" << std::hex << (int)hs.c) << std::dec;
}

inline HexCharStruct hex(unsigned char _c) {
  return HexCharStruct(_c);
}

inline void printBytes(char* buff, unsigned nchars) {
  for(unsigned i = 0; i < nchars; ++i) {
    std::cout << hex(buff[i]) << " ";
  }
  std::cout << std::dec << std::endl;
}

//Bitwise comparison of 8-bytes array with uint_64
inline bool same8(char* array, uint64_t other) {
  return ((*((uint64_t*)array)) ^ other) == 0;
}
//Bitwise comparison of 4-bytes array with uint_32
inline bool same4(char* array, uint32_t other) {
  return ((*((uint32_t*)array)) ^ other) == 0;
}
//Load a big-endian 32-bit unsigned int from an array
inline uint32_t readBE4U(char* array) { return swap32(*((uint32_t*)array)); }
//Load a big-endian 16-bit unsigned int from an array
inline uint16_t readBE2U(char* array) { return swap16(*((uint16_t*)array)); }
//Load a big-endian 32-bit int from an array
inline int32_t  readBE4S(char* array) { return swap32(*((int32_t*)array)); }
//Load a big-endian 16-bit int from an array
inline int16_t  readBE2S(char* array) { return swap16(*((int16_t*)array)); }
//Load a big-endian float from an array
inline float readBE4F(char* array) {
   float r;
   char *rc = ( char* ) & r;
   rc[0] = array[3];
   rc[1] = array[2];
   rc[2] = array[1];
   rc[3] = array[0];
   return r;
}

//Write a big-endian 32-bit unsigned int to an array
inline void  writeBE4F(float f, char* a) {
  char *wc = ( char* ) & f;
  a[3] = wc[0];
  a[2] = wc[1];
  a[1] = wc[2];
  a[0] = wc[3];
}

//Write a big-endian 16-bit unsigned int to an array
inline void  writeBE2U(uint16_t i, char* a) {
  a[1] =  i      & 0xff;
  a[0] = (i>>8)  & 0xff;
}

//Write a big-endian 32-bit unsigned int to an array
inline void  writeBE4U(uint32_t i, char* a) {
  a[3] =  i      & 0xff;
  a[2] = (i>>8)  & 0xff;
  a[1] = (i>>16) & 0xff;
  a[0] = (i>>24) & 0xff;
}

//Write a big-endian 32-bit signed int to an array
inline void  writeBE4S(int32_t i, char* a) {
  a[3] =  i      & 0xff;
  a[2] = (i>>8)  & 0xff;
  a[1] = (i>>16) & 0xff;
  a[0] = (i>>24) & 0xff;
}

//Base64 encoder / decoder -> https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp
inline std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = ( char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }

  return ret;

}

//Base64 encoder / decoder -> https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp
inline std::string base64_decode(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = ( char_array_4[0] << 2       ) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = 0; j < i; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}

// https://stackoverflow.com/questions/7724448/simple-json-string-escape-for-c/33799784#33799784
inline std::string escape_json(const std::string &s) {
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
        case '"' : o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:
            if ('\x00' <= *c && *c <= '\x1f') {
                o << "\\u"
                  << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
            } else {
                o << *c;
            }
        }
    }
    return o.str();
}

inline std::string to_utf8(const std::u16string &s) {
  std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> conv;
  std::string u = conv.to_bytes(s);
  u.erase(std::find(u.begin(), u.end(), '\0'), u.end());
  return u;
}

inline std::string decode_shift_jis(char* addr) {
  std::u16string tag;
  for(unsigned n = 0; n < 16; n+=2) {
    uint16_t b = (readBE2U(addr+n));
    if ((b>>8) == 0x82) {
      if ( (b&0xff) < 0x59) { //Roman numerals
        b -= 0x821f;
      } else if ( (b&0xff) < 0x80) { //Roman letters
        b -= 0x81ff;
      } else { //Hiragana
        b -= 0x525e;
      }
    } else if ((b>>8) == 0x83) {
      if ( (b&0xff) < 0x80) { //Katakana, part 1
        b -= 0x529f;
      } else { //Katakana, part 2
        b -= 0x52a0;
      }
    } else if ((b>>8) == 0x81) {  //Miscellaneous characters
      switch(b&0xff) {
        case(0x81): b = 0x003D; break; // =
        case(0x5b): b = 0x30FC; break; // Hiragana Wave Dash (lol)
        case(0x7c): b = 0x002D; break; // -
        case(0x7b): b = 0x002B; break; // +
        case(0x49): b = 0x0021; break; // !
        case(0x48): b = 0x003F; break; // ?
        case(0x97): b = 0x0040; break; // @
        case(0x93): b = 0x0025; break; // %
        case(0x95): b = 0x0026; break; // &
        case(0x90): b = 0x0024; break; // $
        case(0x44): b = 0x002E; break; // .
        case(0x60): b = 0x301C; break; // Japanese tilde
        case(0x42): b = 0x3002; break; // Japanese period
        case(0x41): b = 0x3001; break; // Japanese comma
        case(0x40): b = 0x3000; break; // Space
        default:
          // DOUT1("    Encountered unknown character in tag: " << b << std::endl);
          b = 0;
          break;
      }
    } else {
      if (b != 0) {
        // DOUT1("    Encountered unknown character in tag: " << b << std::endl);
      }
      b = 0;
    }
    tag += b;
    if (b == 0) {
      break;
    }
  }
  return to_utf8(tag);
}

// From: https://stackoverflow.com/questions/33165171/c-shiftjis-to-utf8-conversion
inline std::string sj2utf8(const std::string &input) {
    std::string output(3 * input.length(), ' '); //ShiftJis won't give 4byte UTF8, so max. 3 byte per input char are needed
    size_t indexInput = 0, indexOutput = 0;

    while(indexInput < input.length())
    {
        char arraySection = ((uint8_t)input[indexInput]) >> 4;

        size_t arrayOffset;
        if(arraySection == 0x8) arrayOffset = 0x100; //these are two-byte shiftjis
        else if(arraySection == 0x9) arrayOffset = 0x1100;
        else if(arraySection == 0xE) arrayOffset = 0x2100;
        else arrayOffset = 0; //this is one byte shiftjis

        //determining real array offset
        if(arrayOffset)
        {
            arrayOffset += (((uint8_t)input[indexInput]) & 0xf) << 8;
            indexInput++;
            if(indexInput >= input.length()) break;
        }
        arrayOffset += (uint8_t)input[indexInput++];
        arrayOffset <<= 1;

        //unicode number is...
        uint16_t unicodeValue = (shiftJIS_convTable[arrayOffset] << 8) | shiftJIS_convTable[arrayOffset + 1];

        //converting to UTF8
        if(unicodeValue < 0x80)
        {
            output[indexOutput++] = unicodeValue;
        }
        else if(unicodeValue < 0x800)
        {
            output[indexOutput++] = 0xC0 | (unicodeValue >> 6);
            output[indexOutput++] = 0x80 | (unicodeValue & 0x3f);
        }
        else
        {
            output[indexOutput++] = 0xE0 | (unicodeValue >> 12);
            output[indexOutput++] = 0x80 | ((unicodeValue & 0xfff) >> 6);
            output[indexOutput++] = 0x80 | (unicodeValue & 0x3f);
        }
    }

    output.resize(indexOutput); //remove the unnecessary bytes
    return output;
}

inline std::string parseConnCode(const std::string &input) {
  std::string  s = sj2utf8(input);
  size_t index = s.find("＃", 0);  //repace 2-byte shift-jis # with ascii one
  if (index != std::string::npos) {
    s.replace(index, 3, "#");
  }
  return s;
}

inline std::string floatToBinary(float f) {
  union { float f; uint32_t i; } u;
  u.f = f;
  std::string s;
  for (int i = 0; i < 32; i++) {
    if (u.i % 2)  s.push_back('1');
    else s.push_back('0');
    u.i >>= 1;
  }
  // Reverse the string since now it's backwards
  std::string temp(s.rbegin(), s.rend());
  return temp;
}

// http://ptspts.blogspot.com/2011/11/how-to-simply-compress-c-string-with.html
inline std::string compressWithLzma(const char* in, const size_t inlen, int level = 6) {
  std::string result;
  result.resize(inlen + (inlen >> 2) + 128);
  size_t out_pos = 0;
  if (LZMA_OK != lzma_easy_buffer_encode(
      level, LZMA_CHECK_CRC32, NULL,
      reinterpret_cast<const uint8_t*>(in), inlen,
      reinterpret_cast<uint8_t*>(&result[0]), &out_pos, result.size()))
    abort();
  result.resize(out_pos);
  return result;
}

inline std::string decompressWithLzma(const uint8_t* in, const size_t inlen) {
  static const size_t kMemLimit = 1 << 30;  // 1 GB.
  lzma_stream strm = LZMA_STREAM_INIT;
  std::string result;
  result.resize(8192);
  size_t result_used = 0;
  lzma_ret ret;
  ret = lzma_stream_decoder(&strm, kMemLimit, LZMA_CONCATENATED);
  if (ret != LZMA_OK)
    abort();
  size_t avail0 = result.size();
  strm.next_in = in;
  strm.avail_in = inlen;
  strm.next_out = reinterpret_cast<uint8_t*>(&result[0]);
  strm.avail_out = avail0;
  while (true) {
    ret = lzma_code(&strm, strm.avail_in == 0 ? LZMA_FINISH : LZMA_RUN);
    if (ret == LZMA_STREAM_END) {
      result_used += avail0 - strm.avail_out;
      if (0 != strm.avail_in)  // Guaranteed by lzma_stream_decoder().
        abort();
      result.resize(result_used);
      lzma_end(&strm);
      return result;
    }
    if (ret != LZMA_OK)
      abort();
    if (strm.avail_out == 0) {
      result_used += avail0 - strm.avail_out;
      result.resize(result.size() << 1);
      strm.next_out = reinterpret_cast<uint8_t*>(&result[0] + result_used);
      strm.avail_out = avail0 = result.size() - result_used;
    }
  }
}

inline std::string decompressWithLzma(const char* in, const size_t inlen) {
  return decompressWithLzma(reinterpret_cast<const uint8_t*>(&in[0]),inlen);
}

inline bool fileExists(std::string fname) {
   std::ifstream i(fname.c_str());
   return i.good();
}

// https://stackoverflow.com/questions/667183/padding-stl-strings-in-c
inline std::string padString(std::string str, const size_t num, const char paddingChar = ' ') {
    if(num > str.size())
        str.insert(0, num - str.size(), paddingChar);
    return str;
}

inline std::string padString(float f, const size_t num, const char paddingChar = ' ') {
  return padString(std::to_string(f),num,paddingChar);
}

inline std::string padString(int i, const size_t num, const char paddingChar = ' ') {
  return padString(std::to_string(i),num,paddingChar);
}


inline int diffBits(uint16_t u1, uint16_t u2) {
  int count  = 0;
  u1        ^= u2;
  for (int i = 0; i < 16; i++) {
    if ((u1 >> i) & 1) {
      ++count;
    }
  }
  return count;
}

inline int countBits(uint16_t u1) {
  return diffBits(u1,0);
}

inline int diffBits(float f1, float f2) {
  union { float f; uint32_t u; } n1, n2;
  n1.f       = f1;
  n2.f       = f2;
  int count  = 0;
  n1.u      ^= n2.u;
  for (int i = 0; i < 32; i++) {
    if ((n1.u >> i) & 1) {
      ++count;
    }
  }
  return count;
}

inline std::string getFileBase(std::string f) {
  int extpos = f.find_last_of(".");
  return f.substr(0,extpos);
}


inline std::string getFileExt(std::string f) {
  int extpos = f.find_last_of(".");
  return f.substr(extpos+1,f.size()-(extpos+1));
}

inline void stringtoChars(std::string s, char** c) {
  *c = new char[s.size()+1];
  strcpy(*c, s.c_str());
}

inline std::string md5tostring(unsigned char* digest) {
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (int i = 0; i < 16; ++i) {
      ss << std::setw(2) << static_cast<unsigned>(digest[i]);
  }
  return ss.str();
}

inline std::string md5data(unsigned char* buffer, size_t length) {
  picohash_ctx_t ctx;
  unsigned char digest[PICOHASH_MD5_DIGEST_LENGTH];
  picohash_init_md5(&ctx);
  picohash_update(&ctx, buffer, length);
  picohash_final(&ctx, digest);

  return md5tostring(digest);
}

inline std::string md5file(std::string fname, bool compressed = false) {
  FILE* f = fopen(fname.c_str(),"r");
  fseek(f, 0, SEEK_END);
  size_t length = ftell(f);
  fseek(f, 0, SEEK_SET);
  unsigned char* b = (unsigned char*)malloc(length);
  fread(b,1,length,f);
  fclose(f);

  if (compressed) {
    std::string decomp = decompressWithLzma(b, length);
    // Get the new file size
    length = decomp.size();
    // Delete the old read buffer
    free(b);
    // Reallocate it with more spce
    b = (unsigned char*)malloc(length);
    // Copy buffer from the decompressed string
    memcpy(b,decomp.c_str(),length);
  }

  std::string m = md5data(b, length);

  free(b);

  return m;
}

inline std::string md5compressed(std::string fname) {
  return md5file(fname,true);
}

inline bool isDirectory(const char* path) {
  struct stat s;
  if( stat(path,&s) == 0 ) {
    if( s.st_mode & S_IFDIR ) {
      return true;
    }
  }
  return false;
}

inline bool makeDirectoryIfNotExists(const char* path) {
  struct stat s;
  if( stat(path,&s) == 0 ) {
    if( s.st_mode & S_IFDIR ) {
      return true;  //path exists and is a directory
    }
    return false;  //path exists but is not a directory
  }
  return std::filesystem::create_directories(path);
}

inline auto timestamp() {
  std::time_t time_now = std::time(nullptr);
  return std::put_time(std::localtime(&time_now), "%Y-%m-%d %OH:%OM:%OS");
}

inline bool ensureExt(const char* ext, const char* fname) {
  std::string fs(fname);
  return (fs.substr(fs.length()-4,4).compare(ext) == 0);
}

}

#endif /* UTIL_H_ */
