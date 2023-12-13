SRCDIR=src
BUILDDIR=build
EXECUTABLE=wsdr
PROGRAM=$(BUILDDIR)/$(EXECUTABLE)

CC=gcc
PERF_OPTS=-O3 -ffast-math -funroll-loops -Wall -Wextra -std=gnu99
PERF_LIBS=-lpthread -lm -lfftw3
PPDEFS=-DRTL_WS_DEBUG

CFLAGS=-I/usr/include

# Embedding files
EMBED_FILES=1

ifeq ($(EMBED_FILES),1)
FILES_TO_EMBED ?= $(wildcard resources/*)
PERF_OPTS += -DMG_ENABLE_PACKED_FS=1
endif

# Souces
CSOURCEFILES=$(shell find $(SRCDIR) -iname \*.c)
COBJFILES=$(subst .c,.o,$(subst $(SRCDIR),$(BUILDDIR),$(CSOURCEFILES)))

# Docker
DOCKERIMAGE=build_system

.PHONY: build pluto build-pluto all clean pack_fs run docker dockerBuild dockerRun dockerClean uploadAndRun

#=====================================================

build: PERF_LIBS+=-lrtlsdr
build: PPDEFS+=-DSENSOR_RTLSDR
build: $(PROGRAM)

build-pluto: PERF_LIBS+=-liio
build-pluto: PPDEFS+=-DSENSOR_PLUTO
build-pluto: $(PROGRAM)

pluto: CC=arm-linux-gnueabihf-gcc
pluto: PERF_OPTS+=-mfloat-abi=hard --sysroot=files/sysroot-v0.38 -std=gnu99
pluto: PERF_LIBS=-lfftw3 -lpthread -liio -lm
pluto: PPDEFS+=-DSENSOR_PLUTO
pluto: pack_fs $(PROGRAM)

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

pack_fs: $(FILES_TO_EMBED)
	gcc pack/pack.c -o pack/pack
	@pack/pack $(FILES_TO_EMBED) > src/mongoose/packed_fs.c
	$(DELETE) pack/pack

#=====================================================

clean:
	rm -rf $(BUILDDIR)

run: build
	# open http://localhost:8000
	./$(PROGRAM)

#=====================================================

docker: dockerClean dockerBuildImage dockerRun

dockerBuildImage:
	docker build -t $(DOCKERIMAGE) -f Dockerfile .

dockerRun:
	docker run --rm -ti -v $(shell pwd):/project -p 8000:8000 $(DOCKERIMAGE) make clean pluto

dockerTerminal:
	docker run --rm -ti -v $(shell pwd):/project -p 8000:8000 $(DOCKERIMAGE) /bin/bash

dockerClean: 
	docker rm $(DOCKERIMAGE) -f
	docker rmi $(DOCKERIMAGE)

#=====================================================
	
uploadAndRun:
	scp -O $(PROGRAM) root@192.168.1.50:/tmp && \
	ssh root@192.168.1.50 /tmp/$(EXECUTABLE)
