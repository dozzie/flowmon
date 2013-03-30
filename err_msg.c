#include <stdio.h>
#include <stdarg.h>   // va_*
#include <unistd.h>   // write()

#include "err_msg.h"

//----------------------------------------------------------------------------

int errprintf(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  char buf[1024];
  int len = vsnprintf(buf, 1024, fmt, args);
  if (len >= 0)
    write(2 /* stderr */, buf, len);
  va_end(args);
  return len;
}

//----------------------------------------------------------------------------
