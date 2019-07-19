#include "util.h"

namespace slip {

// int signof(const float x) {
//   return (x < 0) ? -1 : ( ( x > 0) ? 1 : 0);
// }

// // float activation(const float x) {
// //   float drv = tanh(x);
// //   return (1 - drv*drv);
// // }

// //http://stackoverflow.com/questions/69738/c-how-to-get-fprintf-results-as-a-stdstring-w-o-sprintf#69911
// std::string format (const char *fmt, ...) {
//   va_list ap;
//   va_start (ap, fmt);
//   std::string buf = vformat (fmt, ap);
//   va_end (ap);
//   return buf;
// }

// //http://stackoverflow.com/questions/69738/c-how-to-get-fprintf-results-as-a-stdstring-w-o-sprintf#69911
// std::string vformat (const char *fmt, va_list ap) {
//   // Allocate a buffer on the stack that's big enough for us almost
//   // all the time.
//   size_t size = 1024;
//   char buf[size];

//   // Try to vsnprintf into our buffer.
//   va_list apcopy;
//   va_copy (apcopy, ap);
//   unsigned needed = vsnprintf (&buf[0], size, fmt, ap);
//   // NB. On Windows, vsnprintf returns -1 if the string didn't fit the
//   // buffer.  On Linux & OSX, it returns the length it would have needed.

//   if (needed <= size && needed >= 0) {
//       // It fit fine the first time, we're done.
//       return std::string (&buf[0]);
//   } else {
//       // vsnprintf reported that it wanted to write more characters
//       // than we allotted.  So do a malloc of the right size and try again.
//       // This doesn't happen very often if we chose our initial size
//       // well.
//       std::vector <char> buf;
//       size = needed;
//       buf.resize (size);
//       needed = vsnprintf (&buf[0], size, fmt, apcopy);
//       return std::string (&buf[0]);
//   }
// }

// void fsleep(int frames) {
//   usleep(16666*frames);
// }

// unsigned long hexint(std::string hexstring) {
//   return std::stoul(hexstring, nullptr, 16);
// }

// void endianfix(char (&array)[4]) {
//   char tmp;
//   tmp = array[0];
//   array[0] = array[3];
//   array[3] = tmp;
//   tmp = array[1];
//   array[1] = array[2];
//   array[2] = tmp;
// }

// float hexfloat(unsigned long hexint) {
//   float f;
//   memcpy(&f, &hexint, CHUNKSIZE);
//   return f;
// }

// bool file_available(const char* fname) {
//   std::ifstream infile(fname);
//   return infile.good();
// }

// // std::string readAllLines(std::string fname) {
// //   std::ifstream t(fname);
// //   std::string str;

// //   t.seekg(0, std::ios::end);
// //   str.reserve(t.tellg());
// //   t.seekg(0, std::ios::beg);

// //   str.assign((std::istreambuf_iterator<char>(t)),
// //               std::istreambuf_iterator<char>());

// //   return str;

// //   // std::ifstream t(fname);
// //   // std::string str((std::istreambuf_iterator<char>(t)),std::istreambuf_iterator<char>());
// //   // return str;
// // }

// std::string readAllLines(std::string fname) {
//   std::ifstream t(fname,std::ios::in | std::ios::binary);
//   std::stringstream buffer;
//   buffer << t.rdbuf();
//   std::string s = buffer.str();
//   return s;
// }

// std::string readAllLines(std::string fname, char nullReplacement) {
//   std::ifstream t(fname,std::ios::in | std::ios::binary);
//   std::stringstream buffer;
//   buffer << t.rdbuf();
//   std::string s = buffer.str();
//   std::replace( s.begin(), s.end(), '\0', nullReplacement); // replace all nulls with spaces
//   return s;
// }

// // bool readAllLines(const char* fname, char** buffer, long *filelen) {
// //   FILE *fileptr;
// //   *filelen = 0;

// //   fileptr = fopen(fname, "rb");  // Open the file in binary mode
// //   if (fileptr==NULL) {
// //     *filelen = -1;
// //     return false;
// //   }
// //   setvbuf(fileptr, NULL, _IONBF, 0); //Useful for proc???
// //   fseek(fileptr, 0, SEEK_END);   // Jump to the end of the file
// //   *filelen = ftell(fileptr);      // Get the current byte offset in the file
// //   rewind(fileptr);               // Jump back to the beginning of the file

// //   *buffer = NULL;
// //   *buffer = (char *)malloc((*filelen+1)*sizeof(char)); // Enough memory for file + \0
// //   if(*buffer== NULL)  {
// //     fclose(fileptr); // Close the file
// //     free(*buffer); //Error
// //     return false;
// //   }
// //   fread(*buffer, *filelen, 1, fileptr); // Read in the entire file
// //   fclose(fileptr); // Close the file
// //   (*buffer)[*filelen] = '\0';
// //   return true;
// // }

// bool readProcLines(const char* fname, char** buffer, long *filelen) {
//   FILE *fileptr;
//   *filelen = 0;

//   fileptr = fopen(fname, "rb");  // Open the file in binary mode
//   if (fileptr==NULL) {
//     *filelen = -1;
//     return false;
//   }
//   *filelen = 1024;      // Just read 1024 bytes from procfs files

//   *buffer = NULL;
//   *buffer = (char *)malloc((*filelen+1)*sizeof(char)); // Enough memory for file + \0
//   if(*buffer== NULL)  {
//     fclose(fileptr); // Close the file
//     free(*buffer); //Error
//     return false;
//   }
//   fread(*buffer, *filelen, 1, fileptr); // Read in the entire file
//   fclose(fileptr); // Close the file
//   (*buffer)[*filelen] = '\0';
//   return true;
// }

// std::vector<std::string> splitAndFind(std::string s, std::string delimiter) {
//   std::vector<std::string> sv;
//   size_t pos   = 0;
//   size_t start = 0;
//   size_t dlen  = delimiter.length();
//   while ((pos = s.find(delimiter,start)) != std::string::npos) {
//     sv.push_back(s.substr(start, pos-start));
//     start = pos+dlen;
//   }
//   if (pos >= start) {
//     sv.push_back(s.substr(start, pos-start));
//   }
//   return sv;
// }

// std::string splitAndFind(std::string s, std::string delimiter, unsigned index) {
//   size_t pos       = 0;
//   size_t start     = 0;
//   size_t dlen      = delimiter.length();
//   unsigned counter = 0;
//   while ((pos = s.find(delimiter,start)) != std::string::npos) {
//     if (counter == index) {
//       return s.substr(start, pos-start);
//     }
//     ++counter;
//     start = pos+dlen;
//   }
//   if (counter == index) {
//     return s.substr(start, pos-start);
//   }
//   return "";
// }

// // void splitAndFind(std::string s, std::string delimiter, std::vector<std::string>& buffer) {
// //   buffer.clear();
// //   buffer.reserve(50);
// //   size_t pos = 0;
// //   while ((pos = s.find(delimiter)) != std::string::npos) {
// //     buffer.push_back(s.substr(0, pos));
// //     s.erase(0, pos + delimiter.length());
// //   }
// //   if (s.length() > 0) {
// //     buffer.push_back(s);
// //   }
// // }

// void splitAndFind(std::string s, std::string delimiter, std::string buffer[], unsigned& length) {
//   length       = 0;
//   size_t pos   = 0;
//   size_t start = 0;
//   size_t dlen  = delimiter.length();
//   while ((pos = s.find(delimiter,start)) != std::string::npos) {
//     buffer[length] = s.substr(start, pos-start);
//     start = pos+dlen;
//     ++length;
//   }
//   if (pos >= start) {
//     buffer[length] = s.substr(start, pos-start);
//     ++length;
//   }
// }

// // void splitAndFind(std::string s, std::string delimiter, std::string buffer[], unsigned& length) {
// //   length = 0;
// //   size_t pos = 0;
// //   std::string token;
// //   while ((pos = s.find(delimiter)) != std::string::npos) {
// //     buffer[length] = s.substr(0, pos);
// //     ++length;
// //     s.erase(0, pos + delimiter.length());
// //   }
// //   if (s.length() > 0) {
// //     buffer[length] = s;
// //     ++length;
// //   }
// // }

// void call(const char* cmd) {
//   std::array<char, MAX_LINE_LENGTH> buffer;
//   std::string result;
//   std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
//   if (!pipe) {
//     throw std::runtime_error("popen() failed!");
//   }
//   while (!feof(pipe.get())) {
//     if (fgets(buffer.data(), MAX_LINE_LENGTH, pipe.get()) != nullptr) {
//       std::cout << buffer.data();
//     }
//   }
// }

}
