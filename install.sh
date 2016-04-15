#!/bin/bash

cd precompilation/simulation_basis/libs/jeayeson
chmod +x configure
./configure

cd ../../..
mkdir build
cd build
cmake ..
make
