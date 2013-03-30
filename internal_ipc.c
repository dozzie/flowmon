#include <sys/types.h>
#include <sys/socket.h>

#include "internal_ipc.h"

//----------------------------------------------------------------------------

struct stream_data_buffer {
  int id;
  int time;
  int bytes;
  int packets;
};

void send_stream_data(int fd, int id, int time, int bytes, int packets)
{
  struct stream_data_buffer buffer = { id, time, bytes, packets };
  send(fd, &buffer, sizeof(buffer), MSG_DONTWAIT);
}

void read_stream_data(int fd, int *id, int *time, int *bytes, int *packets)
{
  struct stream_data_buffer buffer;
  recv(fd, &buffer, sizeof(buffer), 0);
  *id      = buffer.id;
  *time    = buffer.time;
  *bytes   = buffer.bytes;
  *packets = buffer.packets;
  // TODO: signal an error, if any
}

int make_pipe(int *read_fd, int *write_fd)
{
  int pair[2];

  if (socketpair(AF_UNIX, SOCK_DGRAM, 0, pair) < 0)
    return -1;

  *read_fd  = pair[0];
  *write_fd = pair[1];

  return 0;
}

//----------------------------------------------------------------------------
