#!/usr/bin/env sh
# No silent failure
set -euo pipefail

[ -e /opt/sysroot/lib ]   || ln -s usr/lib /opt/sysroot/lib
[ -e /opt/sysroot/lib64 ] || ln -s usr/lib /opt/sysroot/lib64

cd /work/src
pwd
ls -alh /opt
ls -alh /opt/sysroot
cmake -Bbuild -DCMAKE_TOOLCHAIN_FILE=./toolchain/aarch64-linux-gnu.cmake -DCMAKE_SYSROOT=/opt/sysroot/ -Dcreate3_build_local=OFF .
cd build
make -j22 | tee build.log
make install
cpack
