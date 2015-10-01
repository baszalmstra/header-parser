#include <iostream>
#include <vector>
#include "parser.h"
#include "token.h"

//--------------------------------------------------------------------------------------------------
Parser::Parser() : writer_(buffer_)
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

  // Start the array
  writer_.StartArray();

  // Reset scope
  topScope_ = scopes_;
  topScope_->name = "";
  topScope_->type = ScopeType::kGlobal;
  topScope_->currentAccessControlType = AccessControlType::kPublic;

  // Parse all statements in the file
  while(ParseStatement())
  {

  }

  // End the array
  writer_.EndArray();

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
  else if(token.token == "R_FUNCTION")
    ParseFunction();
  else if (token.token == "namespace")
    ParseNamespace();
  else if (ParseAccessControl(token, topScope_->currentAccessControlType))
    RequireSymbol(":");
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
  }
  else if(token.token == "include")
  {
    Token includeToken;
    GetToken(includeToken);

    writer_.StartObject();
    writer_.String("type");
    writer_.String("include");
    writer_.String("file");
    writer_.String(includeToken.token.c_str());
    writer_.EndObject();
  }

  // Skip past the end of the token
  char lastChar = '\n';
  do
  {
    // Skip to the end of the line
    char c;
    while((c = GetChar()) != '\n')
      lastChar = c;

  } while(multiLineEnabled && lastChar == '\\');
}

//--------------------------------------------------------------------------------------------------
bool Parser::SkipDeclaration(Token &token)
{
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

  return true;
}

