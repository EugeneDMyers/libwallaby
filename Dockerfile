FROM debian:bullseye

# Outline:
# 1. Install deps
# 2. Add and extract sysroot

RUN apt update && apt install -y \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
    cmake swig doxygen

RUN mkdir -p /work

WORKDIR /work
