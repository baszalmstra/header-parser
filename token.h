#pragma once

#include <cstdint>
#include <array>

enum class TokenType
{
  kNone,
  kSymbol,
  kIdentifier,
  kConst
};

struct Token
{
	TokenType tokenType;
  int32_t startPos;
  int32_t startLine;
  std::array<char, 1024> identifier;
};