#!/usr/bin/make -f

.PHONY: all doc

all: flowmon

doc:
	doxygen doxygen.conf

flowmon: $(wildcard *.c *.h)
	gcc -std=gnu99 -Wall -pthread -lpcap $(filter %.c,$^) -o flowmon

# vim:ft=make
