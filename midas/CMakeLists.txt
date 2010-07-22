#-*- mode: cmake -*-

## CMake pre-settings
cmake_minimum_required(VERSION 2.6)
cmake_policy(VERSION 2.6)
#cmake_policy(SET CMP0011 NEW)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

################################################################
## Project setup
project(MIDAS C CXX)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH    ${PROJECT_BINARY_DIR}/lib)
set(midas_cflags -g -O2)
#add_definitions(-g -O2)
set(svnver r4621)

################################################################
## Include utilities and options
# Find mxml
include(FindMXML)
include_directories(include ${MXML_PATH})

# OS specific flags
# OS-related CMake variables:
#  1. OS_EXTRA_UTILS   --- Extra utilities for specific OS
#  2. OS_EXTRA_LDFLAGS --- Extra link flags for specific OS
#  3. OS_EXTRA_DEFS    --- Extra definitions for specific OS
#  4. Other flags switched on for specific OS
include(OSflags)

include(FindROOT)  # Find ROOT environment
if(ROOT_FOUND)
  message("** Libraries and utilities will be compiled with ROOT.")
else(ROOT_FOUND)
  message("** WARNING: ROOTSYS not found in your system!")
  message("            All ROOT related codes will not be compiled!")
endif(ROOT_FOUND)
include(FindMySQL) # Find MySQL
include(FindODBC)  # Find ODBC
include(FindZLIB)  # Find zlib

## Options for MIDAS
set(NEED_CERNLIB TRUE)  # Need CERNLIB or not?
set(NEED_SHLIB   TRUE)  # Need shared library?
set(NEED_ROOT    TRUE)  # Need root?
set(NEED_FTPLIB  TRUE)  # Need ftp protocol?
#set(NEED_STRLCPY TRUE)  # Need strlcpy.h? -- maybe opened in OSflags.cmake
#set(MIDAS_MAX_EVENT_SIZE 1000000)  # maxmimum event size = ?

################################################################
## Setup options

## OS-specific
add_definitions(${OS_EXTRA_DEFS})
set(EXTRA_UTILS ${EXTRA_UTILS} ${OS_EXTRA_UTILS})
set(midas_cflags ${midas_cflags} ${OS_EXTRA_DEFS})
set(midas_libs   ${OS_EXTRA_LDFLAGS})

## Midas max event size
if(MIDAS_MAX_EVENT_SIZE)
  add_definitions(-DMAX_EVENT_SIZE=${MIDAS_MAX_EVENT_SIZE})
  set(midas_cflags ${midas_cflags} -DMAX_EVENT_SIZE=${MIDAS_MAX_EVENT_SIZE})
endif(MIDAS_MAX_EVENT_SIZE)

## strlcpy in "MXML"?
if(NEED_STRLCPY)
  set(EXTRA_OBJS ${EXTRA_OBJS} strlcpy)
  add_definitions( -DHAVE_STRLCPY )
  message(STATUS "Using Stephen's own strlcpy()")
endif(NEED_STRLCPY)

## FTPLIB needed?
if(NEED_FTPLIB)
  set(FTPLIB ftplib)
  add_definitions(-DINCLUDE_FTPLIB)
  message(STATUS "Using ftplib (FTP library)")
endif(NEED_FTPLIB)

## ODBC: -lodbc or -liodbc
if(ODBC_FOUND)
  message(STATUS "Add ODBC-related targets using ${ODBC_LIBRARY}")
  set(EXTRA_UTILS ${EXTRA_UTILS} mh2sql)
  add_definitions(-DHAVE_ODBC)
  set(midas_cflags ${midas_cflags} -DHAVE_ODBC)
  set(midas_libs   ${midas_libs} -l${ODBC_NAME})
else(ODBC_FOUND)
  message(STATUS "ODBC not found!")
endif(ODBC_FOUND)

## ZLIB
if(ZLIB_FOUND)
  add_definitions(-DHAVE_ZLIB)
  set(midas_cflags ${midas_cflags} -DHAVE_ZLIB)
  set(midas_libs   ${midas_libs} -lz)
endif(ZLIB_FOUND)

## ROOT
if(NEED_ROOT AND ROOT_FOUND)
  message(STATUS "Using ROOTSYS found at ${ROOTSYS}")
  add_definitions(-DHAVE_ROOT)

  # Add rpath
  if(NEED_RPATH)
    execute_process(
      COMMAND ${ROOT_CONFIG} --libdir
      OUTPUT_VARIABLE ROOT_LIBDIR
      OUTPUT_STRIP_TRAILING_WHITESPACE )
    set(ROOT_LIBS ${ROOT_LIBS} -Wl,-rpath,${ROOT_LIBDIR})
  endif(NEED_RPATH)

  ## libRoot.a: Static ROOT library
  # if(NEED_LIBROOTA AND ROOT_LIBA)
  #   set(LDFLAGS ${LDFLAGS} ${ROOT_LIBDIR}/libRoot.a -lssl -ldl -lcrypt)
  #   message(STATUS "Using statlic ROOT library: ${ROOT_LIBA}")
  # else(NEED_LIBROOTA AND ROOT_LIBA)
  #   message("   Static ROOT library not used!")
  # endif(NEED_LIBROOTA AND ROOT_LIBA)

  set(midas_cflags ${midas_cflags} -DHAVE_ROOT)
