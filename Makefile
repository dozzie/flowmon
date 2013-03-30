#!/usr/bin/make -f

.PHONY: all run

all: flowmon

flowmon: $(wildcard *.c *.h)
	gcc -Wall -lpcap $(filter %.c,$^) -o flowmon

run: flowmon
	sudo ./flowmon

# vim:ft=make
