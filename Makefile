#!/usr/bin/make -f

.PHONY: all run

all: flowmon

flowmon: $(wildcard *.c *.h)
	gcc -std=gnu99 -Wall -lpcap $(filter %.c,$^) -o flowmon

run: flowmon
	sudo ./flowmon

# vim:ft=make
