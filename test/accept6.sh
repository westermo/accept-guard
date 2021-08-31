#!/bin/sh
#set -x

# shellcheck source=/dev/null
. "$(dirname "$0")/lib.sh"

topology
server -6 -t

print "Verifying loopback connectivity ..."
./client -6 -t -p 8080 ::1 || FAIL

print "Verifying no connection via a1 ..."
./client -6 -t -p 8080 2001::1:1 && FAIL

print "Verifying connection via a2 ..."
./client -6 -t -p 8080 2001::2:1 || FAIL

OK
