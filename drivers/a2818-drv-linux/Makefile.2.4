#=============================================================================
#
#             --- CAEN SpA - Computing Systems Division ---
#
#  Makefile
#
#
#  January  2006 :   Created.
#
#=============================================================================

###############################
# Kernel < 2.5
###############################
KERNELREV:=$(shell uname -r)
KERNELSRC:=/lib/modules/$(KERNELREV)/build

#  The include directory is right for RedHat systems, modify the
# kernel source location as appropriate for other distributions.
INCLUDE = $(KERNELSRC)/include/

COPTS   = -O2 -Wall -W -Wpointer-arith -Wcast-qual \
          -Wstrict-prototypes -Wmissing-prototypes \
          -Wmissing-declarations -Winline -Wcast-align


# Link-time flags to use
LDFLAGS=
# Additional link libraries
LIBS=

a2818.o: a2818.c a2818.h Makefile
	gcc -I${INCLUDE} -D__KERNEL__ -O -c a2818.c

all:	a2818.o

clean:
	rm -f *.o core
