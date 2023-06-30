SRCDIR=src
BUILDDIR=build
VPATH=$(SRCDIR)
PERF_OPTS=-O3 -ffast-math -funroll-loops -mfloat-abi=hard --sysroot=files/sysroot-v0.37 -std=gnu99
PERF_LIBS=-lpthread -liio -lm -lfftw3
COMPILER=arm-linux-gnueabihf-gcc $(PERF_OPTS)
PPDEFS=-DRTL_WS_DEBUG
PROGRAM=$(BUILDDIR)/rtl-ws-server
CSOURCEFILES=$(shell find $(SRCDIR) -iname \*.c)
COBJFILES=$(subst .c,.o,$(subst $(SRCDIR),$(BUILDDIR),$(CSOURCEFILES)))

DOCKERIMAGE=build_system

.PHONY: all build prepare clean run dockerBuild dockerRun

build: $(PROGRAM)
	
all: clean build
	
$(PROGRAM): $(COBJFILES)
	$(COMPILER) $^ $(PERF_LIBS) -o $@
	@ls -l build/rtl-ws-server >> build.txt

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(COMPILER) $(PPDEFS) -c -B $(SRCDIR) $< -o $@

prepare:
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

run: build
	# open http://localhost:8000
	./build/rtl-ws-server

dockerBuild:
	docker build -t $(DOCKERIMAGE) -f Dockerfile .

dockerRun:
	docker run --rm -ti -v $(shell pwd):/project -p 8000:8000 $(DOCKERIMAGE)

dockerClean: 
	docker rm $(DOCKERIMAGE) -f && \
	docker rmi $(DOCKERIMAGE)
	
# arm-linux-gnueabihf-gcc -mfloat-abi=hard --sysroot=files/sysroot -std=gnu99 -g -o pluto_stream src/ad9361-iiostream.c -lpthread -liio -lm -Wall -Wextra --warn-unused-variable
