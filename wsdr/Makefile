SRCDIR=src
BUILDDIR=build
VPATH=$(SRCDIR)
PERF_OPTS=-O3 -ffast-math -funroll-loops
PERF_LIBS=-lpthread -lm -lfftw3
CC=gcc
PPDEFS=-DRTL_WS_DEBUG
PROGRAM=$(BUILDDIR)/rtl-ws-server
CSOURCEFILES=$(shell find $(SRCDIR) -iname \*.c)
COBJFILES=$(subst .c,.o,$(subst $(SRCDIR),$(BUILDDIR),$(CSOURCEFILES)))

.PHONY: help wsdr pluto all build prepare clean run dockerBuild dockerRun

#=====================================================

help:
	@echo "Make wsdr, pluto"

wsdr: PPDEFS+=-DRTLSDR_SENSOR
wsdr: PERF_LIBS+=-lrtlsdr
wsdr: all

pluto: CC=arm-linux-gnueabihf-gcc
pluto: PERF_OPTS+=-mfloat-abi=hard --sysroot=files/sysroot-v0.37 -std=gnu99
pluto: PERF_LIBS+=-liio
pluto: all

#=====================================================

build: $(PROGRAM)
	
all: clean build
	
# Linker
$(PROGRAM): $(COBJFILES) $(CPPOBJFILES)
	$(CC) $(PERF_OPTS) $^ $(PERF_LIBS) -o $@
	@ls -l build/rtl-ws-server >> buildLog.txt

# Compile files
$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(PERF_OPTS) $(PPDEFS) -c -B $(SRCDIR) $< -o $@

prepare:
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

run: build
	# open http://localhost:8000
	./build/rtl-ws-server

dockerBuild:
	docker build -t build_system -f Dockerfile .

dockerRun:
	docker run --rm -ti -v $(shell pwd):/project -p 8000:8000 build_system make pluto

dockerClean: 
	docker rm build_system -f && \
	docker rmi build_system
	
