#!/bin/bash

# run from 'build' dir (mkdir build && cd build && cmake .. && ../run.sh)

make && cp -r ../working-dir/* ~/.codesaru/csaru-game2d1-cpp/
if [ $? -eq 0 ]; then
	pushd ../working-dir
	csaru-game2d1-cpp
	popd
fi
