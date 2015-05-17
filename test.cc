#include "tokenizer.h"
#include "token.h"
#include <iostream>

const char testContent[] = "int main() { return 0; }";

int main()
{
	Tokenizer tokenizer;
  tokenizer.Reset(testContent);

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