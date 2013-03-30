#include <pcap.h>
#include <string.h>   // memset()
#include <time.h>     // nanosleep()
#include <pthread.h>

#include "err_msg.h"      // errprintf()
#include "internal_ipc.h" // send_stream_data()
#include "pcap_collect.h"

#define FOREVER (-1)

//----------------------------------------------------------------------------

struct stream_data {
  int id;
  int write_fd;
  int time;
  int bytes;
  int packets;
  pthread_mutex_t *mutex;
};

typedef void* (*pthread_main)(void *);

//----------------------------------------------------------------------------

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

static
void* periodic_send_stream_data(struct stream_data *stream)
{
  struct timespec sleep_interval = { 1, 0 }; // 1s
  while (1) {
    nanosleep(&sleep_interval, NULL);

    pthread_mutex_lock(stream->mutex);

    send_stream_data(stream->write_fd, stream->id,
                     time(NULL), stream->bytes, stream->packets);

    // reset stats
    stream->bytes   = 0;
    stream->packets = 0;

    pthread_mutex_unlock(stream->mutex);
  }

  return NULL;
}

//----------------------------------------------------------------------------

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
