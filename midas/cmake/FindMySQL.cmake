#
# Find MySQL installed
#
set(MYSQL_FOUND FALSE)

find_program(MYSQL_CONF mysql_config)

if(MYSQL_CONF)
  set(MYSQL_FOUND TRUE)

  # MYSQL_VER
  execute_process(
    COMMAND mysql_config --version
    OUTPUT_VARIABLE MYSQL_VER
    OUTPUT_STRIP_TRAILING_WHITESPACE )

  # MYSQL_CFLAGS
  execute_process(
    COMMAND mysql_config --cflags
    OUTPUT_VARIABLE MYSQL_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE )

  # MYSQL_LIBS
  execute_process(
    COMMAND mysql_config --libs
    OUTPUT_VARIABLE MYSQL_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE )

  message(STATUS "MySQL found: Version ${MYSQL_VER}")
endif(MYSQL_CONF)