endif(NEED_ROOT AND ROOT_FOUND)

## Convert LDFLAGS from list to string
foreach(f ${midas_libs})
  set(LDFLAGS "${f} ${LDFLAGS}")
endforeach(f)

################################################################
# MIDAS library

## Objects needed to build the libmidas(.a,.so,..)
foreach(obj ${FTPLIB} midas system mrpc odb ybos history alarm elog)
  set(LIB_OBJS ${LIB_OBJS} src/${obj}.c)
endforeach(obj)
foreach(obj mxml ${EXTRA_OBJS})
  set(LIB_OBJS ${LIB_OBJS} ${MXML_PATH}/${obj}.c)
endforeach(obj)
#if(ODBC_FOUND)
#  message(STATUS "ODBC found, but not include history_odbc.cxx")
#endif(ODBC_FOUND)
set(LIB_OBJS ${LIB_OBJS} src/history_sql.cxx src/history_midas.cxx)
message(STATUS "LIB_OBJS: ${LIB_OBJS}")

## Add static lib
add_library(midas-static STATIC ${LIB_OBJS})
set_target_properties(midas-static PROPERTIES OUTPUT_NAME "midas")
## Add shared lib
if(NEED_SHLIB)
  add_library(midas-shared SHARED ${LIB_OBJS})
  set_target_properties(midas-shared PROPERTIES OUTPUT_NAME "midas")
endif(NEED_SHLIB)

################################################################
# Utilities

## Core Utilties
set(CORE_UTILS mserver mhttpd mlogger odbedit lazylogger)

add_executable(mserver    src/mserver.c)
add_executable(mlogger    src/mlogger.c)
add_executable(mhttpd     src/mhttpd.c  src/mgd.c )
add_executable(odbedit    src/odbedit.c src/cmdedit.c )
add_executable(lazylogger src/lazylogger.c)

foreach(exe ${CORE_UTILS})
  target_link_libraries(${exe} midas-static)
  set_target_properties(${exe} PROPERTIES LINK_FLAGS "${LDFLAGS}")
endforeach(exe)

# Specials
set_source_files_properties(src/lazylogger.c  PROPERTIES LANGUAGE CXX)
set_source_files_properties(src/mhttpd.c  PROPERTIES LANGUAGE CXX)
set_target_properties( mhttpd PROPERTIES LINKER_LANGUAGE CXX
  LINK_FLAGS "-lm ${LDFLAGS}")
set_source_files_properties(src/mlogger.c  PROPERTIES LANGUAGE CXX
  COMPILE_FLAGS ${ROOT_CFLAGS})
set_target_properties( mlogger PROPERTIES LINKER_LANGUAGE CXX
  LINK_FLAGS "${MYSQL_LIBS} ${ROOT_LIBS} ${LDFLAGS}" )

## Extra utilities
set(UTILS mtape  mhist  mstat  mdump  mchart  webpaw  odbhist  melog  mcnaf
  mhdump  mtransition # C++ codes
  ${EXTRA_UTILS}  )

foreach(exe mtape mhist mstat mdump melog mchart odbhist webpaw)
  add_executable(${exe} utils/${exe}.c)
endforeach(exe)
add_executable(mcnaf   utils/mcnaf.c drivers/camac/camacrpc.c)
add_executable(mhdump  utils/mhdump.cxx)
add_executable(mtransition src/mtransition.cxx)

if("${UTILS}" MATCHES "mh2sql")
  add_executable(mh2sql utils/mh2sql.cxx)
endif("${UTILS}" MATCHES "mh2sql")
if("${UTILS}" MATCHES "mlxspeaker")
  add_executable(mlxspeaker utils/mlxspeaker.c)
endif("${UTILS}" MATCHES "mlxspeaker")
if("${UTILS}" MATCHES "dio")
  add_executable(dio utils/dio.c)
endif("${UTILS}" MATCHES "dio")

foreach(exe ${UTILS})
  add_dependencies(${exe} midas-static)
  target_link_libraries(${exe} midas-static)
  set_target_properties(${exe} PROPERTIES  LINK_FLAGS "${LDFLAGS}")
endforeach(exe)

# midas-config
add_custom_command( OUTPUT midas-config
  DEPENDS genconf.sh
  COMMAND ${CMAKE_SOURCE_DIR}/genconf.sh
  ARGS ${CMAKE_INSTALL_PREFIX} \"${midas_cflags}\" \"${midas_libs}\" ${svnver}
  WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} )
