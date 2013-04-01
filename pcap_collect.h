/**
 * @file
 * Workers that capture network streams and pass to their parent summarized
 * data about the streams (unit API).
 */

#ifndef __PCAP_COLLECT_H
#define __PCAP_COLLECT_H

//----------------------------------------------------------------------------

void start_pcap_process(int id, int write_fd, char *iface, char *filter);

//----------------------------------------------------------------------------

#endif // #ifndef __PCAP_COLLECT_H
