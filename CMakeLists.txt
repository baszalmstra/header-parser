PROJECT(header-parser)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

SET(SOURCES
  "test.cc"
  "token.h"
  "tokenizer.cc"
  "tokenizer.h"
  "parser.cc"
  "parser.h"
  )

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/external/rapidjson/include)

ADD_DEFINITIONS(-std=c++11)
ADD_EXECUTABLE(header-parser ${SOURCES} parser.cc parser.h test.h)