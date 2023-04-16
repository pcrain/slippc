# syntax=docker/dockerfile:1.4
FROM ubuntu:22.04

WORKDIR /slippc

RUN apt-get update && apt-get install -y --no-install-recommends \
  build-essential=12.9ubuntu3 \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

COPY . .

RUN <<EOF
mkdir -p /slippc/slippc/build
make statictest
./slippc-tests
make static all
EOF
