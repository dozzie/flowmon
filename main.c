/**
 * @file
 * Function \c main() and displayer (workers' parent) process loop.
 *
 * This compilation unit contains code for spawning workers (they capture
 * network traffic) and for receiving from them data about their streams.
 * Then, the data is sent as JSON to \c stdout.
 */

#include <stdio.h>
#include "pcap_collect.h"
#include "internal_ipc.h"
#include "command_line_args.h"
#include "config.h"

#include <unistd.h> // fork()
#include <stdlib.h> // exit()
#include <time.h>   // time()
#include <string.h> // strrchr()

//----------------------------------------------------------------------------

/**
 * Receive data from children and pass it to \c stdout as JSON.
 *
 * @param  read_fd  socket for receiving messages from children (\ref
 *   make_pipe())
 * @param  stream_names  array of names of streams captured by children,
 *   indexed by child IDs
 */
static
void loop_read_dump_json(int read_fd, char **stream_names)
{
  int id;
  int collection_time;
  int bytes;
  int packets;

  // disable buffering
  setbuf(stdout, NULL);

  while (1) {
    read_stream_data(read_fd, &id, &collection_time,
                     &bytes, &packets);
    printf("{\"stream_id\":%d,\"stream_name\":\"%s\","
           "\"time\":%d,\"now\":%d,"
           "\"bytes\":%d,\"packets\":%d}\n",
           id, stream_names[id],
           collection_time, (int)time(NULL),
           bytes, packets);
  }
}

//----------------------------------------------------------------------------

/**
 * Application entry point.
 *
 * This function calls command line parsing, creates children (workers that
 * capture streams) and calls displayer loop (\ref loop_read_dump_json()).
 *
 * @param  argc  number of command line arguments
 * @param  argv  command line arguments themselves
 *
 * @return  exit code
 */
int main(int argc, char **argv)
{
  char *filters[MAXFILTERS];
  char *stream_names[MAXFILTERS]; // was: descrs
  char *ifaces[MAXFILTERS];

  size_t streams_number = count_flow_number(argc, argv,
                                            filters, stream_names, ifaces);

  //--------------------------------------------------------------------------
  // help message

  if (streams_number == 0) {
    char *prog = strrchr(argv[0], '/');
    prog = (prog == NULL) ? argv[0] : prog + 1;

    printf("Usage: %s -f bpf_filter [-i iface] [-d description] "
           "... filter_file\n", prog);
    puts("\n"
         "filter file format:\n"
         "iface-name 'bpf filter within quotes' 'description, also within "
         "quotes'\n"
         "iface2 'another filter' 'another description'\n"
         "[...]");

    return 0;
  }

  //--------------------------------------------------------------------------
  // actual work

  int read_fd, write_fd;
  make_pipe(&read_fd, &write_fd); // TODO: error handling

  for (int i = 0; i < streams_number; ++i) {
    // TODO: remember children and set some signal handler (SIGHUP, SIGINT,
    // SIGTERM)
    if (fork() == 0) {
      // I would love to use this here, but I'm not sure if it doesn't
      // overwrite something on stack, like environment or arguments
      //sprintf(argv[0], "flowmon [%s]", stream_names[i]);
      close(read_fd);
      start_pcap_process(i, write_fd, ifaces[i], filters[i]);
      exit(1);
    }
  }

  loop_read_dump_json(read_fd, stream_names);

  return 0;
}

//----------------------------------------------------------------------------
