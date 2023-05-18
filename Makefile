SRCDIR=src
BUILDDIR=build
VPATH=$(SRCDIR)
PERF_OPTS=-O3 -ffast-math -funroll-loops
GCC=gcc $(PERF_OPTS)
PPDEFS=-DRTL_WS_DEBUG
PROGRAM=$(BUILDDIR)/rtl-ws-server
CSOURCEFILES=$(shell ls $(SRCDIR)/*.c)
COBJFILES=$(subst .c,.o,$(subst $(SRCDIR),$(BUILDDIR),$(CSOURCEFILES)))
REAL_SENSOR:=1

ifeq ($(REAL_SENSOR), 1)
PPDEFS+=-DREAL_SENSOR
SENSORLIB=-lrtlsdr
endif

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

$(BUILDDIR)/%.o: %.c | prepare
	$(compilec)

prepare:
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

run: build
	open http://localhost:8090
	./build/rtl-ws-server
	

