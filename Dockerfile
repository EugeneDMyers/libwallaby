FROM debian:bullseye

RUN apt update && apt install -y \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
    cmake swig doxygen

RUN mkdir -p /work

WORKDIR /work
