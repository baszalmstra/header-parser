#include <iostream>
#include <vector>
#include <algorithm>
#include "parser.h"
#include "token.h"
#include <cstdarg>

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
    {
      writer_.StartObject();
      if (!arg->name.empty())
      {
        writer_.String("name");
        writer_.String(arg->name.c_str());
      }
      writer_.String("type");
      VisitNode(*arg->type);
      writer_.EndObject();
    }
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
  if(!HasError())
    writer_.EndArray();

  return !HasError();
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseStatement()
{
  Token token;
  if(!GetToken(token))
    return false;

  if (!ParseDeclaration(token))
    return false;

  return true;
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseDeclaration(Token &token)
{
  std::vector<std::string>::const_iterator customMacroIt;
  if(token.token == "#")
    return ParseDirective();
  else if(token.token == ";")
    return true; // Empty statement
  else if(token.token == options_.enumNameMacro)
    return ParseEnum(token);
  else if (token.token == options_.classNameMacro)
    return ParseClass(token);
  else if ((customMacroIt = std::find(options_.functionNameMacro.begin(), options_.functionNameMacro.end(), token.token)) != options_.functionNameMacro.end())
    return ParseFunction(token, *customMacroIt);
  else if(token.token == options_.propertyNameMacro)
    return ParseProperty(token);
  else if (token.token == "namespace")
    return ParseNamespace();
  else if (ParseAccessControl(token, topScope_->currentAccessControlType))
    return RequireSymbol(":");
  else if ((customMacroIt = std::find(options_.customMacros.begin(), options_.customMacros.end(), token.token)) != options_.customMacros.end())
    return ParseCustomMacro(token, *customMacroIt);
  else
    return SkipDeclaration(token);

  return true;
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseDirective()
{
  Token token;

  // Check the compiler directive
  if(!GetIdentifier(token))
    return Error("Missing compiler directive after #");

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

  return true;
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
bool Parser::ParseEnum(Token &startToken)
{
  writer_.StartObject();
  writer_.String("type");
  writer_.String("enum");
  writer_.String("line");
  writer_.Uint((unsigned)startToken.startLine);

  WriteCurrentAccessControlType();

  if (!ParseMacroMeta())
    return false;

  if (!RequireIdentifier("enum"))
    return false;

  // C++1x enum class type?
  bool isEnumClass = MatchIdentifier("class");

  // Parse enum name
  Token enumToken;
  if (!GetIdentifier(enumToken))
    return Error("Missing enum name");

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
    if (!GetIdentifier(baseToken))
      return Error("Missing enum type specifier after :");

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

  if (!RequireSymbol("}"))
    return false;
  writer_.EndArray();

  MatchSymbol(";");

  writer_.EndObject();

  return true;
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseMacroMeta()
{
  writer_.String("meta");

  if (!RequireSymbol("("))
    return false;
  if (!ParseMetaSequence())
    return false;

  // Possible ;
  MatchSymbol(";");
  
  return true;
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseMetaSequence()
{
  writer_.StartObject();

  if(!MatchSymbol(")"))
  {
    do
    {
      // Parse key value
      Token keyToken;
      if (!GetIdentifier(keyToken))
        return Error("Expected identifier in meta sequence");

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
      {
        if (!ParseMetaSequence())
          return false;
        // No value
      }
      else
        writer_.Null();
    } while (MatchSymbol(","));

    MatchSymbol(")");
  }

  writer_.EndObject();
  return true;
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
bool Parser::ParseNamespace()
{
  writer_.StartObject();
  writer_.String("type");
  writer_.String("namespace");

  Token token;
  if (!GetIdentifier(token))
    return Error("Missing namespace name");

  writer_.String("name");
  writer_.String(token.token.c_str());

  if (!RequireSymbol("{"))
    return false;

  writer_.String("members");
  writer_.StartArray();

  PushScope(token.token, ScopeType::kNamespace, AccessControlType::kPublic);

  while (!MatchSymbol("}"))
    if (!ParseStatement())
      return false;

  PopScope();

  writer_.EndArray();

  writer_.EndObject();
  return true;
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
bool Parser::ParseClass(Token &token)
{
  writer_.StartObject();
  writer_.String("type");
  writer_.String("class");
  writer_.String("line");
  writer_.Uint((unsigned)token.startLine);

  WriteCurrentAccessControlType();
  if (!ParseComment())
    return false;
  if (!ParseMacroMeta())
    return false;

  if(MatchIdentifier("template") && !ParseClassTemplate())
    return false;

  bool isStruct = MatchIdentifier("struct");
  if (!(MatchIdentifier("class") || isStruct))
    return Error("Missing identifier class or struct");

  writer_.String("isstruct");
  writer_.Bool(isStruct);

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
      if (!ParseType())
        return false;

      writer_.EndObject();
    }
    while (MatchSymbol(","));

    writer_.EndArray();
  }

  if (!RequireSymbol("{"))
    return false;

  writer_.String("members");
  writer_.StartArray();

  PushScope(classNameToken.token, ScopeType::kClass, isStruct ? AccessControlType::kPublic : AccessControlType::kPrivate);

  while (!MatchSymbol("}"))
    if (!ParseStatement())
      return false;

  PopScope();

  writer_.EndArray();

  if (!RequireSymbol(";"))
    return false;

  writer_.EndObject();
  return true;
}

//-------------------------------------------------------------------------------------------------
bool Parser::ParseProperty(Token &token)
{
  writer_.StartObject();
  writer_.String("type");
  writer_.String("property");
  writer_.String("line");
  writer_.Uint((unsigned) token.startLine);

  if (!ParseMacroMeta())
    return false;

  WriteCurrentAccessControlType();

  // Process method specifiers in any particular order
  bool isMutable = false, isStatic = false;
  for (bool matched = true; matched;)
  {
    matched = (!isMutable && (isMutable = MatchIdentifier("mutable"))) ||
      (!isStatic && (isStatic = MatchIdentifier("static")));
  }

  // Check mutable
  if(isMutable)
  {
    writer_.String("mutable");
    writer_.Bool(true);
  }

  // Check mutable
  if (isStatic)
  {
    writer_.String("static");
    writer_.Bool(true);
  }

  // Parse the type
  writer_.String("dataType");
  if (!ParseType())
    return false;

  // Parse the name
  Token nameToken;
  if(!GetIdentifier(nameToken))
    throw; // Expected a property name

  writer_.String("name");
  writer_.String(nameToken.token.c_str());

  // Parse array
  writer_.String("elements");
  if (MatchSymbol("["))
  {
	  Token arrayToken;
	  if(!GetConst(arrayToken))
		  if(!GetIdentifier(arrayToken))
			  throw; // Expected a property name
	  writer_.String(arrayToken.token.c_str());

	  if(!MatchSymbol("]"))
		  throw;
  }
  else
  {
	writer_.Null();
  }

  writer_.EndObject();


  // Skip until the end of the definition
  Token t;
  while(GetToken(t))
    if(t.token == ";")
      break;

  return true;
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseFunction(Token &token, const std::string& macroName)
{
  writer_.StartObject();
  writer_.String("type");
  writer_.String("function");
  writer_.String("macro");
  writer_.String(macroName.c_str());
  writer_.String("line");
  writer_.Uint((unsigned) token.startLine);

  if (!ParseComment())
    return false;
  
  if (!ParseMacroMeta())
    return false;

  WriteCurrentAccessControlType();

  // Process method specifiers in any particular order
  bool isVirtual = false, isInline = false, isConstExpr = false, isStatic = false;
  for(bool matched = true; matched;)
  {
    matched = (!isVirtual && (isVirtual = MatchIdentifier("virtual"))) ||
        (!isInline && (isInline = MatchIdentifier("inline"))) ||
        (!isConstExpr && (isConstExpr = MatchIdentifier("constexpr"))) ||
        (!isStatic && (isStatic = MatchIdentifier("static")));
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
  if (isStatic)
  {
    writer_.String("static");
    writer_.Bool(isStatic);
  }

  // Parse the return type
  writer_.String("returnType");
  if (!ParseType())
    return false;

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
      if (!ParseType())
        return false;
      
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
  if (!SkipDeclaration(skipToken))
    return false;
  return true;
}

//-------------------------------------------------------------------------------------------------
bool Parser::ParseComment()
{
  std::string comment = lastComment_.endLine == cursorLine_ ? lastComment_.text : "";
  if (!comment.empty())
  {
    writer_.String("comment");
    writer_.String(comment.c_str());
  }

  return true;
}

//--------------------------------------------------------------------------------------------------
bool Parser::ParseType()
{
  std::unique_ptr<TypeNode> node = ParseTypeNode();
  if (node == nullptr)
    return false;
  TypeNodeWriter writer(writer_);
  writer.VisitNode(*node);
  return true;
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
      auto node = ParseTypeNode();
      if (node == nullptr)
        return nullptr;
      templateNode->arguments.emplace_back(std::move(node));
    } while (MatchSymbol(","));

    if (!MatchSymbol(">"))
    {
      Error("Expected closing >");
      return nullptr;
    }

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
        std::unique_ptr<FunctionNode::Argument> argument(new FunctionNode::Argument);
        argument->type = ParseTypeNode();
        if (argument->type == nullptr)
          return nullptr;

        // Get , or name identifier
        if (!GetToken(token))
        {
          Error("Unexpected end of file");
          return nullptr;
        }

        // Parse optional name
        if (token.tokenType == TokenType::kIdentifier)
          argument->name = token.token;
        else
          UngetToken(token);          

        funcNode->arguments.emplace_back(std::move(argument));

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
bool Parser::ParseCustomMacro(Token & token, const std::string& macroName)
{
  writer_.StartObject();
  writer_.String("type");
  writer_.String("macro");
  writer_.String("name");
  writer_.String(macroName.c_str());
  writer_.String("line");
  writer_.Uint((unsigned) token.startLine);

  WriteCurrentAccessControlType();

  if (!ParseMacroMeta())
    return false;

  writer_.EndObject();
  return true;
}

//-------------------------------------------------------------------------------------------------
bool Parser::ParseClassTemplate()
{
  writer_.String("template");
  writer_.StartObject();

  if(!RequireSymbol("<"))
    return false;

  writer_.String("arguments");
  writer_.StartArray();

  do
  {
    if(!ParseClassTemplateArgument())
      return false;
  } while(MatchSymbol(","));

  writer_.EndArray();

  if(!RequireSymbol(">"))
    return false;
  writer_.EndObject();

  return true;
}

//-------------------------------------------------------------------------------------------------
bool Parser::ParseClassTemplateArgument()
{
  writer_.StartObject();

  Token token;
  if(!GetToken(token) || token.tokenType != TokenType::kIdentifier || !(token.token == "class" || token.token == "typename"))
  {
    Error("expected either 'class' or 'identifier' in template argument");
    return false;
  }

  writer_.String("typeParameterKey");
  writer_.String(token.token.c_str());

  // Parse the name
  GetToken(token);
  if(token.tokenType != TokenType::kIdentifier)
  {
    Error("expected identifier");
    return false;
  }

  writer_.String("name");
  writer_.String(token.token.c_str());

  // Optionally check if there is a default initializer
  if(MatchSymbol("="))
  {
    writer_.String("defaultType");
    if(!ParseType())
      return false;
  }

  writer_.EndObject();
  return true;
}
