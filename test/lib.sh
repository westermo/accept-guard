#!/bin/sh

# Test name, used everywhere as /tmp/$NM/foo
if [ "$0" = "-bash" ]; then
    NM="bash"
else
    NM=$(basename "$0" .sh)
fi

print()
{
    printf "\e[7m>> %-80s\e[0m\n" "$1"
}

SKIP()
{
    print "TEST: SKIP"
    [ $# -gt 0 ] && echo "$*"
    exit 77
}

FAIL()
{
    print "TEST: FAIL"
    [ $# -gt 0 ] && echo "$*"
    exit 99
}

OK()
{
    print "TEST: OK"
    [ $# -gt 0 ] && echo "$*"
    exit 0
}

topology()
{
    print "Creating world ..."
    ip link add a1 type dummy
    ip link add a2 type dummy

    ip link set lo up
    ip link set a1 up
    ip link set a2 up

    ip addr add 10.0.0.1/24 dev a1
    ip addr add 20.0.0.1/24 dev a2

    ip addr add 2001::1:1/64 dev a1
    ip addr add 2001::2:1/64 dev a2

    ip -br l
    ip -br a
}

server()
{
    print "Starting server ..."
    export LD_PRELOAD=../accept-guard.so
    export ACCEPT_GUARD_ACL="lo:8080;a2:8080"
    ./server "$@" -p 8080 &
    PID=$!
    unset LD_PRELOAD
    unset ACCEPT_GUARD_ACL
    sleep 1
}

cleanup()
{
    [ -n "$PID" ] && kill -9 "$PID"
}

trap cleanup INT TERM QUIT EXIT
