/**
 * @file
 * Workers that capture network streams and pass to their parent summarized
 * data about the streams.
 */

#include <pcap.h>
#include <string.h>   // memset()
#include <time.h>     // nanosleep()
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>   // exit()

#include "err_msg.h"      // errprintf()
#include "internal_ipc.h" // send_stream_data()
#include "pcap_collect.h"

/// How long should \c pcap_loop() run.
#define FOREVER (-1)

//----------------------------------------------------------------------------

/**
 * Container for data about the stream.
 */
struct stream_data {
  /// Stream ID (chosen by parent).
  int id;
  /// Where to send data (with \ref send_stream_data()) periodically.
  int write_fd;
  /// <i>unused</i>
  int time;
  /// Bytes collected so far.
  int bytes;
  /// Packets collected so far.
  int packets;
  /// Mutex for modifying data in this container.
  pthread_mutex_t *mutex;
};

/// Type of thread's main function.
typedef void* (*pthread_main)(void *);

//----------------------------------------------------------------------------

/**
 * Callback function for libpcap, aggregating information about the stream.
 *
 * @param  stream  data about the stream
 * @param  hdr  information about the packet
 * @param  bytes  packet itself
 */
static
void count_packets(struct stream_data *stream,
                   const struct pcap_pkthdr *hdr,
                   const u_char *bytes)
{
  pthread_mutex_lock(stream->mutex);

  stream->bytes   += hdr->len;
  stream->packets += 1;

  pthread_mutex_unlock(stream->mutex);
}

//----------------------------------------------------------------------------

/**
 * Sender thread main loop.
 * This function every second sends data collected by \ref count_packets() to
 * parent process.
 *
 * @param  stream  data about the stream
 *
 * @return  \c NULL
 */
static
void* periodic_send_stream_data(struct stream_data *stream)
{
  struct timespec sleep_interval = { 1, 0 }; // 1s
  while (1) {
    nanosleep(&sleep_interval, NULL);

    pthread_mutex_lock(stream->mutex);

    int result = send_stream_data(stream->write_fd, stream->id,
                                  time(NULL), stream->bytes, stream->packets);

    if (result > -1) {
      // reset stats
      stream->bytes   = 0;
      stream->packets = 0;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // do nothing, not even reset the counters
    } else if (errno == ENOTCONN || errno == ECONNREFUSED) {
      // remote endpoint finished
      exit(0);
    } else {
      // error
      errprintf("stream=%d [errno=%d] %s\n",
                stream->id, errno, strerror(errno));
      exit(1);
    }

    pthread_mutex_unlock(stream->mutex);
  }

  return NULL;
}

//----------------------------------------------------------------------------

/**
 * Spawn a thread that will periodically send data to parent and setup
 * libpcap capture.
 *
 * @param  id  steram identifier
 * @param  write_fd  where to send data to parent process
 * @param  iface  interface on which libpcap should capture the stream
 * @param  filter  BPF filter specification to use with capture
 */
void start_pcap_process(int id, int write_fd, char *iface, char *filter)
{
  struct bpf_program compiled_filter;
  struct stream_data data;

  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_t thread;

  pcap_t *socket;
  char error[PCAP_ERRBUF_SIZE] = "";

  //--------------------------------------------------------------------------
  // prepare pcap_t socket {{{

  socket = pcap_open_live(iface, 1, 0 /* promisc */, 0 /* ms */, error);
  if (error[0] != 0) {
    errprintf("open_live(%s) error: %s\n", iface, error);
  }
  if (socket == NULL) {
    return;
  }

  // }}}
  //--------------------------------------------------------------------------
  // set filter on pcap_t socket {{{

  if (pcap_compile(socket, &compiled_filter, filter, 1, 0) == -1) {
    errprintf("BPF compile error (\"%s\"): %s\n",
              filter, pcap_geterr(socket));
    pcap_close(socket);
    return;
  }

  pcap_setfilter(socket, &compiled_filter);

  // }}}
  //--------------------------------------------------------------------------
  // collect packets

  memset(&data, 0, sizeof(data));
  data.id = id;
  data.write_fd = write_fd;
  data.mutex = &mutex;

  pthread_create(&thread, NULL,
                 (pthread_main)periodic_send_stream_data, (void *)&data);

  pcap_loop(socket, FOREVER, (pcap_handler)count_packets, (u_char *)&data);
}

//----------------------------------------------------------------------------
// vim:foldmethod=marker
