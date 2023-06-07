SRCDIR=src
BUILDDIR=build
VPATH=$(SRCDIR)
PERF_OPTS=-O3 -ffast-math -funroll-loops
GCC=gcc $(PERF_OPTS)
PPDEFS=-DRTL_WS_DEBUG
PROGRAM=$(BUILDDIR)/rtl-ws-server
CSOURCEFILES=$(shell find $(SRCDIR) -iname \*.c)
COBJFILES=$(subst .c,.o,$(subst $(SRCDIR),$(BUILDDIR),$(CSOURCEFILES)))

.PHONY: all build prepare clean run

build: $(PROGRAM)
	
all: clean build
	
$(PROGRAM): $(COBJFILES) $(CPPOBJFILES)
	$(GCC) $^ -lpthread -lwebsockets -lrtlsdr -lrt -lm -lfftw3 -o $@
	@ls -l build/rtl-ws-server >> build.txt

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(GCC) $(PPDEFS) -c -B $(SRCDIR) $< -o $@

prepare:
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

run: build
	open http://localhost:8090
	./build/rtl-ws-server
	

