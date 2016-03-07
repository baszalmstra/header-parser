PROJECT(header-parser)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

SET(SOURCES
  "main.cc"
  "options.h"
  "token.h"
  "tokenizer.cc"
  "tokenizer.h"
  "parser.cc"
  "parser.h"
  "type_node.h"
  )

INCLUDE_DIRECTORIES(
	"${PROJECT_SOURCE_DIR}/external/rapidjson/include"
	"${PROJECT_SOURCE_DIR}/external/tclap/include"
	)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR
   ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  add_definitions(-std=c++11)
endif()
ADD_EXECUTABLE(header-parser ${SOURCES} parser.cc parser.h main.h)
