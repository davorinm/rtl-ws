# Build image:
#   docker build -t build_system -f Dockerfile .
# Run image:
#   docker run --rm -ti -v $(pwd):/project build_system
# Stop image
#   docker stop -t build_system

FROM ubuntu:22.04

RUN apt update && \
	apt upgrade -y && \
	apt install -y cmake

ENV PATH=$PATH:/project/files/gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf/bin

WORKDIR /project