add_custom_target(midas-conf ALL DEPENDS midas-config)

## Scripts
set(SCRIPTS utils/stripchart.tcl ${EXECUTABLE_OUTPUT_PATH}/midas-config)   # TCL script

################################################################
# Special targets

## generating mfe.o
add_custom_command( OUTPUT ${LIBRARY_OUTPUT_PATH}/mfe.o
  DEPENDS src/mfe.c
  COMMAND ${CMAKE_C_COMPILER}
  ARGS -I${CMAKE_SOURCE_DIR}/include -c ${CMAKE_SOURCE_DIR}/src/mfe.c
  WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH})
add_custom_target(mfe ALL DEPENDS ${LIBRARY_OUTPUT_PATH}/mfe.o)

## generating fal.o
add_custom_command( OUTPUT ${LIBRARY_OUTPUT_PATH}/fal.o
  DEPENDS src/fal.c
  COMMAND ${CMAKE_C_COMPILER}
  ARGS -I${CMAKE_SOURCE_DIR}/include -Dextname -DMANA_LITE
  -c ${CMAKE_SOURCE_DIR}/src/fal.c
  WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH})
add_custom_target(fal ALL DEPENDS ${LIBRARY_OUTPUT_PATH}/fal.o)

## analyzer objects: mana.o
add_custom_command( OUTPUT ${LIBRARY_OUTPUT_PATH}/mana.o
  DEPENDS src/mana.c
  COMMAND ${CMAKE_C_COMPILER}
  ARGS -I${CMAKE_SOURCE_DIR}/include -c ${CMAKE_SOURCE_DIR}/src/mana.c
  WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH})
add_custom_target(mana ALL DEPENDS ${LIBRARY_OUTPUT_PATH}/mana.o)

set(EXTRA_MOBJS mfe.o fal.o mana.o)

# rmana.o
if(NEED_ROOT AND ROOT_FOUND)
  set(ARG_LIST "-DUSE_ROOT ${ROOT_CFLAGS} -I${CMAKE_SOURCE_DIR}/include -o rmana.o -c ${CMAKE_SOURCE_DIR}/src/mana.c")
  separate_arguments(ARG_LIST)
  add_custom_command( OUTPUT ${LIBRARY_OUTPUT_PATH}/rmana.o
    DEPENDS src/mana.c
    COMMAND ${CMAKE_CXX_COMPILER}
    ARGS ${ARG_LIST}
    WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH})
  add_custom_target(rmana ALL DEPENDS ${LIBRARY_OUTPUT_PATH}/rmana.o )
  set(EXTRA_MOBJS ${EXTRA_MOBJS} rmana.o )
endif(NEED_ROOT AND ROOT_FOUND)

## hmana.o
if(NEED_CERNLIB)
  add_custom_command( OUTPUT ${LIBRARY_OUTPUT_PATH}/hmana.o
    DEPENDS src/mana.c
    COMMAND ${CMAKE_C_COMPILER}
    ARGS -I${CMAKE_SOURCE_DIR}/include -Dextname -DHAVE_HBOOK -o hmana.o
    -c ${CMAKE_SOURCE_DIR}/src/mana.c
    WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH})
  add_custom_target(hmana ALL DEPENDS ${LIBRARY_OUTPUT_PATH}/hmana.o)
  set(EXTRA_MOBJS ${EXTRA_MOBJS} hmana.o )
endif(NEED_CERNLIB)

################################################################
#  Building drivers and modules -- Not yet

################################################################
# Building examples -- Not yet

################################################################
#  Installation 

## Library and objects (custom targets)
install(TARGETS midas-static ARCHIVE DESTINATION lib)
if(NEED_SHLIB)
  install(TARGETS midas-shared LIBRARY DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE WORLD_EXECUTE WORLD_READ)
endif(NEED_SHLIB)

foreach(obj ${EXTRA_MOBJS})
  install(FILES ${PROJECT_BINARY_DIR}/lib/${obj} DESTINATION lib/midas)
endforeach(obj)

## Core and extra utilities, scripts
install(TARGETS ${CORE_UTILS} ${UTILS}
  RUNTIME DESTINATION bin
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  GROUP_READ GROUP_EXECUTE WORLD_EXECUTE WORLD_READ)
install(PROGRAMS ${SCRIPTS} DESTINATION bin)

## include, src, examples, drivers
set(MSHAREDIR "share/midas")
if(NOT MSHAREDIR)
  set(MSHAREDIR ".")
endif(NOT MSHAREDIR)
install(DIRECTORY include/ DESTINATION include/midas
  FILES_MATCHING PATTERN "*.h")
install(DIRECTORY src      DESTINATION ${MSHAREDIR})
install(DIRECTORY examples DESTINATION ${MSHAREDIR})
install(DIRECTORY drivers  DESTINATION ${MSHAREDIR})

