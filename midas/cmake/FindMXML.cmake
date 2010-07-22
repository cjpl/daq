#
# Find the mxml sources needed
#
# Provide:
#   MXML_DIR  -- the directory contains xml.h/c, strlcpy.h/c
#

set(MXML_FOUND FALSE)

find_path(MXML_PATH mxml.h
  mxml/
  ../mxml/
  ${CMAKE_SOURCE_DIR}/../mxml/
  /opt/DAQ/bot/mxml/
  DOC "Specify the directory containing mxml.h"
  )

if(MXML_PATH)
  set(MXML_FOUND TRUE)
  message(STATUS "MXML directory: ${MXML_PATH}")
else(MXML_PATH)
  message(FATAL_ERROR "Failed to found the mxml source directory!")
endif(MXML_PATH)

