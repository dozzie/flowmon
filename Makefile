#!/usr/bin/make -f

.PHONY: all run

all flowmon:
	gcc -Wall -lpcap collect.c -o flowmon

run: flowmon
	sudo ./flowmon

# vim:ft=make
