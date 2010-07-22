## 
#  Find the ROOT system default installed on system
#
#  ROOT is a platform developed in CERN for nuclear and particle physics.
#
# This module defines
#   ROOTSYS      --- The main directory which ROOT has been installed
#   ROOT_VER     --- ROOT version
#   ROOT_SVNVER  --- ROOT subversion revision number
#   ROOT_CFLAGS  --- CFLAGS while building with ROOT
#   ROOT_LIBS    --- The link flags while linking with ROOT
#   ROOT_GLIBS   --- As above, but include graphical libraries
#

find_program(ROOT_CONFIG root-config)

if(ROOT_CONFIG)

  set(ROOT_FOUND TRUE)

  execute_process(
    COMMAND ${ROOT_CONFIG} --prefix
    OUTPUT_VARIABLE ROOTSYS
    OUTPUT_STRIP_TRAILING_WHITESPACE )

  execute_process(
    COMMAND ${ROOT_CONFIG} --version
    OUTPUT_VARIABLE ROOT_VER
    OUTPUT_STRIP_TRAILING_WHITESPACE )

  execute_process(
    COMMAND ${ROOT_CONFIG} --svn-revision
    OUTPUT_VARIABLE ROOT_SVNVER
    OUTPUT_STRIP_TRAILING_WHITESPACE )

  # Add to libs: ${ROOT_CONFIG} --libs
  execute_process(
    COMMAND ${ROOT_CONFIG} --libs
    OUTPUT_VARIABLE ROOT_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE )

  # Add ROOT_GLIBS: ${ROOT_CONFIG} --glibs
  execute_process(
    COMMAND ${ROOT_CONFIG} --glibs
    OUTPUT_VARIABLE ROOT_GLIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE )

  # Add to CFLAGS: ${ROOT_CONFIG} --cflags
  execute_process(
    COMMAND ${ROOT_CONFIG} --cflags
    OUTPUT_VARIABLE ROOT_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE )

  message(STATUS "ROOTSYS (${ROOT_VER}, svn: r${ROOT_SVNVER}) found at: ${ROOTSYS}")

  # Find static ROOT libraries
  execute_process(
    COMMAND ${ROOT_CONFIG} --libdir
    OUTPUT_VARIABLE ROOT_LIBDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  find_file(ROOT_LIBA libRoot.a ${ROOT_LIBDIR})
  if(ROOT_LIBA)
    message(STATUS "Found static ROOT library at ${ROOT_LIBA}")
  else(ROOT_LIBA)
    message(STATUS "Static ROOT library not found!")
  endif(ROOT_LIBA)

else(ROOT_CONFIG)
  message(STATUS "ROOT system not found!")
endif(ROOT_CONFIG)
