#!/bin/bash

STANDARD_DIR=

if [ "$STANDARD_DIR" = "" ]; then
	echo "Error: you need to specify the path to the standard library includes of gcc in the script"
	exit 1
fi

FOLDER=$(dirname `realpath $0`)
$FOLDER/bin/precompilation $@ -- -std=c++14 "-I$STANDARD_DIR"
