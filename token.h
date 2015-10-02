#pragma once

#include <cstdint>
#include <string>
#include <array>

enum class TokenType
{
  kNone,
  kSymbol,
  kIdentifier,
  kConst
};

enum class ConstType
{
  kString,
  kBoolean,
  kUInt32,
  kInt32,
  kUInt64,
  kInt64,
  kReal
};

struct Token
{
	TokenType tokenType;
  std::size_t startPos;
  std::size_t startLine;
  std::string token;

  ConstType constType;
  union
  {
    bool boolConst;
    uint32_t uint32Const;
    int32_t int32Const;
    uint64_t uint64Const;
    int64_t int64Const;
    double realConst;
  };
  std::string stringConst;
};