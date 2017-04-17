#!/usr/bin/make -f

CFLAGS = -std=gnu99 -g -O2
LDFLAGS = -pthread -lpcap

CC = gcc
LD = gcc

#-----------------------------------------------------------------------------

.PHONY: all doc

all: flowmon

doc:
	doxygen doxygen.conf

flowmon: main.o pcap_collect.o internal_ipc.o command_line_args.o err_msg.o
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) -c $(CFLAGS) $(filter %.c,$^)

#-----------------------------------------------------------------------------
# *.o dependencies

main.o: pcap_collect.h internal_ipc.h command_line_args.h config.h

pcap_collect.o: err_msg.h internal_ipc.h pcap_collect.h

internal_ipc.o: internal_ipc.h

command_line_args.o: config.h err_msg.h command_line_args.h

err_msg.o: err_msg.h

#-----------------------------------------------------------------------------
# vim:ft=make
