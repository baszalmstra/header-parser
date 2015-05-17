#pragma once

#include <cstdint>
#include <string>

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
  std::size_t startPos;
  std::size_t startLine;
  std::string token;
};