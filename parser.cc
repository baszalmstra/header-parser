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
    multiLineEnabled = true;

  // Skip past the end of the token
  char lastChar;
  do
  {
    // Skip to the end of the line
    char c;
    while((c = GetChar()) != '\n')
      lastChar = c;

  } while(multiLineEnabled && lastChar == '\\');

  // DEBUG
  std::cout << "Parsed directive: " << token.token << std::endl;
}

//--------------------------------------------------------------------------------------------------
bool Parser::SkipDeclaration(Token &token)
{
  // DEBUG
  std::cout << "Skipping directive, start: " << token.token;

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

  std::cout << "Parsed enum '" << enumToken.token << "' with " << values.size() << " values: ";
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
