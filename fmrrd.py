#!/usr/bin/python
#
# This script is an example of how to put stream data from flowmon to RRD
# database.
#
# You can use following command to create an RRD database suitable for this
# script:
#
# rrdtool create database.rrd \
#   --step 1 \
#   DS:bytes:ABSOLUTE:2:0:U \
#   DS:packets:ABSOLUTE:2:0:U \
#   RRA:AVERAGE:0.25:1:301 \
#   RRA:AVERAGE:0.25:60:361 \
#   RRA:AVERAGE:0.25:3600:721
#
# Record count notes:
#   301 = 1 +  5 * 60 # 5 minutes worth data of granularity 1s
#   361 = 1 +  6 * 60 # 6 hours worth data of granularity 60s = 1min
#   721 = 1 + 30 * 24 # 30 days worth data of granularity 3600s = 1h
#
# For each RRA an additional record is for the record currently being filled.
#
# See rrdcreate(1) man page for details.
#

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
