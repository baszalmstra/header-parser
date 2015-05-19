#pragma once

#include "tokenizer.h"

class Parser : private Tokenizer
{
public:
  Parser();
  virtual ~Parser();

  // No copying of parser
  Parser(const Parser& other) = delete;
  Parser(Parser&& other) = delete;

  // Parses the given input
  bool Parse(const char* input);

protected:
  /// Called to parse the next statement. Returns false if there are no more statements.
  bool ParseStatement();
  bool ParseDeclaration(Token &token);
  void ParseDirective();
  bool SkipDeclaration(Token &token);
  void ParseEnum();
  void ParseMacroMeta();
};


