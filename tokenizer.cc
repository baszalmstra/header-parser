#include "tokenizer.h"
#include "token.h"
#include <string>

namespace {
  static const char EndOfFileChar = std::char_traits<char>::to_char_type(std::char_traits<char>::eof());
}

//--------------------------------------------------------------------------------------------------
Tokenizer::Tokenizer() :
  input_(nullptr),
  inputLength_(0),
  cursorPos_(0),
  cursorLine_(0)
{

}

//--------------------------------------------------------------------------------------------------
Tokenizer::~Tokenizer()
{

}

//--------------------------------------------------------------------------------------------------
void Tokenizer::Reset(const char* input, std::size_t startingLine)
{
  input_ = input;
  inputLength_ = std::char_traits<char>::length(input);
  cursorPos_ = 0;
  cursorLine_ = startingLine;
}

//--------------------------------------------------------------------------------------------------
char Tokenizer::NextChar()
{
  char c = input_[cursorPos_];

  prevCursorPos_ = cursorPos_;
  prevCursorLine_ = cursorLine_;

  // New line moves the cursor to the new line
  if(c == '\n')
    cursorLine_++;

  cursorPos_++;
  return c;
}

//--------------------------------------------------------------------------------------------------
void Tokenizer::ResetChar()
{
  cursorLine_ = prevCursorLine_;
  cursorPos_ = prevCursorPos_;
}

//--------------------------------------------------------------------------------------------------
char Tokenizer::peek() const
{
  return !is_eof() ?
            input_[cursorPos_] :
            EndOfFileChar;
}

//--------------------------------------------------------------------------------------------------
char Tokenizer::NextLeadingChar()
{
  char c;
  for(c = NextChar(); !is_eof(); c = NextChar())
  {
    // If this is a whitespace character skip it
    std::char_traits<char>::int_type intc = std::char_traits<char>::to_int_type(c);
    if(std::isspace(intc) || std::iscntrl(intc))
      continue;

    // If this is a single line comment
    char next = peek();
    if(c == '/' && next == '/')
    {
      // Search for the end of the line
      for (c = NextChar();
           c != EndOfFileChar && c != '\n';
           c = NextChar());

      // Go to the next
      continue;
    }

    // If this is a block comment
    if(c == '/' && next == '*')
    {
      // Search for the end of the block comment
      for (c = NextChar(), next = peek();
           c != EndOfFileChar && (c != '*' || next != '/');
           c = NextChar(), next = peek());

      // Skip past the slash
      if(c != EndOfFileChar)
        NextChar();

      // Move to the next character
      continue;
    }

    break;
  }

  return c;
}

//--------------------------------------------------------------------------------------------------
bool Tokenizer::GetToken(Token &token)
{
  // Get the next character
  char c = NextLeadingChar();
  std::char_traits<char>::int_type intc = std::char_traits<char>::to_int_type(c);

  if(!std::char_traits<char>::not_eof(intc))
  {
    ResetChar();
    return false;
  }

  // Record the start of the token position
  token.startPos = prevCursorPos_;
  token.startLine = prevCursorLine_;
  token.token.clear();
  token.tokenType = TokenType::kNone;

  // Alphanumeric token
  if(std::isalpha(intc) || c == '_')
  {
    // Read the rest of the alphanumeric characters
    do
    {
      token.token.push_back(c);
      c = NextChar();
      intc = std::char_traits<char>::to_int_type(c);
    } while(std::isalnum(intc) || c == '_');

    // Put back the last read character since it's not part of the identifier
    ResetChar();

    // Set the type of the token
    token.tokenType = TokenType::kIdentifier;

    return true;
  }
  // Symbol
  else
  {
    // Push back the symbol
    token.token.push_back(c);

    #define PAIR(cc,dd) (c==cc&&d==dd) /* Comparison macro for two characters */
    const char d = NextChar();
    if(PAIR('<', '<') ||
       PAIR('-', '>') ||
       PAIR('>', '>') ||
       PAIR('!', '=') ||
       PAIR('<', '=') ||
       PAIR('>', '=') ||
       PAIR('+', '+') ||
       PAIR('-', '-') ||
       PAIR('+', '=') ||
       PAIR('-', '=') ||
       PAIR('*', '=') ||
       PAIR('/', '=') ||
       PAIR('^', '=') ||
       PAIR('|', '=') ||
       PAIR('&', '=') ||
       PAIR('~', '=') ||
       PAIR('%', '=') ||
       PAIR('&', '&') ||
       PAIR('|', '|') ||
       PAIR('=', '=') ||
       PAIR(':', ':')
      )
    #undef PAIR
    {
      token.token.push_back(d);
    }
    else
      ResetChar();

    token.tokenType = TokenType::kSymbol;

    return true;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------
bool Tokenizer::is_eof() const
{
  return cursorPos_ >= inputLength_;
}
