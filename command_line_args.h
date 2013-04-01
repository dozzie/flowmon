/**
 * @file
 * Command line and config file parsing (unit API).
 */

#ifndef __COMMAND_LINE_ARGS_H
#define __COMMAND_LINE_ARGS_H

#include <stdlib.h> // size_t

size_t count_flow_number(int argc, char **argv,
                         char **bpfilters, char **descrs, char **ifaces);

#endif // #ifndef __COMMAND_LINE_ARGS_H
