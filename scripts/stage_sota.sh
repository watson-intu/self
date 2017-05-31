#!/bin/bash
#Usage: stage_self.sh [profile]

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
$DIR/stage.sh sota $1

