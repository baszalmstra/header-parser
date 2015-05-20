#pragma once

#include "tokenizer.h"
#include <string>

enum class ScopeType
{
  kGlobal,
  kNamespace,
  kClass
};

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

  void PushScope(const std::string& name, ScopeType scopeType);
  void PopScope();
  std::string GetFullyQualifiedName(const std::string& name);

  void ParseNamespace();

private:
  struct Scope
  {
    ScopeType type;
    std::string name;
  };

  Scope scopes_[64];
  Scope *topScope_;

  void ParseClass();
};


