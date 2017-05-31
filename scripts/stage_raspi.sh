#!/bin/bash
#Usage: stage_raspi.sh [profile]

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
$DIR/stage.sh raspi $1

