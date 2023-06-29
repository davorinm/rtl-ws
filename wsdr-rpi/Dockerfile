# Build image:
#   docker build -t build_system -f Dockerfile .
# Run image:
#   docker run --rm -ti -v $(pwd):/project build_system
# Stop image
#   docker stop -t build_system

FROM ubuntu:22.04

RUN apt update && \
	apt upgrade -y && \
	apt install -y build-essential cmake

# COPY files/firmware/plutosdr-sysroot-v0.35 /root/sysroot
# COPY files/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabihf /root/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabihf
# COPY source /root/source

# ENV PATH=$PATH:/root/gcc-linaro-7.2.1-2017.11-x86_64_arm-linux-gnueabihf/bin

# COPY dockerEntrypoint.sh /opt/dockerEntrypoint.sh
# RUN chmod +x /opt/dockerEntrypoint.sh

WORKDIR /project

# set entrypoint
ENTRYPOINT [ "./dockerEntrypoint.sh" ]
