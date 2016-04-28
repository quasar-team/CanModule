#ifndef __GETTIMEOFDAY_H_
#define __GETTIMEOFDAY_H_

#ifdef _WIN32
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

#include <time.h>

struct timezone 
{
  int  tz_minuteswest; // minutes W of Greenwich 
  int  tz_dsttime;     // type of dst correction 
};

int gettimeofday(struct timeval *tv, struct timezone *tz = NULL);

#endif

#endif /*__GETTIMEOFDAY_H_*/
