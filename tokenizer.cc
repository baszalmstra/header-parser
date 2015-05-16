#include "tokenizer.h"
#include <string>

//-------------------------------------------------------------------------------------------------
Tokenizer::Tokenizer() :
  input_(nullptr),
  inputLength_(0),
  cursorPos_(0),
  cursorLine_(0)
{

}

//-------------------------------------------------------------------------------------------------
Tokenizer::~Tokenizer()
{

}

//-------------------------------------------------------------------------------------------------
void Tokenizer::Reset(const char* input, std::size_t startingLine)
{
  input_ = input;
  inputLength_ = std::char_traits<char>::length(input);
  cursorPos_ = 0;
  cursorLine_ = 0;
}

//-------------------------------------------------------------------------------------------------
char Tokenizer::GetChar(bool inLiteral)
{
  uint32_t commentCount = 0;
  char c = 0;

  while (true)
  {
    c = input_[cursorPos_];

    // Move cursor to new line if this is a new line
    if (c == '\n')
      ++cursorLine_;

    if (!inLiteral)
    {
      const char nextChar = peek();

      // Entering a comment
      if (c == '/' && nextChar == '*')
      {
        commentCount++;
        cursorPos_++;
        continue;
      }
      // Leaving a comment block
      else if (c == '*' && nextChar == '/' && commentCount > 0)
      {
        commentCount--;
        cursorPos_++;
        continue;
      }
    }

    // If this is a comment character; skip it
    if (commentCount > 0)
    {
      if (c == 0)
        break;

      continue;
    }
      
    // Character wasn't caught, so done
    break;
  }

  return c;
}

//-------------------------------------------------------------------------------------------------
char Tokenizer::peek() const
{
  return (cursorPos_ < inputLength_) ? input_[cursorPos_] : 0;
}