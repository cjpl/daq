datadir=/etc
bin_PROGRAMS=wavedump
wavedump_SOURCES=wavedump.c WDplot.c keyb.c
wavedump_LDADD = -lCAENDigitizer -lm
INCLUDES = -I../inc
AM_CFLAGS=  -fPIC
pkgdata_DATA=WaveDumpConfig.txt
