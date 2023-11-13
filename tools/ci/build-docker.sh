#!/bin/bash
#
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# This script builds the project's ktf.iso file in the docker environment
set -e

COMPILER=$1

# Execute relative to this script
SCRIPTDIR="$( cd "$(dirname "$0")" ; pwd -P )"
declare -r SCRIPTDIR

# Go to root directory of repository
cd "$SCRIPTDIR"/../..

# Build project in docker
make clean V=1
make docker:ktf.iso V=1 CC=$COMPILER
