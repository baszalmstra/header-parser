#include "parser.h"
#include "handler.h"
#include <iostream>

#include <fstream>
#include <sstream>

const char testContent[] = "#include <iostream>\nint main() { return 0; }";

//----------------------------------------------------------------------------------------------------
void print_usage()
{
  std::cout << "Usage: inputFile" << std::endl;
}

//----------------------------------------------------------------------------------------------------
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
  {
    print_usage();
    return -1;
  }

  Parser parser;
  if (parser.Parse(buffer.str().c_str()))
    std::cout << parser.result() << std::endl;
  
  {
    std::ofstream output("result.json");
    output << parser.result();
  }
	return 0;
}
