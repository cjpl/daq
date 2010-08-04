# Created by: Exaos Lee <Exaos.Lee(at)gmail.com>, 2010-07
# Bridge libCAENVME.so and MIDAS/PSI mvme

## Targets:
# 1. default: libmvme_v2718.a, libmvme_v1718.a
# 2. shared: libmvme_v2718.so, libmvme_v1718.so
# 3. both: lib*.a, lib*.so

CC  = gcc
CXX = g++
CP := cp -p

WFLAGS   := -Wstrict-prototypes -Wmissing-prototypes \
            -Wmissing-declarations -Wimplicit -Wreturn-type -Wunused \
            -Wcomment -Wformat
#CFLAGS   := -g -ansi $(WFLAGS)

## Lib for Python interface
#PYDRVDIR := ../PySIS3100
#PYDRVLIB := ${PYDRVDIR}/$(DRV_LIBSO)

ifndef MIDASSYS
MIDASSYS := /opt/DAQ
endif
CFLAGS += -I$(MIDASSYS)/include/midas -DLINUX -DOS_LINUX

LIBS := libmvme_v2718.a
ifdef NEED_SHLIB
LIBS   += libmvme_v2718.so
CFLAGS += -fPIC -shared
endif

##################################################
all: $(LIBS)

shared: clean
	$(MAKE) NEED_SHLIB=1

.PHONY: clean clean-all

clean:
	rm -f *.o *~ \#* *Cint*

clean-all: clean
	rm -f lib*.a lib*.so

##################################################
%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

%.a:
	ar cr $@ $^

%.so:
	$(CC) -shared -o $@ $^

libmvme_v2718.a:  CAENV2718_mvme.o
libmvme_v2718.so: CAENV2718_mvme.o

