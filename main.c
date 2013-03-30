#include <stdio.h>
#include "pcap_collect.h"
#include "internal_ipc.h"

#include <unistd.h> // fork()
#include <stdlib.h> // exit()
#include <time.h>   // time()

//----------------------------------------------------------------------------

static
void loop_read_dump_json(int read_fd, char **stream_names)
{
  int id;
  int collection_time;
  int bytes;
  int packets;

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

int main(int argc, char **argv)
{
  // TODO: read stuff from command line
#define NSTREAMS 2
  char *stream_names[] = {
    "all jarowit.net traffic",
    "jarowit.net HTTP traffic"
  };
  char *ifaces[] = {
    "wlan0",
    "wlan0"
  };
  char *filters[] = {
    "host jarowit.net and not port 22",
    "host jarowit.net and (port 80 or port 443)"
  };

  int read_fd, write_fd;

  make_pipe(&read_fd, &write_fd); // TODO: error handling

  for (int i = 0; i < NSTREAMS; ++i) {
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
