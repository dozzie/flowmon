#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

#include "config.h"
#include "err_msg.h"

#include "command_line_args.h"

//----------------------------------------------------------------------------

#define WHITESPACES " \t\n"

//----------------------------------------------------------------------------

// function splits lines to interface name, filter and description
// function returns 0 on failure and 1 on success
// *iface, *filter and *descr are allocated with malloc(), so caller needs to
// free them on his own
static
int split_line(char *line, char **iface, char **filter, char **descr)
{
  // skip leading whitespaces
  line += strspn(line, WHITESPACES);

  // get iface name
  size_t end = strcspn(line, WHITESPACES);
  if (line[end] == 0)
    return 0;

  line[end] = 0;
  *iface = strdup(line);

  line += end + 1;

  // skip whitespaces between iface name and BPF filter
  line += strspn(line, WHITESPACES);
  if (*line != '\'' && *line != '"') {
    free(*iface);
    return 0;
  }

  ++line;
  char *l = line;
  while (*l != 0 && *l != '\'' && *line != '"') {
    if (*l == '\\')
      ++l;
    ++l;
  }
  *l++ = 0;

  *filter = strdup(line);

  line = l;

  // skip whitespaces between BPF filter and description
  line += strspn(line, WHITESPACES);
  if (*line == 0) {
    *descr = strdup(*filter);
  } else {
    if (*line != '\'' && *line != '"') {
      free(*iface);
      free(*filter);
      return 0;
    }

    ++line;
    l = line + 1;
    while (*l != 0 && *l != '\'' && *line != '"') {
      if (*l == '\\')
        ++l;
      ++l;
    }
    *l++ = 0;

    *descr = strdup(line);
  }

  return 1;
}

//----------------------------------------------------------------------------

size_t count_flow_number(int argc, char **argv,
                         char **bpfilters, char **descrs, char **ifaces)
{
  char *files[MAXFILTERS];
  size_t bcnt = 0, dcnt = 0, icnt = 0, fcnt = 0;

  // descriptions and other stuff coming from command line (argv)
  for (size_t i = 1; i < argc && bcnt < MAXFILTERS; ++i) {
    if (argv[i][0] == '-' && argv[i][2] == 0) {
      switch (argv[i][1]) {
        case 'f': bpfilters[bcnt++] = strdup(argv[++i]); break;
        case 'd': descrs[dcnt++]    = strdup(argv[++i]); break;
        case 'i': ifaces[icnt++]    = strdup(argv[++i]); break;
        default: /* ignore */; break;
      }
    } else {
      files[fcnt++] = argv[i];
    }
  }

  // missing descriptions are filled with BPF
  for (/* nothing */; dcnt < bcnt; ++dcnt)
    descrs[dcnt] = strdup(bpfilters[dcnt]);

  // missing interfaces are filled with "any" equivalent
  for (/* nothing */; icnt < bcnt; ++icnt)
    ifaces[icnt] = NULL;

  dcnt = bcnt;
  icnt = bcnt;

  // read filters from file
  for (size_t i = 0; i < fcnt && bcnt < MAXFILTERS; ++i) {
    FILE *file = fopen(files[i], "r");
    if (file == NULL) {
      errprintf("open(\"%s\") failed: ", files[i]);
      perror(NULL);
      continue;
    }

    char buffer[1024];
    size_t lineno = 0;
    while (bcnt < MAXFILTERS && fgets(buffer, sizeof(buffer), file)) {
      ++lineno;

      // skip empty lines and comments
      size_t f = strspn(buffer, " \t\n");
      if (buffer[f] == 0 || buffer[f] == '#')
        continue;

      if (split_line(buffer, ifaces + bcnt, bpfilters + bcnt, descrs + bcnt))
        ++bcnt;
      else {
        errprintf("%s, line %d: syntax error\n", files[i], lineno);
        sleep(1);
      }
    }

    fclose(file);
  }

  return bcnt;
}

//----------------------------------------------------------------------------
