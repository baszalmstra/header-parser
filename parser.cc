#include <iostream>
#include <vector>
#include "parser.h"
#include "token.h"

//--------------------------------------------------------------------------------------------------
Parser::Parser()
{

}

//--------------------------------------------------------------------------------------------------
Parser::~Parser()
{

}

//--------------------------------------------------------------------------------------------------
bool Parser::Parse(const char *input)
{
  // Pass the input to the tokenizer
  Reset(input);

  // Reset scope
  topScope_ = scopes_;
  topScope_->name = "";
  topScope_->type = ScopeType::kGlobal;

  // Parse all statements in the file
  while(ParseStatement())
  {

  }

  return true;
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseStatement()
{
  Token token;
  if(!GetToken(token))
    return false;

  if(!ParseDeclaration(token))
    throw;

  return true;
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseDeclaration(Token &token)
{
  if(token.token == "#")
    ParseDirective();
  else if(token.token == ";")
    ; // Empty statement
  else if(token.token == "R_ENUM")
    ParseEnum();
  else if(token.token == "R_CLASS")
    ParseClass();
  else if(token.token == "namespace")
    ParseNamespace();
  else
    return SkipDeclaration(token);

  return true;
}

//--------------------------------------------------------------------------------------------------
void Parser::ParseDirective()
{
  Token token;

  // Check the compiler directive
  if(!GetIdentifier(token))
    throw; // Missing compiler directive after #

  bool multiLineEnabled = false;
  if(token.token == "define")
  {
    multiLineEnabled = true;
    std::cout << "Skipping directive: " << token.token << std::endl;
  }
  else if(token.token == "include")
  {
    Token includeToken;
    GetToken(includeToken);

    std::cout << "Parsed include directive: \"" << includeToken.token << "\"" << std::endl;
  }
  else
    std::cout << "Skipping directive: " << token.token << std::endl;

  // Skip past the end of the token
  char lastChar = '\n';
  do
  {
    // Skip to the end of the line
    char c;
    while((c = GetChar()) != '\n')
      lastChar = c;

  } while(multiLineEnabled && lastChar == '\\');

  // DEBUG
}

//--------------------------------------------------------------------------------------------------
bool Parser::SkipDeclaration(Token &token)
{
  // DEBUG
  std::cout << "Skipping declaration, start: " << token.token;

  int32_t scopeDepth = 0;
  while(GetToken(token))
  {
    if(token.token == ";" && scopeDepth == 0)
      break;

    if(token.token == "{")
      scopeDepth++;

    if(token.token == "}")
    {
      --scopeDepth;
      if(scopeDepth == 0)
        break;
    }
  }

  std::cout << ", end: " << token.token << std::endl;

  return true;
}

//--------------------------------------------------------------------------------------------------
void Parser::ParseEnum()
{
  ParseMacroMeta();

  RequireIdentifier("enum");

  // C++1x enum class type?
  bool isEnumClass = MatchIdentifier("class");

  // Parse enum name
  Token enumToken;
  if(!GetIdentifier(enumToken))
    throw; // Missing enum name?

  // Parse C++1x enum base
  if(isEnumClass && MatchSymbol(":"))
  {
    Token baseToken;
    if(!GetIdentifier(baseToken))
      throw; // Missing enum base

    // Validate base token
  }

  // Require opening brace
  RequireSymbol("{");

  // Parse all the values
  Token token;
  std::vector<std::string> values;
  while(GetIdentifier(token))
  {
    // Store the identifier
    values.push_back(token.token);

    // Parse constant
    if(MatchSymbol("="))
    {
      // Just parse the value, not doing anything with it atm
      GetToken(token);
    }

    // Next value?
    if(!MatchSymbol(","))
      break;
  }

  RequireSymbol("}");
  MatchSymbol(";");

  std::cout << "Parsed enum '" << GetFullyQualifiedName(enumToken.token) << "' with " << values.size() << " values: ";
  for(std::size_t i = 0; i < values.size(); ++i)
  {
    if(i > 0)
      std::cout << ", ";
    std::cout << values[i];
  }
  std::cout << std::endl;
}

//--------------------------------------------------------------------------------------------------
void Parser::ParseMacroMeta()
{
  RequireSymbol("(");
  while(!MatchSymbol(")"))
  {
    Token token;
    GetToken(token);
  }
}

//--------------------------------------------------------------------------------------------------
void Parser::PushScope(const std::string &name, ScopeType scopeType)
{
  if(topScope_ == scopes_ + (sizeof(scopes_) / sizeof(Scope)) - 1)
    throw; // Max scope depth

  topScope_++;
  topScope_->type = scopeType;
  topScope_->name = name;
}

//--------------------------------------------------------------------------------------------------
void Parser::PopScope()
{
  if(topScope_ == scopes_)
    throw; // Scope error

  topScope_--;
}

//--------------------------------------------------------------------------------------------------
void Parser::ParseNamespace()
{
  Token token;
  if(!GetIdentifier(token))
    throw; // Missing namespace name

  RequireSymbol("{");

  PushScope(token.token, ScopeType::kNamespace);

  while(!MatchSymbol("}"))
    ParseStatement();

  PopScope();
}

//--------------------------------------------------------------------------------------------------
void Parser::ParseClass()
{
  ParseMacroMeta();

  RequireIdentifier("class");

  // Get the class name
  Token classNameToken;
  if(!GetIdentifier(classNameToken))
    throw; // Missing class name

  // Match base types
//  if(MatchSymbol(":"))
//  {
//
//  }

  RequireSymbol("{");

  PushScope(classNameToken.token, ScopeType::kClass);

  while(!MatchSymbol("}"))
    ParseStatement();

  PopScope();

  RequireSymbol(";");
}

//--------------------------------------------------------------------------------------------------
std::string Parser::GetFullyQualifiedName(const std::string &name)
{
  std::string result;
  for(Scope *scope = scopes_; scope <= topScope_; scope++)
  {
    if(!result.empty())
      result += "::";
    result += scope->name;
  }
  if(!result.empty())
    result += "::";
  return result + name;
}
