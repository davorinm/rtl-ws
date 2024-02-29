SRCDIR=src
BUILDDIR=build
EXECUTABLE=wsdr
PROGRAM=$(BUILDDIR)/$(EXECUTABLE)

CC=gcc
PERF_OPTS=-O3 -ffast-math -funroll-loops -Wall -Wextra -std=gnu99
PERF_LIBS=-lpthread -lm -lfftw3
PPDEFS=-DRTL_WS_DEBUG

CFLAGS=-I/usr/include

# Platform
PLUTO=1

# Embedding files
EMBED_FILES=1

# Souces
MAIN_CSOURCEFILES=src/main.c src/mongoose/mongoose.c src/web/web_handler.c
DSP_CSOURCEFILES=src/dsp/audio.c src/dsp/cic_decimate.c src/dsp/dsp.c src/dsp/resample.c src/dsp/decimator.c src/dsp/spectrum.c
TOOLS_CSOURCEFILES=src/tools/helpers.c src/tools/list.c src/tools/timer.c
PLUTO_CSOURCEFILES=src/sensor/sensor_pluto_helpers.c src/sensor/sensor_pluto.c
RTLSDR_CSOURCEFILES=src/sensor/sensor_rtlsdr.c
CSOURCEFILES=$(MAIN_CSOURCEFILES) $(DSP_CSOURCEFILES) $(TOOLS_CSOURCEFILES)

ifeq ($(PLUTO),1)
CSOURCEFILES += $(PLUTO_CSOURCEFILES)
PERF_LIBS += -liio
else
CSOURCEFILES += $(RTLSDR_CSOURCEFILES)
PERF_LIBS += -lrtlsdr
endif

ifeq ($(EMBED_FILES),1)
FILES_TO_EMBED ?= $(wildcard resources/*)
PERF_OPTS += -DMG_ENABLE_PACKED_FS=1
CSOURCEFILES += src/mongoose/packed_fs.c
endif

COBJFILES=$(subst .c,.o,$(subst $(SRCDIR),$(BUILDDIR),$(CSOURCEFILES)))

# Docker
DOCKERIMAGE=build_system

.PHONY: build pluto build-pluto all clean pack_fs run docker dockerBuild dockerRun dockerClean uploadAndRun

#=====================================================

build: $(PROGRAM)

build-pluto: $(PROGRAM)

pluto: CC=arm-linux-gnueabihf-gcc
pluto: PERF_OPTS+=-mfloat-abi=hard --sysroot=files/sysroot-v0.38 -std=gnu99
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
	ssh -t root@192.168.1.50 /tmp/$(EXECUTABLE)
