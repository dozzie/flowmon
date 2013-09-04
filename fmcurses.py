#!/usr/bin/python

import sys
import os
#import optparse
#import Queue
import time

#-----------------------------------------------------------------------------
# non-blocking I/O {{{

def set_nonblocking(filehandle):
  from fcntl import fcntl, F_SETFL, F_GETFL
  fd = filehandle.fileno()
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | os.O_NONBLOCK)

def try_read_json(filehandle):
  import errno
  import json
  result = None

  try:
    line = filehandle.readline()
    if line != '':
      result = json.loads(line)
  except IOError, e:
    if e.errno == errno.EAGAIN:
      pass
    else:
      raise e

  return result

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

  def update(self, value, time = None):
    if time == None:
      from time import time as timestamp
      time = int(timestamp())

    if self.latest != None:
      if time <= self.latest:
        # TODO: raise an error
        return

      skipped_entries = (time - self.latest + self.step - 1) / self.step - 1

      if skipped_entries >= len(self.queue):
        # kill all the entries and reset position
        for x in range(len(self.queue)):
          self.queue[x] = None
        self.queue_position = 0
      else:
        # kill entries that were skipped and update pointer
        for x in range(skipped_entries):
          self.queue[(x + self.queue_position) % len(self.queue)] = None
        self.queue_position += skipped_entries
        self.queue_position %= len(self.queue)

    self.queue[self.queue_position] = value
    self.queue_position += 1
    self.queue_position %= len(self.queue)
    self.latest = time

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

import curses

class Curses:
  def __init__(self):
    self.screen = curses.initscr()
    self.curses = curses
    curses.cbreak()
    curses.noecho()
    curses.nonl()
    #self.screen.nodelay(1)
    self.screen.timeout(100)

    self.tty = open('/dev/tty', 'r')
    set_nonblocking(self.tty)
    # if there wasn't (n)curses in use, I would set terminal into raw noecho
    # mode (equivalent to following code, but without calling os.system())
    #os.system('stty raw -echo <&%d' % self.tty.fileno())

  def __del__(self):
    import curses
    curses.endwin()

  def prn(self, y, x, string, attrs = 0):
    self.screen.addstr(y, x, string, attrs)

  def refresh(self):
    self.screen.refresh()
    char = None
    try:
      if self.tty.read(1) == 'q':
        raise KeyboardInterrupt()
    except IOError, e:
      if e.errno == os.errno.EAGAIN:
        pass # ignore
      else:
        raise e

# }}}
#-----------------------------------------------------------------------------

set_nonblocking(sys.stdin)

screen = Curses()
mean = {}
periods = [5, 30, 180]
max_period = max(periods)

def print_stream(stream):
  i = stream['stream_id']
  name = stream['stream_name']
  if i not in mean:
    mean[i] = WindowedMean(window = max_period)

  mean[i].update(stream['bytes'], stream['time'])

  # print stream name
  screen.prn(i * 2 + 1, 0, "[%d] %s" % (i, name), curses.A_BOLD)
  # print stream flow
  for p in range(len(periods)):
    flow = mean[i].mean(periods[p])
    screen.prn(i * 2 + 2, 22 * p, "%9.2f kB/s [%3ds]" % (flow, periods[p]))

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
  pass

#-----------------------------------------------------------------------------
# vim:ft=python:foldmethod=marker
