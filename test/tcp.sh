#!/bin/sh
#set -x

# shellcheck source=/dev/null
. "$(dirname "$0")/lib.sh"

topology
server -t

print "Verifying loopback connectivity ..."
./client -t -p 8080 127.0.0.1 || FAIL

print "Verifying no connection via a1 ..."
./client -t -p 8080 10.0.0.1 && FAIL

print "Verifying connection via a2 ..."
./client -t -p 8080 20.0.0.1 || FAIL

OK
