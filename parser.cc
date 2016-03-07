#include <iostream>
#include <vector>
#include <algorithm>
#include "parser.h"
#include "token.h"

//-------------------------------------------------------------------------------------------------
// Class used to write a typenode structure to json
//-------------------------------------------------------------------------------------------------
class TypeNodeWriter : public TypeNodeVisitor
{
public:
  TypeNodeWriter(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) :
    writer_(writer) {}

  //-------------------------------------------------------------------------------------------------
  virtual void Visit(FunctionNode& node) override
  {
    writer_.String("type");
    writer_.String("function");

    writer_.String("returnType");
    VisitNode(*node.returns);

    writer_.String("arguments");
    writer_.StartArray();
    for (auto& arg : node.arguments)
      VisitNode(*arg);
    writer_.EndArray();
  }

  //-------------------------------------------------------------------------------------------------
  virtual void Visit(LReferenceNode& node) override
  {
    writer_.String("type");
    writer_.String("lreference");

    writer_.String("baseType");
    VisitNode(*node.base);
  }

  //-------------------------------------------------------------------------------------------------
  virtual void Visit(LiteralNode& node) override
  {
    writer_.String("type");
    writer_.String("literal");

    writer_.String("name");
    writer_.String(node.name.c_str());
  }

  //-------------------------------------------------------------------------------------------------
  virtual void Visit(PointerNode& node) override
  {
    writer_.String("type");
    writer_.String("pointer");

    writer_.String("baseType");
    VisitNode(*node.base);
  }

  //-------------------------------------------------------------------------------------------------
  virtual void Visit(ReferenceNode& node) override
  {
    writer_.String("type");
    writer_.String("reference");

    writer_.String("baseType");
    VisitNode(*node.base);
  }

  //-------------------------------------------------------------------------------------------------
  virtual void Visit(TemplateNode& node) override
  {
    writer_.String("type");
    writer_.String("template");

    writer_.String("name");
    writer_.String(node.name.c_str());

    writer_.String("arguments");
    writer_.StartArray();
    for (auto& arg : node.arguments)
      VisitNode(*arg);
    writer_.EndArray();
  }

  //-------------------------------------------------------------------------------------------------
  virtual void VisitNode(TypeNode &node) override
  {
    writer_.StartObject();
    if (node.isConst)
    {
      writer_.String("const");
      writer_.Bool(true);
    }
    if (node.isMutable)
    {
      writer_.String("mutable");
      writer_.Bool(true);
    }
    if (node.isVolatile)
    {
      writer_.String("volatile");
      writer_.Bool(true);
    }
    TypeNodeVisitor::VisitNode(node);
    writer_.EndObject();
  }

private:
  rapidjson::PrettyWriter<rapidjson::StringBuffer> &writer_;
};

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
  else if ((customMacroIt = std::find(options_.functionNameMacro.begin(), options_.functionNameMacro.end(), token.token)) != options_.functionNameMacro.end())
    ParseFunction(token, *customMacroIt);
  else if(token.token == options_.propertyNameMacro)
    ParseProperty(token);
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
  writer_.Uint((unsigned)startToken.startLine);

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
      std::string value;
      while (GetToken(token) && (token.tokenType != TokenType::kSymbol || (token.token != "," && token.token != "}")))
      {
        value += token.token;
      }
      UngetToken(token);
  
      writer_.String("value");
      writer_.String(value.c_str());
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
  writer_.Uint((unsigned)token.startLine);

  WriteCurrentAccessControlType();
  ParseComment();
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
      ParseType();

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

//-------------------------------------------------------------------------------------------------
void Parser::ParseProperty(Token &token)
{
  writer_.StartObject();
  writer_.String("type");
  writer_.String("property");
  writer_.String("line");
  writer_.Uint((unsigned) token.startLine);

  ParseMacroMeta();
  WriteCurrentAccessControlType();

  // Check mutable
  bool isMutable = MatchIdentifier("mutable");
  if(isMutable)
  {
    writer_.String("mutable");
    writer_.Bool(true);
  }

  // Parse the type
  writer_.String("dataType");
  ParseType();

  // Parse the name
  Token nameToken;
  if(!GetIdentifier(nameToken))
    throw; // Expected a property name

  writer_.String("name");
  writer_.String(nameToken.token.c_str());

  writer_.EndObject();

  // Skip until the end of the definition
  Token t;
  while(GetToken(t))
    if(t.token == ";")
      break;
}

