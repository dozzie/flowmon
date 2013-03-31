#ifndef __COMMAND_LINE_ARGS_H
#define __COMMAND_LINE_ARGS_H

#include <stdlib.h> // size_t

// this function stores in bpfilters, descrs and ifaces filters, descriptions
// and names of interfaces read from command line; all the strings from these
// arrays should be freed with free()
// function stores no more than MAXFILTERS filters
// function returns number of filters written
size_t count_flow_number(int argc, char **argv,
                         char **bpfilters, char **descrs, char **ifaces);

#endif // #ifndef __COMMAND_LINE_ARGS_H
