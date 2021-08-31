#!/bin/sh

for test in "$@"; do
    nm=$(basename "$test" .sh)
    unshare -mrun "./$test" >"$nm".log 2>&1
    case "$?" in
	"0")
	    echo "PASS: $test"
	    ;;
	"77")
	    echo "SKIP: $test"
	    ;;
	"99")
	    echo "FAIL: $test"
	    cat "$nm".log
	    ;;
    esac
done
