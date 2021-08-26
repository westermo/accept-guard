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

ip addr add 10.0.0.1/24 dev a1
ip addr add 20.0.0.1/24 dev a2

ip -br l
ip -br a

export LD_PRELOAD=../accept-guard.so
export ACCEPT_GUARD_ACL="lo:8080;a2:8080"
./server -u -p 8080 &
PID=$!
unset LD_PRELOAD
sleep 1

if ! ./client -u -p 8080 127.0.0.1; then
    echo "Cannot connect via loopback"
    exit 1
fi

if ./client -u -p 8080 10.0.0.1; then
    echo "Should not be able to connect via a1"
    exit 1
fi

if ! ./client -u -p 8080 20.0.0.1; then
    echo "Cannot connect via a1"
    exit 1
fi

echo "Test successful."
exit 0