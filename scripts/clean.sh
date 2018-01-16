#!/bin/bash

cd ../lib

for name in "libtp3" "libgnode" "libcflora"
do
    cd ${name}
    rm -rf release
    rm -rf debug
    cd ..
done

cd ..

for name in "gcg" "gos"
do
    cd ${name}
    rm -rf release
    rm -rf debug 
    cd ..
done

