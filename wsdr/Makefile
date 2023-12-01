SRCDIR=src
BUILDDIR=build
EXECUTABLE=wsdr

CC=gcc
PERF_OPTS=-O3 -ffast-math -funroll-loops -Wall -Wextra -std=gnu99
PERF_LIBS=-lpthread -lm -lfftw3
PPDEFS=-DRTL_WS_DEBUG

PROGRAM=$(BUILDDIR)/$(EXECUTABLE)
CSOURCEFILES=$(shell find $(SRCDIR) -iname \*.c)
COBJFILES=$(subst .c,.o,$(CSOURCEFILES))

DOCKERIMAGE=build_system

.PHONY: build pluto all prepare clean run docker dockerBuild dockerRun dockerClean

#=====================================================

build: PERF_LIBS+=-lrtlsdr
build: $(PROGRAM)

pluto: CC=arm-linux-gnueabihf-gcc
pluto: PERF_OPTS+=-mfloat-abi=hard --sysroot=files/sysroot-v0.37
pluto: PERF_LIBS+=-liio
pluto: $(PROGRAM)

#=====================================================
	
all: clean build

#=====================================================
	
# Linker
$(PROGRAM): $(COBJFILES)
	$(CC) $(PERF_OPTS) $^ $(PERF_LIBS) -o $@
	@ls -l $(PROGRAM) >> buildLog.txt

# Compile files
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(PERF_OPTS) $(PPDEFS) -c -B $(SRCDIR) $< -o $@

#=====================================================

clean:
	rm -rf $(BUILDDIR)

run: build
	# open http://localhost:8000
	./build/rtl-ws-server

#=====================================================

docker: dockerClean dockerBuild dockerRun

dockerBuild:
	docker build -t $(DOCKERIMAGE) -f Dockerfile .

dockerRun:
	docker run --rm -ti -v $(shell pwd):/project -p 8000:8000 $(DOCKERIMAGE) make pluto

dockerClean: 
	docker rm $(DOCKERIMAGE) -f && \
	docker rmi $(DOCKERIMAGE)

#=====================================================
	
uploadAndRun:
	scp $(PROGRAM) root@192.168.1.50:/tmp && \
	ssh root@192.168.1.50 /tmp/$(EXECUTABLE)
