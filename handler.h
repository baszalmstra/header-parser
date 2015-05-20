#pragma once

#include <string>
#include <vector>

// ----------------------------------------------------------------------------------------------------

enum AccessSpecifierType
{
    PUBLIC,
    PROTECTED,
    PRIVATE
};

// ----------------------------------------------------------------------------------------------------

typedef std::string Type;

// ----------------------------------------------------------------------------------------------------

struct FieldInfo
{
    std::string name;
    Type type;
    AccessSpecifierType access_specifier;
};

// ----------------------------------------------------------------------------------------------------

struct ArgumentInfo
{
    std::string name;
    Type type;
};

// ----------------------------------------------------------------------------------------------------

struct FunctionInfo
{
    std::string name;
    Type return_type;
    std::vector<ArgumentInfo> arguments;
    bool is_const;
};

// ----------------------------------------------------------------------------------------------------

struct EnumInfo
{
    std::string name;
    std::vector<std::string> enumerators;
};

// ----------------------------------------------------------------------------------------------------

struct ClassInfo
{
    std::string name;
    std::vector<FunctionInfo> methods;
    std::vector<FieldInfo> fields;
};

// ----------------------------------------------------------------------------------------------------

struct StructInfo
{
    std::string name;
    std::vector<FunctionInfo> methods;
    std::vector<FieldInfo> fields;
};

// ----------------------------------------------------------------------------------------------------

class Handler
{

public:

    virtual void Class(const ClassInfo& info) {}
    virtual void Struct(const StructInfo& info) {}
    virtual void Function(const FunctionInfo& info) {}
    virtual void Enum(const EnumInfo& info) {}
};
