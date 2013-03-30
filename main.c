#include <stdio.h>
#include "pcap_collect.h"
#include "internal_ipc.h"

#include <unistd.h> // fork()
#include <stdlib.h> // exit()
#include <time.h>   // time()

//----------------------------------------------------------------------------

int main(void)
{
  int read_fd, write_fd;
  make_pipe(&read_fd, &write_fd); // TODO: error

  if (fork()) {
    // parent

    close(write_fd);

    int id;
    int collection_time;
    int bytes;
    int packets;

    while (1) {
      read_stream_data(read_fd, &id, &collection_time,
                       &bytes, &packets);
      printf("{\"stream_id\":%d,\"time\":%d,\"now\":%d,"
             "\"bytes\":%d,\"packets\":%d}\n",
             id, collection_time, (int)time(NULL), bytes, packets);
    }

  } else {
    // child

    close(read_fd);
    start_bpf_process(1638, write_fd, "wlan0",
                      "host jarowit.net and not port 22");
    exit(1);
  }

  return 0;
}

//----------------------------------------------------------------------------
