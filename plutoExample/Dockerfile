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

WORKDIR /project

# set entrypoint
ENTRYPOINT [ "./dockerEntrypoint.sh" ]
