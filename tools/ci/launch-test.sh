#!/bin/bash
#
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# This script builds the project's boot.iso file in the docker environment and
# next launches a test and checks for a successful timeout.
set -e

COMPILER=$1

# Execute relative to this script
SCRIPTDIR="$( cd "$(dirname "$0")" ; pwd -P )"
declare -r SCRIPTDIR

# Go to root directory of repository
cd "$SCRIPTDIR"/../..

# Build project in docker
echo "Building project from scratch"
make clean V=1
make UNITTEST=1 docker:boot.iso V=1 CC=$COMPILER

# Use QEMU to launch the guest
echo "Launching KTF"
declare -i STATUS=0
timeout 10 make UNITTEST=1 docker:boot V=1 CC=$COMPILER || STATUS=$?

# Check if the expected exit code happened (124 for timeout)
if [ "$STATUS" -ne 0 ] && [ "$STATUS" -ne 124 ]
then
    echo "error: received unexpected exit code $STATUS"
    exit $STATUS
elif [ "$STATUS" -eq 124 ]
then
    echo "info: terminated with expected timeout signal"
fi

exit 0
