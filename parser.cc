#include <iostream>
#include <vector>
#include "parser.h"
#include "token.h"

//--------------------------------------------------------------------------------------------------
Parser::Parser(const Options &options) : options_(options), writer_(buffer_)
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
  std::vector<std::string>::const_iterator customMacroIt;
  if(token.token == "#")
    ParseDirective();
  else if(token.token == ";")
    ; // Empty statement
  else if(token.token == options_.enumNameMacro)
    ParseEnum(token);
  else if (token.token == options_.classNameMacro)
    ParseClass(token);
  else if (token.token == options_.functionNameMacro)
    ParseFunction(token);
  else if (token.token == "namespace")
    ParseNamespace();
  else if (ParseAccessControl(token, topScope_->currentAccessControlType))
    RequireSymbol(":");
  else if ((customMacroIt = std::find(options_.customMacros.begin(), options_.customMacros.end(), token.token)) != options_.customMacros.end())
    ParseCustomMacro(token, *customMacroIt);
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
    GetToken(includeToken, true);

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
    while(!this->is_eof() && (c = GetChar()) != '\n')
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
void Parser::ParseEnum(Token &startToken)
{
  writer_.StartObject();
  writer_.String("type");
  writer_.String("enum");
  writer_.String("line");
  writer_.Uint(startToken.startLine);

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
  
      writer_.String("value");
      WriteToken(token);
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
  writer_.String("meta");

  RequireSymbol("(");
  ParseMetaSequence();

  // Possible ;
  MatchSymbol(";");
}

//--------------------------------------------------------------------------------------------------
void Parser::ParseMetaSequence()
{
  writer_.StartObject();

  if(!MatchSymbol(")"))
  {
    do
    {
      // Parse key value
      Token keyToken;
      if (!GetIdentifier(keyToken))
        throw; // Expected identifier

      writer_.String(keyToken.token.c_str());

      // Simple value?
      if (MatchSymbol("=")) {
        Token token;
        if (!GetToken(token))
          throw; // Expected token

        WriteToken(token);
      }
        // Compound value
      else if (MatchSymbol("("))
        ParseMetaSequence();
        // No value
      else
        writer_.Null();
    } while (MatchSymbol(","));

    MatchSymbol(")");
  }

  writer_.EndObject();
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
void Parser::ParseClass(Token &token)
{
  writer_.StartObject();
  writer_.String("type");
  writer_.String("class");
  writer_.String("line");
  writer_.Uint(token.startLine);

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
      if (!ParseAccessControl(accessOrName, accessControlType))
        UngetToken(accessOrName);
      WriteAccessControlType(accessControlType);
      
      // Get the name of the class
      writer_.String("name");
      std::string declarator = ParseTypename();
      writer_.String(declarator.c_str());

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
void Parser::ParseFunction(Token &token)
{
  writer_.StartObject();
  writer_.String("type");
  writer_.String("function");
  writer_.String("line");
  writer_.Uint(token.startLine);

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
        GetToken(token);
        if(token.tokenType == TokenType::kConst)
          WriteToken(token);
        else
        {
          do
          {
            if (token.token == "," ||
              token.token == ")")
            {
              UngetToken(token);
              break;
            }
            defaultValue += token.token;
          } while (GetToken(token));
          writer_.String(defaultValue.c_str());
        }
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

  // Skip either the ; or the body of the function
  Token skipToken;
  SkipDeclaration(skipToken);
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
  std::string declarator = ParseTypename();

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

//-------------------------------------------------------------------------------------------------
std::string Parser::ParseTypename()
{
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

  // Parse template arguments
  if (MatchSymbol("<"))
  {
    declarator += "<";
    int templateCount = 1;
    while (templateCount > 0)
    {
      Token token;
      GetToken(token);
      if (token.token == "<")
        templateCount++;
      else if (token.token == ">")
        templateCount--;
      
      declarator += token.token;
    }
  }

  return declarator;
}

//----------------------------------------------------------------------------------------------------------------------
void Parser::WriteToken(const Token &token)
{
  if(token.tokenType == TokenType::kConst)
  {
    switch(token.constType)
    {
    case ConstType::kBoolean:
      writer_.Bool(token.boolConst);
      break;
    case ConstType::kUInt32:
      writer_.Uint(token.uint32Const);
      break;
    case ConstType::kInt32:
      writer_.Int(token.int32Const);
      break;
    case ConstType::kUInt64:
      writer_.Uint64(token.uint64Const);
      break;
    case ConstType::kInt64:
      writer_.Int64(token.int64Const);
      break;
    case ConstType::kReal:
      writer_.Double(token.realConst);
      break;
    case ConstType::kString:
      //writer_.String((std::string("\"") + token.stringConst + "\"").c_str());
      writer_.String(token.stringConst.c_str());
      break;
    }
  }
  else
    writer_.String(token.token.c_str());
}

//-------------------------------------------------------------------------------------------------
void Parser::ParseCustomMacro(Token & token, const std::string& macroName)
{
  writer_.StartObject();
  writer_.String("type");
  writer_.String("macro");
  writer_.String("name");
  writer_.String(macroName.c_str());
  writer_.String("line");
  writer_.Uint(token.startLine);

  WriteCurrentAccessControlType();

  ParseMacroMeta();

  writer_.EndObject();
}
