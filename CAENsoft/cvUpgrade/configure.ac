#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.50)
AC_INIT(cvUpgrade, 2.0, support.computing@caen.it)
AC_CONFIG_AUX_DIR([src])
AM_INIT_AUTOMAKE(cvUpgrade, 2.1)
AC_CONFIG_SRCDIR([include/flash.h])
#AC_CONFIG_HEADER([config.h])

# Checks for programs.
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
                CAENVMELib Library must be installed on your system.])
fi
AC_CHECK_HEADER(CAENComm.h,[a=0],[a=1])
AC_CHECK_LIB([CAENComm], [CAENComm_OpenDevice],[a=0], [a=1])
if test $a == 1; then
        AC_MSG_ERROR([
                CAENComm must be installed on your system 
		Please download the CAENComm from CAEN website. http://www.caen.it.])
fi

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([malloc.h stdlib.h string.h])

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
