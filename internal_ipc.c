#define _GNU_SOURCE 1
#include <stdio.h>    // dprintf()

//----------------------------------------------------------------------------

void send_stream_data(int fd, int id, int time, int bytes, int packets)
{
  // TODO: change to send()
  dprintf(fd, "{\"stream_id\":%d,\"time\":%d,\"bytes\":%d,\"packets\":%d}\n",
              id, time, bytes, packets);
}

//----------------------------------------------------------------------------
