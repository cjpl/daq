
LIB	=	libCAENdigitizerSDK.so.1.3

CC	=	gcc

COPTS	=	-fPIC -DLINUX -Wall 

FLAGS	=	-shared -Wall,-soname -nostartfiles -s

DEPLIBS	=     -L/opt/DAQ/lib  -lCAENVME

LIBS	=	

INCLUDEDIR =	-I/opt/DAQ/include -I./ -I/usr/include/

#INSTALLDIR =	/usr/lib
INSTALLDIR =   /opt/DAQ

OBJS	=	cvt_board_commons.o cvt_V1724.o 

INCLUDES =	CAENVMElib.h CAENVMEtypes.h CAENVMEoslib.h

HEADERS  = 	cvt_board_commons.h cvt_common_defs.h cvt_V1724.h

#########################################################################

all	:	$(LIB)

clean	:
		/bin/rm -f $(OBJS) $(LIB)

install	:	$(LIB)
		/bin/rm -f $(INSTALLDIR)/lib/libCAENdigitizerSDK.so
		install $(LIB) $(INSTALLDIR)/lib
		ln -sf $(INSTALLDIR)/lib/$(LIB) $(INSTALLDIR)/lib/libCAENdigitizerSDK.so
		cp -p $(HEADERS) $(INSTALLDIR)/include/
#		#ldconfig

$(LIB)	:	$(OBJS)
		/bin/rm -f $(BINLIB)
		$(CC) $(FLAGS) -o $(LIB) $(OBJS) $(DEPLIBS)

$(OBJS)		:	 Makefile

%.o	:	%.c
		$(CC) $(COPTS) $(INCLUDEDIR) -c -o $@ $<

