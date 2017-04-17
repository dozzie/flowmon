#!/usr/bin/make -f

CFLAGS = -std=gnu99 -g -O2
LDFLAGS = -pthread -lpcap

CC = gcc
LD = gcc

#-----------------------------------------------------------------------------

.PHONY: all
all: flowmon

.PHONY: clean
clean:
	rm -f flowmon *.o
	rm -rf doc/api

flowmon: main.o pcap_collect.o internal_ipc.o command_line_args.o err_msg.o
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) -c $(CFLAGS) $(filter %.c,$^)

#-----------------------------------------------------------------------------
# documentation

.PHONY: doxygen
doxygen:
	doxygen doxygen.conf

.PHONY: man
man: man/flowmon.8

man/%.8: man/%.pod
	pod2man --section=8 --center="Linux System Administration" --release="" $< $@

man/%.1: man/%.pod
	pod2man --section=1 --center="User Commands" --release="" $< $@

#-----------------------------------------------------------------------------
# *.o dependencies

main.o: pcap_collect.h internal_ipc.h command_line_args.h config.h

pcap_collect.o: err_msg.h internal_ipc.h pcap_collect.h

internal_ipc.o: internal_ipc.h

command_line_args.o: config.h err_msg.h command_line_args.h

err_msg.o: err_msg.h

#-----------------------------------------------------------------------------
# vim:ft=make
