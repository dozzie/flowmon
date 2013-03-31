#!/usr/bin/python

import sys
import time
import rrdtool

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

if len(sys.argv) < 2:
  print "Usage: %s database.rrd" % (sys.argv[0].split('/')[-1])
  print ""
  print "RRD database should have two data sources, one for B/s and" + \
        " one for packets/s."
  sys.exit(0)

rrd_file = sys.argv[1]

set_nonblocking(sys.stdin)

try:
  while True:
    stream = try_read_json(sys.stdin)
    while stream != None:
      if stream['stream_id'] == 0:
        data = '%(time)d:%(bytes)d:%(packets)d' % (stream)
        print "updating stream %d: %s" % (stream['stream_id'], data)
        rrdtool.update(rrd_file, data)
      else:
        print "ignored stream %d" % (stream['stream_id'])
      stream = try_read_json(sys.stdin)

    # no stream data left -- wait for more
    time.sleep(0.01)
except KeyboardInterrupt:
  pass

#-----------------------------------------------------------------------------
# vim:ft=python:foldmethod=marker
