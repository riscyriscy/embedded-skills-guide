FROM ubuntu:24.04

# Every package is pinned to an exact version, so the image builds the same tools next year.
RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        cmake=3.28.3-1build7 \
        ninja-build=1.11.1-2 \
        git=1:2.43.0-1ubuntu7.4 \
        gcc \
        make \
        gcc-arm-none-eabi=15:13.2.rel1-2 \
        libnewlib-arm-none-eabi=4.4.0.20231231-2 \
        qemu-system-arm=1:8.2.2+ds-0ubuntu1.18 \
    && rm -rf /var/lib/apt/lists/*

# The source tree is mounted at /src and belongs to another user. Without this line git refuses to
# read it, and the version stamp would say no-git.
RUN git config --system --add safe.directory /src

WORKDIR /src
