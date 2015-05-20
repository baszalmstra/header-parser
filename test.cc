#include "parser.h"
#include "handler.h"
#include <iostream>

#include <fstream>
#include <sstream>

const char testContent[] = "#include <iostream>\nint main() { return 0; }";

// ----------------------------------------------------------------------------------------------------

class MyHandler : public Handler
{
  void Class(const ClassInfo& info)
  {
    std::cout << "Class:" << std::endl;

    std::cout << "  Fields:" << std::endl;
    for(FieldInfo f : info.fields)
      std::cout << "    " << f.type << " " << f.name << std::endl;

    for(FunctionInfo f : info.methods)
      std::cout << "    " << f.name << std::endl;
  }

  void Struct(const StructInfo& info)
  {
    std::cout << "Struct:" << std::endl;

    std::cout << "  Fields:" << std::endl;
    for(FieldInfo f : info.fields)
      std::cout << "    " << f.type << " " << f.name << std::endl;

    for(FunctionInfo f : info.methods)
      std::cout << "    " << f.name << std::endl;
  }

  void Function(const FunctionInfo& info)
  {
    std::cout << "Function: " << info.name << std::endl;
  }

  void Enum(const EnumInfo& info)
  {
    std::cout << "Enum:" << std::endl;
    for(std::string e : info.enumerators)
      std::cout << "  " << e << std::endl;
  }
};

// ----------------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{
  std::stringstream buffer;

  // Store content from file or test string
  if (argc > 1)
  {
    // Open from file
    std::ifstream t(argv[1]);
    buffer << t.rdbuf();
  }
  else
    buffer << testContent;

  Parser parser;
  MyHandler handler;
//  parser.Parse(buffer.str().c_str(), handler);
  parser.Parse(buffer.str().c_str());
	return 0;
}
