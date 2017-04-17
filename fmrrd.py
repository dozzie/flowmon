#!/usr/bin/python

import sys
import rrdtool
import json

#-----------------------------------------------------------------------------

if len(sys.argv) < 2:
    print "Usage: %s database.rrd" % (sys.argv[0].split('/')[-1])
    print ""
    print "RRD database should have two data sources, one for B/s and" \
          " one for packets/s."
    sys.exit(0)

rrd_file = sys.argv[1]

try:
    while True:
        line = sys.stdin.readline()
        if line == '':
            break
        stream = json.loads(line)

        if stream['stream_id'] == 0:
            data = '%(time)d:%(bytes)d:%(packets)d' % (stream)
            print "updating stream %d: %s" % (stream['stream_id'], data)
            rrdtool.update(rrd_file, data)
        else:
            print "ignored stream %d" % (stream['stream_id'])

except KeyboardInterrupt:
    pass

#-----------------------------------------------------------------------------
# vim:ft=python:foldmethod=marker
