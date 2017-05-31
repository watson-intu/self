#!/bin/bash
#Usage: stage_linux.sh [profile]

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
$DIR/stage.sh linux $1

