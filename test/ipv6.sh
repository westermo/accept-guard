#!/bin/sh

cleanup()
{
    [ -n "$PID" ] && kill -9 "$PID"
}

trap cleanup INT TERM QUIT EXIT

ip link add a1 type dummy
ip link add a2 type dummy

ip link set lo up
ip link set a1 up
ip link set a2 up

ip addr add 2001::1:1/64 dev a1
ip addr add 2001::2:1/6 dev a2

ip -br l
ip -br a

export LD_PRELOAD=../accept-guard.so
export ACCEPT_GUARD_ACL="lo:8080;a2:8080"
./server -6 -t -p 8080 &
PID=$!
unset LD_PRELOAD
sleep 1

if ! ./client -6 -t -p 8080 ::1; then
    echo "Cannot connect via loopback"
    exit 1
fi

if ./client -6 -t -p 8080 2001::1:1; then
    echo "Should not be able to connect via a1"
    exit 1
fi

if ! ./client -6 -t -p 8080 2001::2:1; then
    echo "Cannot connect via a1"
    exit 1
fi

echo "Test successful."
exit 0
