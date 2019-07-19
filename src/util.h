#ifndef UTIL_H_
#define UTIL_H_

#include <stdio.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <curses.h>
#include <string>
#include <streambuf>
#include <sstream>
#include <algorithm> //std::replace

#include <cstdarg>
#include <vector>
#include <string>
#include <string.h>

namespace slip {

const unsigned char CHUNKSIZE = 4;

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
inline int16_t  readBE2s(char* array) { return __builtin_bswap16(*((int16_t*)array)); }
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

// int signof(const float x);
// // float activation(const float x);
// std::string format (const char *fmt, ...);
// std::string vformat (const char *fmt, va_list ap);
// void fsleep(int frames);
// unsigned long hexint(std::string hexstring);
// float hexfloat(unsigned long hexint);
// void endianfix(char (&array)[4]);
// bool file_available(const char* fname);

// std::string readAllLines(std::string fname);
// std::string readAllLines(std::string fname, char nullReplacement);
// bool readProcLines(const char* fname, char** buffer, long* filelen);

// //Find the fieldNum'th occurrence of a substring in a string
// std::vector<std::string> splitAndFind(std::string s, std::string delimiter);
// std::string splitAndFind(std::string s, std::string delimiter, unsigned index);
// // void splitAndFind(std::string s, std::string delimiter, std::vector<std::string>& buffer);
// void splitAndFind(std::string s, std::string delimiter, std::string buffer[], unsigned& length);

// inline long long int getTimeInUsecs() {
//   return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
// }

// void call(const char* cmd);

}

#endif /* UTIL_H_ */
