PROJECT(header-parser)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

SET(SOURCES
  "test.cc"
  "token.h"
  "tokenizer.h"
  )

ADD_DEFINITIONS(-std=c++11)
ADD_EXECUTABLE(header-parser ${SOURCES})