#!/bin/sh
# Verifies basic UDP recvfrom() and recvmsg() API wrappers
#set -x

# shellcheck source=/dev/null
. "$(dirname "$0")/lib.sh"

topology
server -u

print "Verifying loopback connectivity ..."
./client -u -p 8080 127.0.0.1 || FAIL

print "Verifying no connection via a1 ..."
./client -u -p 8080 10.0.0.1 && FAIL

print "Verifying connection via a2 ..."
./client -u -p 8080 20.0.0.1 || FAIL

OK
