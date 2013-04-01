/**
 * @file
 * Error messages and logging.
 */

#include <stdio.h>
#include <stdarg.h>   // va_*
#include <unistd.h>   // write()

#include "err_msg.h"

//----------------------------------------------------------------------------

/**
 * Print formatted error to \c stderr.
 * Function is very similar to \c printf(), except that it prints to standard
 * error and it uses file descriptor instead of \c FILE*.
 *
 * @param  fmt  format string
 *
 * @return  number of bytes written.
 */
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
