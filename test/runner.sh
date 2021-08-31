#!/bin/sh

for test in "$@"; do
    nm=$(basename "$test" .sh)
    unshare -mrun "./$test" >"$nm".log 2>&1
    case "$?" in
	"0")
	    printf "\033[0;32mPASS\033[0m: $test\n"
	    ;;
	"77")
	    echo "\033[0;33mSKIP\033[0m: $test\n"
	    ;;
	"99")
	    echo "\033[0;31mFAIL\033[0m: $test\n"
	    cat "$nm".log
	    ;;
    esac
done
