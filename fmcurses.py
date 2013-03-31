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

def read_json_list(filehandle):
  result = []
  while True:
    data = try_read_json(filehandle)
    if data == None:
      break
    result.append(data)
  return result if len(result) > 0 else None

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
streams = read_json_list(sys.stdin)
while streams == None:
  time.sleep(0.1)
  streams = read_json_list(sys.stdin)

stream_names = {}
for x in streams:
  stream_names[ x['stream_id'] ] = x['stream_name']

screen = Curses()
for i in stream_names.keys():
  screen.prn(i * 2 + 1, 0, "[%d] %s" % (i, stream_names[i]), curses.A_BOLD)

def print_stream(stream):
  i = stream['stream_id']
  flow = stream['bytes'] / 1024.0
  screen.prn(i * 2 + 2, 0, "%9.2f kB/s [%3ds]" % (flow, 1))

for stream in streams:
  print_stream(stream)

screen.refresh()

while True:
  stream = try_read_json(sys.stdin)
  while stream != None:
    sys.stderr.write("stream: %s\n" % (stream))
    print_stream(stream)
    stream = try_read_json(sys.stdin)
  screen.refresh()
  time.sleep(0.01)

#-----------------------------------------------------------------------------
# vim:ft=python
