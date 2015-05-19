#include "parser.h"
#include <iostream>

#include <fstream>
#include <sstream>

const char testContent[] = "#include <iostream>\nint main() { return 0; }";

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
  parser.Parse(buffer.str().c_str());

	return 0;
}
