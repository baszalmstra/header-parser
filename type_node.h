#pragma once

#include <vector>
#include <memory>

struct TypeNode
{
  enum class Type
  {
    kPointer,
    kReference,
    kLReference,
    kLiteral,
    kTemplate,
    kFunction
  };

  TypeNode(Type t) :
    type(t) {}

  bool isConst = false;
  bool isVolatile = false;
  bool isMutable = false;

  Type type;
};

struct PointerNode : public TypeNode
{
  PointerNode(std::unique_ptr<TypeNode> && b) :
    TypeNode(TypeNode::Type::kPointer),
    base(std::forward<std::unique_ptr<TypeNode>>(b)){}

  std::unique_ptr<TypeNode> base;
};

struct ReferenceNode : public TypeNode
{
  ReferenceNode(std::unique_ptr<TypeNode> && b) :
    TypeNode(TypeNode::Type::kReference),
    base(std::forward<std::unique_ptr<TypeNode>>(b)){}

  std::unique_ptr<TypeNode> base;
};

struct LReferenceNode : public TypeNode
{
  LReferenceNode(std::unique_ptr<TypeNode> && b) :
    TypeNode(TypeNode::Type::kLReference),
    base(std::forward<std::unique_ptr<TypeNode>>(b)){}

  std::unique_ptr<TypeNode> base;
};

struct TemplateNode : public TypeNode
{
  TemplateNode(const std::string& n) :
    TypeNode(TypeNode::Type::kTemplate),
    name(n) {}

  std::string name;
  std::vector<std::unique_ptr<TypeNode>> arguments;
};

struct LiteralNode : public TypeNode
{
  LiteralNode(const std::string& ref) :
    TypeNode(TypeNode::Type::kLiteral),
    name(ref) {}

  std::string name;
};

struct FunctionNode : public TypeNode
{
  FunctionNode() : TypeNode(TypeNode::Type::kFunction) {}

  struct Argument
  {
    std::string name;
    std::unique_ptr<TypeNode> type;
  };


  std::unique_ptr<TypeNode> returns;
  std::vector<std::unique_ptr<Argument>> arguments;
};

struct ITypeNodeVisitor
{
  virtual void VisitNode(TypeNode &node) = 0;
  virtual void Visit(PointerNode& node) = 0;
  virtual void Visit(ReferenceNode& node) = 0;
  virtual void Visit(LReferenceNode& node) = 0;
  virtual void Visit(LiteralNode& node) = 0;
  virtual void Visit(TemplateNode& node) = 0;
  virtual void Visit(FunctionNode& node) = 0;
};

struct TypeNodeVisitor : public ITypeNodeVisitor
{
  void VisitNode(TypeNode &node) override
  {
    switch (node.type)
    {
    case TypeNode::Type::kPointer:
      Visit(reinterpret_cast<PointerNode&>(node));
      break;
    case TypeNode::Type::kReference:
      Visit(reinterpret_cast<ReferenceNode&>(node));
      break;
    case TypeNode::Type::kLReference:
      Visit(reinterpret_cast<LReferenceNode&>(node));
      break;
    case TypeNode::Type::kLiteral:
      Visit(reinterpret_cast<LiteralNode&>(node));
      break;
    case TypeNode::Type::kTemplate:
      Visit(reinterpret_cast<TemplateNode&>(node));
      break;
    case TypeNode::Type::kFunction:
      Visit(reinterpret_cast<FunctionNode&>(node));
      break;
    }
  }
};
