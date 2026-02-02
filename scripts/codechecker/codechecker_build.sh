#!/bin/bash
cd tests/functional/model/
./download_deps.sh
rm -rf build/
mkdir -p build/
cd build/
cmake -DLT_CAL=mbedtls_v4 ..
make -j