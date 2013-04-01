/**
 * @file
 * Communication between processes spawned internally.
 */

#include <sys/types.h>
#include <sys/socket.h>

#include "internal_ipc.h"

//----------------------------------------------------------------------------

/**
 * Buffer containing information about stream that is passed from child worker
 * to parent displayer.
 */
struct stream_data_buffer {
  /// ID of stream (chosen by parent).
  int id;
  /// UNIX timestamp of when the data was collected.
  int time;
  /// Number of bytes collected up to \ref time.
  int bytes;
  /// Number of packets collected up to \ref time.
  int packets;
};

/**
 * Send data about the stream to parent process.
 *
 * @param  fd  file descriptor to send data through
 * @param  id  stream identifier (chosen by parent)
 * @param  time  UNIX timestamp (\c time()) of when data was collected
 * @param  bytes  number of bytes collected
 * @param  packets  number of packets collected
 *
 * @see  \ref read_stream_data()
 * @see  \ref make_pipe()
 */
void send_stream_data(int fd, int id, int time, int bytes, int packets)
{
  struct stream_data_buffer buffer = { id, time, bytes, packets };
  send(fd, &buffer, sizeof(buffer), MSG_DONTWAIT);
}

/**
 * Receive data about a stream from one of children.
 *
 * @param  fd  file descriptor to read data from
 * @param[out]  id  stream identifier
 * @param[out]  time  UNIX timestamp (\c time()) of when data was collected
 * @param[out]  bytes  number of bytes collected
 * @param[out]  packets  number of packets collected
 *
 * @see  \ref send_stream_data()
 * @see  \ref make_pipe()
 */
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

/**
 * Prepare pair of sockets for use with \ref read_stream_data() and \ref
 * send_stream_data().
 * Prepared sockets will be \c AF_UNIX datagram sockets.
 *
 * @param[out]  read_fd  socket for reading in parent process
 * @param[out]  write_fd  socket for writing in child processes
 *
 * @note  \c read_fd and \c write_fd are undistinguishable, so one may use
 *   them for the opposite purpose. It's merely a convention which one to use
 *   how.
 *
 * @return  0 on success, -1 on failure (check \c errno)
 */
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
