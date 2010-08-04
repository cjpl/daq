#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
m4_include(wxwin.m4)
AC_PREREQ(2.50)
AC_INIT(CAENPLLConfig,2.0, support.computing@caen.it)
AC_CONFIG_SRCDIR([src/trash.cpp])
AC_CONFIG_AUX_DIR([src])
AM_INIT_AUTOMAKE(CAENPLLConfig, 2.0)
#AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

AC_PROG_INSTALL
AC_DEFINE(LINUX,[1],"Define LINUX")
# Checks for libraries.
AC_CHECK_HEADER(CAENVMElib.h,[a=0],[a=1])
AC_CHECK_HEADER(CAENVMEoslib.h,[a=0],[a=1])
AC_CHECK_HEADER(CAENVMEtypes.h,[a=0],[a=1])
AC_CHECK_LIB(CAENVME,CAENVME_Init,[a=0], [a=1])
if test $a == 1; then 
	AC_MSG_ERROR([
		CAENVMELib must be installed on your system.])
fi
AC_CHECK_LIB([CAENdigitizerSDK], [cvt_V1724_open],[b=0], [b=1])
if test $b == 1; then
        AC_MSG_ERROR([
                CAENdigitizerSDK must be installed on your system.])
fi
# check for xwWidgets
AM_OPTIONS_WXCONFIG
reqwx=2.4.0
AM_PATH_WXCONFIG($reqwx, wxWin=1)
if test "$wxWin" != 1; then
        AC_MSG_ERROR([
                wxWidgets must be installed on your system.

                Please check that wx-config is in path, the directory
                where wxWidgets libraries are installed (returned by
                'wx-config --libs' or 'wx-config --static --libs' command)
                is in LD_LIBRARY_PATH or equivalent variable and
                wxWidgets version is $reqwx or above.
                ])
fi

CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS"
CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS_ONLY"
CFLAGS="$CFLAGS $WX_CFLAGS_ONLY"
LIBS="$LIBS $WX_LIBS"

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([float.h malloc.h memory.h stdint.h stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([floor memset])
AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
