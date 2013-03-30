#define _GNU_SOURCE 1
#include <stdio.h>    // fdprintf()
#include <stdarg.h>   // va_*
#include <unistd.h>   // write()

#include <pcap.h>
#include <string.h>   // memset()

#define FOREVER (-1)

//----------------------------------------------------------------------------

struct stream_data {
  int id;
  int write_fd;
  int time;
  int bytes;
  int packets;
};

//----------------------------------------------------------------------------
// auxiliary functions {{{

static
void send_stream_data(int fd, int id, int time, int bytes, int packets)
{
  dprintf(fd, "{\"stream_id\":%d,\"time\":%d,\"bytes\":%d,\"packets\":%d}\n",
              id, time, bytes, packets);
}

int errprintf(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  char buf[1024];
  int len = vsnprintf(buf, 1024, fmt, args);
  if (len >= 0)
    write(2 /* stderr */, buf, len);
  va_end(args);
  return len;
}

// }}}
//----------------------------------------------------------------------------

static
void count_packets(struct stream_data *stream,
                   const struct pcap_pkthdr *hdr,
                   const u_char *bytes)
{
  send_stream_data(stream->write_fd,
                   stream->id, hdr->ts.tv_sec,
                   hdr->len, 1);
}

//----------------------------------------------------------------------------

void start_bpf_process(int id, int write_fd, char *iface, char *filter)
{
  struct bpf_program compiled_filter;
  struct stream_data data;

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

  pcap_loop(socket, FOREVER, (pcap_handler)count_packets, (u_char *)&data);
}

//----------------------------------------------------------------------------

int main(void)
{
  start_bpf_process(1638, 1 /* stdout */, "wlan0",
                    "host jarowit.net and not port 22");
  return 0;
}

//----------------------------------------------------------------------------
// vim:foldmethod=marker