//--------------------------------------------------------------------------------------------------
void Parser::ParseFunction(Token &token, const std::string& macroName)
{
  writer_.StartObject();
  writer_.String("type");
  writer_.String("function");
  writer_.String("macro");
  writer_.String(macroName.c_str());
  writer_.String("line");
  writer_.Uint((unsigned) token.startLine);

  ParseComment();


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

//-------------------------------------------------------------------------------------------------
void Parser::ParseComment()
{
  std::string comment = lastComment_.endLine == cursorLine_ ? lastComment_.text : "";
  if (!comment.empty())
  {
    writer_.String("comment");
    writer_.String(comment.c_str());
  }
}

//--------------------------------------------------------------------------------------------------
void Parser::ParseType()
{
  std::unique_ptr<TypeNode> node = ParseTypeNode();
  TypeNodeWriter writer(writer_);
  writer.VisitNode(*node);
}

//-------------------------------------------------------------------------------------------------
std::unique_ptr<TypeNode> Parser::ParseTypeNode()
{
  std::unique_ptr<TypeNode> node;
  Token token;

  bool isConst = false, isVolatile = false, isMutable = false;
  for (bool matched = true; matched;)
  {
    matched = (!isConst && (isConst = MatchIdentifier("const"))) ||
      (!isVolatile && (isVolatile = MatchIdentifier("volatile"))) ||
      (!isMutable && (isMutable = MatchIdentifier("mutable")));
  }

  // Parse a literal value
  std::string declarator = ParseTypeNodeDeclarator();

  // Postfix const specifier
  isConst |= MatchIdentifier("const");

  // Template?
  if (MatchSymbol("<"))
  {
    std::unique_ptr<TemplateNode> templateNode(new TemplateNode(declarator));
    do
    {
      templateNode->arguments.emplace_back(ParseTypeNode());      
    } while (MatchSymbol(","));

    if (!MatchSymbol(">"))
      throw; // Expected >

    node.reset(templateNode.release());
  }
  else
  {
    node.reset(new LiteralNode(declarator));
  }

  // Store gathered stuff
  node->isConst = isConst;

  // Check reference or pointer types
  while (GetToken(token))
  {
    if (token.token == "&")
      node.reset(new ReferenceNode(std::move(node)));
    else if (token.token == "&&")
      node.reset(new LReferenceNode(std::move(node)));
    else if (token.token == "*")
      node.reset(new PointerNode(std::move(node)));
    else
    {
      UngetToken(token);
      break;
    }

    if (MatchIdentifier("const"))
      node->isConst = true;
  }

  // Function pointer?
  if (MatchSymbol("("))
  {
    // Parse void(*)(args, ...)
    //            ^
    //            |
    if (MatchSymbol("*"))
    {
      Token token;
      GetToken(token);
      if (token.token != ")" || (token.tokenType != TokenType::kIdentifier && !MatchSymbol(")")))
        throw;
    }

    // Parse arguments
    std::unique_ptr<FunctionNode> funcNode(new FunctionNode());
    funcNode->returns = std::move(node);

    if (!MatchSymbol(")"))
    {
      do
      {
        funcNode->arguments.emplace_back(ParseTypeNode());
      } while (MatchSymbol(","));
      if (!MatchSymbol(")"))
        throw;
    }

    node = std::move(funcNode);
  }

  // This stuff refers to the top node
  node->isVolatile = isVolatile;
  node->isMutable = isMutable;

  return std::move(node);
}

//-------------------------------------------------------------------------------------------------
std::string Parser::ParseTypeNodeDeclarator()
{
  // Skip optional forward declaration specifier
  MatchIdentifier("class");
  MatchIdentifier("struct");
  MatchIdentifier("typename");

  // Parse a type name 
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

  return declarator;
}

//-------------------------------------------------------------------------------------------------
std::string Parser::ParseTypename()
{
  return "";
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
  writer_.Uint((unsigned) token.startLine);

  WriteCurrentAccessControlType();

  ParseMacroMeta();

  writer_.EndObject();
}
