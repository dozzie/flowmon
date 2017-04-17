#!/usr/bin/python
#
# This script summarizes data collected by flowmon(8) and displays it on
# screen as averages over several time windows of different lengths (e.g.
# average over 5s, 30s, and 180s).
#

import sys
import os
import optparse
import time
import fcntl
import errno
import json
import curses

#-----------------------------------------------------------------------------
# command line options {{{

parser = optparse.OptionParser(
    usage = "%prog [options]"
)

parser.add_option(
    "-t", "--interval", dest = "intervals", action = "append", default = None,
    type = "int",
    help = "display average over interval (option may be used multiple times)",
    metavar = "SECONDS",
)

(options, args) = parser.parse_args()

if options.intervals is None:
    options.intervals = [5, 30, 180]

options.max_interval = max(options.intervals)

# }}}
#-----------------------------------------------------------------------------
# non-blocking I/O {{{

def set_nonblocking(filehandle):
    fd = filehandle.fileno()
    flags = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, flags | os.O_NONBLOCK)

def try_read_json(filehandle):
    try:
        line = filehandle.readline()
        if line != "":
            return json.loads(line)
        else:
            return None
    except IOError, e:
        if e.errno == errno.EWOULDBLOCK or e.errno == errno.EAGAIN:
            return None
        else:
            raise

# }}}
#-----------------------------------------------------------------------------
# mean over a window {{{

class WindowedMean:
    def __init__(self, step = 1, window = 5):
        self.window = window
        self.step = step

        queue_length = (window + step - 1) / step
        self.queue = [None] * queue_length
        self.queue_position = 0 # next value will be put here

        self.latest = None

    def update(self, value, timestamp = None):
        if timestamp == None:
            timestamp = int(time.time())

        if self.latest != None:
            if timestamp <= self.latest:
                # TODO: raise an error
                return

            skipped_entries = \
                (timestamp - self.latest + self.step - 1) / self.step - 1

            if skipped_entries >= len(self.queue):
                # kill all the entries and reset position
                for x in range(len(self.queue)):
                    self.queue[x] = None
                self.queue_position = 0
            else:
                # kill entries that were skipped and update pointer
                for x in range(skipped_entries):
                    pos = (x + self.queue_position) % len(self.queue)
                    self.queue[pos] = None
                self.queue_position += skipped_entries
                self.queue_position %= len(self.queue)

        self.queue[self.queue_position] = value
        self.queue_position += 1
        self.queue_position %= len(self.queue)
        self.latest = timestamp

    def __getitem__(self, index):
        if index < 0:
            # queue position less requested index
            #   * index is negative
            #   * index starts at 1, but queue position is 1 after last entry
            i = self.queue_position + index + len(self.queue)
            return self.queue[i % len(self.queue)]

        if index > self.latest:
            # TODO: raise an error
            return None

        # FIXME: this may be inaccurate
        idx = (self.latest - index) / self.step
        return self.queue[idx]

    def mean(self, window = None):
        if window == None:
            window = len(self.queue)

        acc = 0.0
        nitems = 0
        for x in range(-window, 0):
            if self[x] != None:
                acc += self[x]
                nitems += 1
        return acc / nitems

# }}}
#-----------------------------------------------------------------------------
# curses support {{{

class Curses:
    def __init__(self):
        self.screen = curses.initscr()
        curses.cbreak()
        curses.noecho()
        curses.nonl()
        #self.screen.nodelay(1)
        self.screen.timeout(100)

        self.tty = open("/dev/tty", "r")
        set_nonblocking(self.tty)
        # if there wasn't (n)curses in use, I would set terminal into raw
        # noecho mode (equivalent to following code, but without calling
        # os.system())
        #os.system("stty raw -echo <&%d" % (self.tty.fileno(),))

    def stop(self):
        curses.endwin()

    def prn(self, y, x, string, attrs = 0):
        self.screen.addstr(y, x, string, attrs)

    def refresh(self):
        self.screen.refresh()
        self.screen.move(0, 0)
        try:
            if self.tty.read(1) == "q":
                raise KeyboardInterrupt()
        except IOError, e:
            if e.errno == errno.EWOULDBLOCK or e.errno == errno.EAGAIN:
                pass # ignore
            else:
                raise

# }}}
#-----------------------------------------------------------------------------

set_nonblocking(sys.stdin)

screen = Curses()
mean = {}

def print_stream(stream):
    i = stream["stream_id"]
    name = stream["stream_name"]
    if i not in mean:
        mean[i] = WindowedMean(window = options.max_interval)

    mean[i].update(stream["bytes"], stream["time"])

    # print stream name
    screen.prn(i * 2 + 1, 0, "[%d] %s" % (i, name), curses.A_BOLD)
    # print stream flow
    for p in range(len(options.intervals)):
        # display units are kB/s, but collected are B/s
        flow = mean[i].mean(options.intervals[p]) / 1024.0
        screen.prn(i * 2 + 2, 22 * p,
                   "%9.2f kB/s [%3ds]" % (flow, options.intervals[p]))

try:
    while True:
        stream = try_read_json(sys.stdin)
        while stream != None:
            print_stream(stream)
            stream = try_read_json(sys.stdin)

        # no stream data left -- refresh and wait for more
        screen.refresh()
        time.sleep(0.01)
except KeyboardInterrupt:
    # either ^C or "q" from terminal
    pass

screen.stop()

#-----------------------------------------------------------------------------
# vim:ft=python:foldmethod=marker
