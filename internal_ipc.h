/**
 * @file
 * Communication between processes spawned internally (unit API).
 */

#ifndef __INTERNAL_IPC_H
#define __INTERNAL_IPC_H

//----------------------------------------------------------------------------

void send_stream_data(int fd, int id, int time, int bytes, int packets);
void read_stream_data(int fd, int *id, int *time, int *bytes, int *packets);
int make_pipe(int *read_fd, int *write_fd);

//----------------------------------------------------------------------------

#endif // #ifndef __INTERNAL_IPC_H
