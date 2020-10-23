#ifndef UTIL_H_
#define UTIL_H_

#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <codecvt>
#include <algorithm> //std::find

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
inline uint32_t readBE4U(char* array) { return __builtin_bswap32(*((uint32_t*)array)); }
//Load a big-endian 16-bit unsigned int from an array
inline uint16_t readBE2U(char* array) { return __builtin_bswap16(*((uint16_t*)array)); }
//Load a big-endian 32-bit int from an array
inline int32_t  readBE4S(char* array) { return __builtin_bswap32(*((int32_t*)array)); }
//Load a big-endian 16-bit int from an array
inline int16_t  readBE2S(char* array) { return __builtin_bswap16(*((int16_t*)array)); }
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
        if (*c == '"' || *c == '\\' || ('\x00' <= *c && *c <= '\x1f')) {
            o << "\\u"
              << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
        } else {
            o << *c;
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

inline std::string floatToBinary(float f)
{
    union { float f; uint32_t i; } u;
    u.f = f;
    std::string s;

    for (int i = 0; i < 32; i++)
    {
        if (u.i % 2)  s.push_back('1');
        else s.push_back('0');
        u.i >>= 1;
    }

    // Reverse the string since now it's backwards
    std::string temp(s.rbegin(), s.rend());
    return temp;
}

}

#endif /* UTIL_H_ */
