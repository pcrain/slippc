#ifndef TESTS_H_
#define TESTS_H_

#include <string>
#include <filesystem>

#include "util.h"
#include "parser.h"
#include "analyzer.h"
#include "compressor.h"

#ifdef _WIN32
#include <Windows.h> //sleep()
#else
#include <unistd.h> //sleep()
#endif

#include <algorithm>

namespace slip {

typedef struct _test_suite {
  const char* name;
  unsigned p = 0;  //pass
  unsigned w = 0;  //warn
  unsigned f = 0;  //fail
  unsigned e = 0;  //error
  unsigned t = 0;  //total
  _test_suite* next;
} test_suite;

static unsigned __tests_total  = 0;
static unsigned __tests_passed = 0;
static unsigned __tests_warned = 0;
static unsigned __tests_failed = 0;
static unsigned __tests_errors = 0;
static test_suite* suites = NULL, *tail = NULL;
static test_suite* s;

static bool __test_passed__ = 1;

#define NEART(x,y,t) \
  ((((y)-(t)) <= (x)) && (((y)+(t)) >= (x)))
#define NEAR(x,y) NEART((x),(y),0.001f)

#define TSUITE(tag) \
  std::cout << "----------\nTesting suite " << CYN << (tag) << BLN << std::endl; \
  s = (test_suite*)calloc(1,sizeof(test_suite)); \
  s->name = (tag); \
  if(!tail) {suites     = s;} \
  else      {tail->next = s;} \
  tail = s;

#define TPASS() \
  std::cout << "\r " << GRN << " pass\n" << BLN; \
  ++__tests_total; \
  ++__tests_passed; \
  ++(tail->t); \
  ++(tail->p);
#define TWARN() \
  std::cout << "\r " << YLW << " warn\n" << BLN; \
  ++__tests_total; \
  ++__tests_warned; \
  ++(tail->t); \
  ++(tail->w);
#define TFAIL() \
  std::cout << "\r " << RED << " fail\n" << BLN; \
  ++__tests_total; \
  ++__tests_failed; \
  ++(tail->t); \
  ++(tail->f);
#define TERR(e) \
  std::cout << "\r " << CRT << "error\n" << BLN; \
  ++__tests_total; \
  ++__tests_errors; \
  ++(tail->t); \
  ++(tail->e);

#define BAILONFAIL(r) if (!__test_passed__) { return (r); }
#define NEXTONFAIL()  if (!__test_passed__) { continue; }

#define ASSERTNOERR(name,expr,onfail) \
  std::cout << "       " << (name) << BLN; \
  if ((expr)) { TPASS(); __test_passed__ = 1; } else { TFAIL();  __test_passed__ = 0; std::cout << "    " << RED << onfail << BLN << " (Asserted: " << RED << #expr << BLN << ")" << std::endl;}

#define ASSERT(name,expr,onfail) \
  std::cout << "       " << (name) << BLN; \
  try { \
    if ((expr)) { TPASS(); __test_passed__ = 1; } else { TFAIL();  __test_passed__ = 0; std::cout << "    " << RED << onfail << BLN << " (Asserted: " << RED << #expr << BLN << ")" << std::endl;} \
  } catch (const std::exception& e) { TERR(e);  __test_passed__ = 0; }

#define SUGGEST(name,expr,onfail) \
  std::cout << "       " << (name) << BLN; \
  try { \
    if ((expr)) { TPASS(); __test_passed__ = 1; } else { TWARN();  __test_passed__ = 0; std::cout << "    " << YLW << onfail << BLN << " (Suggested: " << YLW << #expr << BLN << ")" << std::endl;} \
  } catch (const std::exception& e) { TERR(e);  __test_passed__ = 0; }

#define TESTRESULTS() \
  std::cout << "----------TEST SUMMARY----------" << std::endl; \
  std::cout << GRN << "  " << __tests_passed << "/" << __tests_total << " tests passed" << " (" << ((100.0*__tests_passed)/(__tests_passed+__tests_failed+__tests_warned+__tests_errors))<< "%)" << BLN; \
  if (__tests_warned) { \
    std::cout << " with " << YLW << __tests_warned << " warning" << (__tests_warned==1 ? "" : "s") << BLN; \
    if (__tests_failed) { \
      std::cout << " and " << RED << __tests_failed << " failure" << (__tests_failed==1 ? "" : "s") << BLN; \
    } \
    if (__tests_errors) { \
      std::cout << " and " << CRT << __tests_errors << " error" << (__tests_errors==1 ? "" : "s") << BLN; \
    } \
  } else if (__tests_failed) { \
    std::cout << " with " << RED << __tests_failed << " failure" << (__tests_failed==1 ? "" : "s") << BLN; \
    if (__tests_errors) { \
      std::cout << " and " << CRT << __tests_errors << " error" << (__tests_errors==1 ? "" : "s") << BLN; \
    } \
  } else if (__tests_errors) { \
    std::cout << " with " << CRT << __tests_errors << " error" << (__tests_errors==1 ? "" : "s") << BLN; \
  } \
  std::cout << std::endl; \
  for(test_suite* s = suites; s; ) { \
    printf("    %s%40s%s: %3u test%s passed (%3.0f%%)",s->e ? CRT : s->f ? RED : s->w ? YLW : GRN,s->name,BLN,s->p,s->p==1 ? " " : "s",(100.0f*s->p/s->t)); \
    if (s->w) { \
      printf(" with %s%u warning%s%s",YLW,s->w,s->w==1 ? "" : "s",BLN);\
      if (s->f) { \
        printf(" and %s%u failure%s%s",RED,s->f,s->f==1 ? "" : "s",BLN);\
      }\
      if (s->e) { \
        printf(" and %s%u error%s%s",CRT,s->e,s->e==1 ? "" : "s",BLN);\
      }\
    } else if (s->f) { \
      printf(" with %s%u failure%s%s",RED,s->f,s->f==1 ? "" : "s",BLN);\
      if (s->e) { \
        printf(" and %s%u error%s%s",CRT,s->e,s->e==1 ? "" : "s",BLN);\
      }\
    } else if (s->e) { \
      printf(" with %s%u error%s%s",CRT,s->e,s->e==1 ? "" : "s",BLN);\
    }\
    printf("\n"); \
    struct _test_suite* next = s->next; \
    free(s); \
    s = next; \
  } \
  std::cout << "--------END TEST SUMMARY--------" << std::endl; \

}

#endif /* TESTS_H_ */

