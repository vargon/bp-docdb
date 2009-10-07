#####
# SearchDB
#####

cmake_minimum_required(VERSION 2.6)

INCLUDE(BuildConfigs.cmake)

SET (serviceName SearchDB)

PROJECT(${serviceName}Service)

SET (outputDir "${CMAKE_CURRENT_BINARY_DIR}/${serviceName}")

# default to a debug build.  This gets us symbols by default under
# the cmake makefile generator
IF (NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Debug")
ENDIF ()

INCLUDE_DIRECTORIES(
  "${CMAKE_CURRENT_SOURCE_DIR}/external/${CMAKE_SYSTEM_NAME}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/external/bpsdk/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/external/bp-service-tools/CppTools/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/external/bp-service-framework/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/external/clucene/clucene-core-0.9.21/src"
)

LINK_DIRECTORIES(
  "${CMAKE_CURRENT_SOURCE_DIR}/external/${CMAKE_SYSTEM_NAME}/lib"
  "${CMAKE_CURRENT_SOURCE_DIR}/external/${CMAKE_SYSTEM_NAME}/lib/${CMAKE_BUILD_TYPE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/lib"
#  "${CMAKE_CURRENT_SOURCE_DIR}/external/clucene/clucene-core-0.9.21/src/.libs"
)

#SET(EXT_SRCS
#  ${CMAKE_CURRENT_SOURCE_DIR}/external/bp-service-tools/CppTools/src/bptypeutil.cpp
#  ${CMAKE_CURRENT_SOURCE_DIR}/external/bp-service-tools/CppTools/src/bpurlutil.cpp
#)

SET(SRCS src/service.cpp)
SET(HDRS src/service.h)

FILE(GLOB_RECURSE OS_SRCS "external/*.h" "external/*.hh")

# add required OS libs here
SET(OSLIBS)

IF (APPLE)
  FIND_LIBRARY(CARBON_LIBRARY Carbon)
  MARK_AS_ADVANCED(CARBON_LIBRARY)
  SET(OSLIBS ${CARBON_LIBRARY})
ENDIF ()


ADD_LIBRARY(${serviceName} MODULE ${EXT_SRCS} ${SRCS} ${OS_SRCS} ${HDRS})

TARGET_LINK_LIBRARIES(${serviceName} clucene ${OSLIBS})

# create service directory
FILE(MAKE_DIRECTORY ${outputDir})

# copy in manifest
GET_TARGET_PROPERTY(loc ${serviceName} LOCATION)
GET_FILENAME_COMPONENT(ServiceLibrary "${loc}" NAME)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/manifest.json"
               "${outputDir}/manifest.json")  
ADD_CUSTOM_COMMAND(TARGET ${serviceName} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           \"${loc}\" \"${outputDir}\")
IF (APPLE)
  ADD_CUSTOM_COMMAND(TARGET ${serviceName} POST_BUILD
                     COMMAND strip -x \"${outputDir}/${ServiceLibrary}\")
ENDIF (APPLE)
