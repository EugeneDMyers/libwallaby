#!/bin/sh

docker build -t libwallaby-builder:bullseye .

[ -d ./container/sysroot ] || tar -C ./container -xzf ./container/sysroot.tar.gz

SRC="$(pwd)"

docker run \
  --mount type=bind,src="${SRC}",dst=/work/src \
  --mount type=bind,src=./container/sysroot,dst=/opt/sysroot \
  libwallaby-builder:bullseye \
  bash "/work/src/container/build.sh"
