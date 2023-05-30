SRCDIR=src
BUILDDIR=build
VPATH=$(SRCDIR)
PERF_OPTS=-O3 -ffast-math -funroll-loops
GCC=gcc $(PERF_OPTS)
PPDEFS=-DRTL_WS_DEBUG
PROGRAM=$(BUILDDIR)/rtl-ws-server
#CSOURCEFILES=$(shell ls $(SRCDIR)/*.c)
CSOURCEFILES=$(shell find $(SRCDIR) -iname \*.c)
COBJFILES=$(subst .c,.o,$(subst $(SRCDIR),$(BUILDDIR),$(CSOURCEFILES)))

SENSORLIB=-lrtlsdr

define compilec
$(GCC) $(PPDEFS) -c -B $(SRCDIR) $< -o $@
endef

define link
$(GCC) $^ -lpthread -lwebsockets $(SENSORLIB) -lrt -lm -lfftw3 -o $@
endef

.PHONY: all build prepare clean run

build: $(PROGRAM)
	
all: clean build
	
$(PROGRAM): $(COBJFILES) $(CPPOBJFILES)
	$(link) 
	@ls -lh build/rtl-ws-server >> build.txt

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(compilec)

prepare:
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

run: build
	open http://localhost:8090
	./build/rtl-ws-server
	