//--------------------------------------------------------------------------------------------------
void Parser::ParseEnum()
{
  writer_.StartObject();
  writer_.String("type");
  writer_.String("enum");

  WriteCurrentAccessControlType();

  ParseMacroMeta();

  RequireIdentifier("enum");

  // C++1x enum class type?
  bool isEnumClass = MatchIdentifier("class");

  // Parse enum name
  Token enumToken;
  if(!GetIdentifier(enumToken))
    throw; // Missing enum name?

  writer_.String("name");
  writer_.String(enumToken.token.c_str());

  if (isEnumClass)
  {
    writer_.String("cxxclass");
    writer_.Bool(isEnumClass);
  }

  // Parse C++1x enum base
  if(isEnumClass && MatchSymbol(":"))
  {
    Token baseToken;
    if(!GetIdentifier(baseToken))
      throw; // Missing enum base

    // Validate base token
    writer_.String("base");
    writer_.String(baseToken.token.c_str());
  }

  // Require opening brace
  RequireSymbol("{");

  writer_.String("members");
  writer_.StartArray();

  // Parse all the values
  Token token;
  while(GetIdentifier(token))
  {
    writer_.StartObject();
    
    // Store the identifier
    writer_.String("key");
    writer_.String(token.token.c_str());

    // Parse constant
    if(MatchSymbol("="))
    {
      // Just parse the value, not doing anything with it atm
      GetToken(token);
  
      // TODO: Output number if number
      writer_.String("value");
      writer_.String(token.token.c_str());
    }

    writer_.EndObject();

    // Next value?
    if(!MatchSymbol(","))
      break;
  }

  RequireSymbol("}");
  writer_.EndArray();

  MatchSymbol(";");

  writer_.EndObject();
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
void Parser::PushScope(const std::string &name, ScopeType scopeType, AccessControlType accessControlType)
{
  if(topScope_ == scopes_ + (sizeof(scopes_) / sizeof(Scope)) - 1)
    throw; // Max scope depth

  topScope_++;
  topScope_->type = scopeType;
  topScope_->name = name;
  topScope_->currentAccessControlType = accessControlType;
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
  writer_.StartObject();
  writer_.String("type");
  writer_.String("namespace");

  Token token;
  if(!GetIdentifier(token))
    throw; // Missing namespace name

  writer_.String("name");
  writer_.String(token.token.c_str());

  RequireSymbol("{");

  writer_.String("members");
  writer_.StartArray();

  PushScope(token.token, ScopeType::kNamespace, AccessControlType::kPublic);

  while(!MatchSymbol("}"))
    ParseStatement();

  PopScope();

  writer_.EndArray();

  writer_.EndObject();
}

//-------------------------------------------------------------------------------------------------
bool Parser::ParseAccessControl(const Token &token, AccessControlType& type)
{
  if (token.token == "public")
  {
    type = AccessControlType::kPublic;
    return true;
  }
  else if (token.token == "protected")
  {
    type = AccessControlType::kProtected;
    return true;
  }
  else if (token.token == "private")
  {
    type = AccessControlType::kPrivate;
    return true;
  }
  
  return false;
}

//-------------------------------------------------------------------------------------------------
void Parser::WriteCurrentAccessControlType()
{
  // Writing access is not required if the current scope is not owned by a class
  if (topScope_->type != ScopeType::kClass)
    return;

  WriteAccessControlType(current_access_control_type());
}

//-------------------------------------------------------------------------------------------------
void Parser::WriteAccessControlType(AccessControlType type)
{
  writer_.String("access");
  switch (type)
  {
  case AccessControlType::kPublic:
    writer_.String("public");
    break;
  case AccessControlType::kProtected:
    writer_.String("protected");
    break;
  case AccessControlType::kPrivate:
    writer_.String("private");
    break;
  default:
    throw; // Unknown access control type
  }
}

//--------------------------------------------------------------------------------------------------
void Parser::ParseClass()
{
  writer_.StartObject();
  writer_.String("type");
  writer_.String("class");

  WriteCurrentAccessControlType();

  ParseMacroMeta();

  RequireIdentifier("class");

  // Get the class name
  Token classNameToken;
  if(!GetIdentifier(classNameToken))
    throw; // Missing class name

  writer_.String("name");
  writer_.String(classNameToken.token.c_str());

  // Match base types
  if(MatchSymbol(":"))
  {
    writer_.String("parents");
    writer_.StartArray();

    do
    {
      writer_.StartObject();

      Token accessOrName;
      if (!GetIdentifier(accessOrName))
        throw; // Missing class or access control specifier

      // Parse the access control specifier
      AccessControlType accessControlType = AccessControlType::kPrivate;
      if (ParseAccessControl(accessOrName, accessControlType))
        GetIdentifier(accessOrName);

      // Get the name of the class
      WriteAccessControlType(accessControlType);
      writer_.String("name");
      writer_.String(accessOrName.token.c_str());

      writer_.EndObject();
    }
    while (MatchSymbol(","));

    writer_.EndArray();
  }

  RequireSymbol("{");

  writer_.String("members");
  writer_.StartArray();

  PushScope(classNameToken.token, ScopeType::kClass, AccessControlType::kPrivate);

  while(!MatchSymbol("}"))
    ParseStatement();

  PopScope();

  writer_.EndArray();

  RequireSymbol(";");

  writer_.EndObject();
}

//--------------------------------------------------------------------------------------------------
void Parser::ParseFunction()
{
  writer_.StartObject();
  writer_.String("type");
  writer_.String("function");

  ParseMacroMeta();
  WriteCurrentAccessControlType();

  // Process method specifiers in any particular order
  bool isVirtual = false, isInline = false, isConstExpr = false;
  for(bool matched = true; matched;)
  {
    matched = (!isVirtual && (isVirtual = MatchIdentifier("virtual"))) ||
        (!isInline && (isInline = MatchIdentifier("inline"))) ||
        (!isConstExpr && (isConstExpr = MatchIdentifier("constexpr")));
  }

  // Write method specifiers
  if (isVirtual)
  {
    writer_.String("virtual");
    writer_.Bool(isVirtual);
  }
  if (isInline)
  {
    writer_.String("inline");
    writer_.Bool(isInline);
  }
  if (isConstExpr)
  {
    writer_.String("constexpr");
    writer_.Bool(isConstExpr);
  }

  // Parse the return type
  writer_.String("returnType");
  ParseType();

  // Parse the name of the method
  Token nameToken;
  if(!GetIdentifier(nameToken))
    throw; // Expected method name

  writer_.String("name");
  writer_.String(nameToken.token.c_str());

  writer_.String("arguments");
  writer_.StartArray();

  // Start argument list from here
  MatchSymbol("(");

  // Is there an argument list in the first place or is it closed right away?
  if (!MatchSymbol(")"))
  {
    // Walk over all arguments
    do
    {
      writer_.StartObject();

      // Get the type of the argument
      writer_.String("type");
      ParseType();
      
      // Parse the name of the function
      writer_.String("name");
      if (!GetIdentifier(nameToken))
        throw; // Expected identifier
      writer_.String(nameToken.token.c_str());

      // Parse default value
      if (MatchSymbol("="))
      {
        writer_.String("defaultValue");

        std::string defaultValue;
        Token token;
        while (GetToken(token))
        {
          if (token.token == "," ||
            token.token == ")")
          {
            UngetToken(token);
            break;
          }
          defaultValue += token.token;
        }
        writer_.String(defaultValue.c_str());
      }

      writer_.EndObject();
    } while (MatchSymbol(",")); // Only in case another is expected

    MatchSymbol(")");
  }

  writer_.EndArray();
  
  // Optionally parse constness
  if (MatchIdentifier("const"))
  {
    writer_.String("const");
    writer_.Bool(true);
  }

  // Pure?
  if (MatchSymbol("="))
  {
    Token token;
    if (!GetToken(token) || token.token != "0")
      throw; // Expected nothing else than null

    writer_.String("abstract");
    writer_.Bool(true);
  }

  writer_.EndObject();
}

//--------------------------------------------------------------------------------------------------
void Parser::ParseType()
{
  struct Indirection
  {
    bool isReference;
    bool isPointer;
    bool isConst;
    bool isVolatile;
  };

  std::vector<Indirection> indirectionQueue;
  indirectionQueue.emplace_back(Indirection { false, false, false, false });

  // Parse CV specifier
  bool isConst = false, isVolatile = false;
  for (bool matched = true; matched;)
  {
    matched = (!isConst && (isConst = MatchIdentifier("const"))) ||
      (!isVolatile && (isVolatile = MatchIdentifier("volatile")));
  }

  // Parse the declarator name
  std::string declarator;
  Token token;
  bool first = true;
  do
  {
    // Parse the declarator
    if (MatchSymbol("::"))
      declarator += "::";
    else if (!first)
      break;

    // Mark that this is not the first time in this loop
    first = false;

    // Match an identifier
    if (!GetIdentifier(token))
      throw; // Expected identifier

    declarator += token.token;

  } while (true);

  // Build stack of pointer values
  do
  {
    // CV specifier may also follow the declaration
    for (bool matched = true; matched;)
    {
      matched = (!isConst && (isConst = MatchIdentifier("const"))) ||
        (!isVolatile && (isVolatile = MatchIdentifier("volatile")));
    }

    // Store the cv-ness of the last indirection value
    indirectionQueue.back().isConst = isConst;
    indirectionQueue.back().isVolatile = isVolatile;
    isConst = isVolatile = false;

    // Check if there is indeed an indirection
    bool isReference = MatchSymbol("&");
    bool isPointer = !isReference && MatchSymbol("*");
    if (!isReference && !isPointer)
      break;

    // Add the indirection
    indirectionQueue.emplace_back(Indirection{ isReference, isPointer, false, false });
  } while (true);
     
  size_t indirectionCount = indirectionQueue.size();
  while (!indirectionQueue.empty())
  {
    Indirection indirection = indirectionQueue.back();
    indirectionQueue.pop_back();

    writer_.StartObject();

    if (indirection.isConst)
    {
      writer_.String("const");
      writer_.Bool(indirection.isConst);
    }

    if (indirection.isVolatile)
    {
      writer_.String("volatile");
      writer_.Bool(indirection.isVolatile);
    }
    
    if (!indirection.isPointer && !indirection.isReference)
    {
      writer_.String("type");
      writer_.String("literal");

      writer_.String("name");
      writer_.String(declarator.c_str());
    }
    else if (indirection.isPointer)
    {
      writer_.String("type");
      writer_.String("pointer");

      writer_.String("baseType");
    }
    else if (indirection.isReference)
    {
      writer_.String("type");
      writer_.String("reference");

      writer_.String("baseType");
    }
  }

  for (size_t i = 0; i < indirectionCount; ++i)
    writer_.EndObject();
}
