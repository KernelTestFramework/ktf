#!/bin/sh
#
# Not all systems ship a 'python' wrapper, try version specific ones if we
# fail to find the version-agnostic one.
#
# Copyright (c) 2023 Open Source Security, Inc.

try_python() {
	if $1 -c 'exit' 2>/dev/null; then
		echo "$1"
		exit 0
	fi
}

try_python python
try_python python3
try_python python2

echo "error: python interpreter not found!" >&2
exit 1
