#!/usr/bin/python

import sys
#import optparse
#import Queue
import time

#-----------------------------------------------------------------------------
# non-blocking I/O {{{

def set_nonblocking(filehandle):
  from os import O_NONBLOCK
  from fcntl import fcntl, F_SETFL, F_GETFL
  fd = filehandle.fileno()
  fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)

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
# {{{

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

  def __del__(self):
    import curses
    curses.endwin()

  def prn(self, y, x, string, attrs = 0):
    self.screen.addstr(y, x, string, attrs)

  def refresh(self):
    self.screen.refresh()

# }}}
#-----------------------------------------------------------------------------

set_nonblocking(sys.stdin)

screen = Curses()

def print_stream(stream):
  i = stream['stream_id']
  name = stream['stream_name']
  flow = stream['bytes'] / 1024.0
  # print stream name
  screen.prn(i * 2 + 1, 0, "[%d] %s" % (i, name), curses.A_BOLD)
  # print stream flow
  screen.prn(i * 2 + 2, 0, "%9.2f kB/s [%3ds]" % (flow, 1))

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
