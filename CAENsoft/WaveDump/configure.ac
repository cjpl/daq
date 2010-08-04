#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.50)
AC_INIT(wavedump, 1.9, support.computing@caen.it)
AC_CONFIG_AUX_DIR([src])
AM_INIT_AUTOMAKE(wavedump, 1.9)
AC_CONFIG_SRCDIR([inc/wave.h])
#AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_DEFINE(LINUX,[1],"Define LINUX")
AC_DEFINE(__USE_CAEN_VME_BRIDGES__,[1],"Define __USE_CAEN_VME_BRIDGES__")
# Checks for libraries.
AC_CHECK_HEADER(CAENVMElib.h,[a=0],[a=1])
AC_CHECK_HEADER(CAENVMEoslib.h,[a=0],[a=1])
AC_CHECK_HEADER(CAENVMEtypes.h,[a=0],[a=1])
AC_CHECK_LIB(CAENVME,CAENVME_Init,[a=0], [a=1])
if test $a == 1; then
        AC_MSG_ERROR([
                CAENVMELib must be installed on your system.])
fi

AC_CHECK_PROG(GNUPLOT,gnuplot,"ok","no")
if test $GNUPLOT == "no"; then
        AC_MSG_WARN(
	*****************************************************************
		    GnuPLot not fount WaveDump will be unable to show graphical data.
		    *****************************************************************
	)
fi

AC_MSG_WARN
# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([malloc.h stdint.h stdlib.h string.h sys/time.h sys/timeb.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_SELECT_ARGTYPES
AC_CHECK_FUNCS([atexit gettimeofday memset pow select strstr])
AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
