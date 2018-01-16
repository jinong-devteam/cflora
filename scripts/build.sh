#!/bin/bash

#It is necessary to execute following two commands at root of the source tree.
#git submodule init
#git submodule update

# iniparser compile
cd ../lib/iniparser
make
cd ..

for name in "libtp3" "libgnode" "libcflora" 
do
    cd ${name}
    mkdir release
    cd release
    cmake ..
    make
    cd ../..
done
cd ..

for name in "gcg" "gos"
do
    cd ${name}
    mkdir release
    cd release
    cmake ..
    make
    cd ../..
done

