#include "tokenizer.h"
#include "token.h"
#include <iostream>

#include <fstream>
#include <sstream>

const char testContent[] = "int main() { return 0; }";

int main(int argc, char** argv)
{
  Tokenizer tokenizer;

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

  // Give the tokenizer the text
  tokenizer.Reset(buffer.str().c_str());

  Token t;
  while(tokenizer.GetToken(t))
  {
    switch(t.tokenType)
    {
      case TokenType::kIdentifier:
        std::cout << "[Identi] " << t.token << std::endl;
        break;
      case TokenType::kSymbol:
        std::cout << "[Symbol] " << t.token << std::endl;
        break;
      case TokenType::kConst:
        std::cout << "[Const ] " << t.token << std::endl;
        break;
      default:
        break;
    }
  }

	return 0;
}
