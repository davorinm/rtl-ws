SRCDIR=src
BUILDDIR=build
TARGET=rtl-ws-server
VPATH=$(SRCDIR)

CC=gcc
PERF_OPTS=-O3 -ffast-math -funroll-loops -Wall -Wextra -std=gnu99
PERF_LIBS=-lpthread -lm -lfftw3
PPDEFS=-DRTL_WS_DEBUG

PROGRAM=$(BUILDDIR)/$(TARGET)
CSOURCEFILES=$(shell find $(SRCDIR) -iname \*.c)
COBJFILES=$(subst .c,.o,$(subst $(SRCDIR),$(BUILDDIR),$(CSOURCEFILES)))

.PHONY: build pluto all prepare clean run dockerBuild dockerRun dockerClean

#=====================================================

build: PERF_LIBS+=-lrtlsdr
build: PPDEFS+=-DSENSOR_RTLSDR
build: $(PROGRAM)

pluto: CC=arm-linux-gnueabihf-gcc
pluto: PERF_OPTS+=-mfloat-abi=hard --sysroot=files/sysroot-v0.37
pluto: PERF_LIBS+=-liio
pluto: PPDEFS+=-DSENSOR_PLUTO
pluto: $(PROGRAM)

#=====================================================
	
all: clean build

#=====================================================
	
# Linker
$(PROGRAM): $(COBJFILES) $(CPPOBJFILES)
	$(CC) $(PERF_OPTS) $^ $(PERF_LIBS) -o $@
	@ls -l build/rtl-ws-server >> buildLog.txt

# Compile files
$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(PERF_OPTS) $(PPDEFS) -c -B $(SRCDIR) $< -o $@

#=====================================================

clean:
	rm -rf $(BUILDDIR)

run: build
	# open http://localhost:8000
	./build/rtl-ws-server

#=====================================================

dockerBuild:
	docker build -t build_system -f Dockerfile .

dockerRun:
	docker run --rm -ti -v $(shell pwd):/project -p 8000:8000 build_system make pluto

dockerClean: 
	docker rm build_system -f && \
	docker rmi build_system
	
