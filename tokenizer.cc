#include "tokenizer.h"
#include "token.h"
#include <string>
#include <cctype>
#include <stdexcept>

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
char Tokenizer::GetChar()
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
void Tokenizer::UngetChar()
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
char Tokenizer::GetLeadingChar()
{
  char c;
  for(c = GetChar(); !is_eof(); c = GetChar())
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
      for (c = GetChar();
           c != EndOfFileChar && c != '\n';
           c = GetChar());

      // Go to the next
      continue;
    }

    // If this is a block comment
    if(c == '/' && next == '*')
    {
      // Search for the end of the block comment
      for (c = GetChar(), next = peek();
           c != EndOfFileChar && (c != '*' || next != '/');
           c = GetChar(), next = peek());

      // Skip past the slash
      if(c != EndOfFileChar)
        GetChar();

      // Move to the next character
      continue;
    }

    break;
  }

  return c;
}

//--------------------------------------------------------------------------------------------------
bool Tokenizer::GetToken(Token &token, bool angleBracketsForStrings)
{
  // Get the next character
  char c = GetLeadingChar();
  char p = peek();
  std::char_traits<char>::int_type intc = std::char_traits<char>::to_int_type(c);
  std::char_traits<char>::int_type intp = std::char_traits<char>::to_int_type(p);

  if(!std::char_traits<char>::not_eof(intc))
  {
    UngetChar();
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
      c = GetChar();
      intc = std::char_traits<char>::to_int_type(c);
    } while(std::isalnum(intc) || c == '_');

    // Put back the last read character since it's not part of the identifier
    UngetChar();

    // Set the type of the token
    token.tokenType = TokenType::kIdentifier;

    if(token.token == "true")
    {
      token.tokenType = TokenType::kConst;
      token.constType = ConstType::kBoolean;
      token.boolConst = true;
    }
    else if(token.token == "false")
    {
      token.tokenType = TokenType::kConst;
      token.constType = ConstType::kBoolean;
      token.boolConst = false;
    }

    return true;
  }
  // Constant
  else if(std::isdigit(intc) || ((c == '-' || c == '+') && std::isdigit(intp)))
  {
    bool isFloat = false;
    bool isHex = false;
    bool isNegated = c == '-';
    do
    {
      if(c == '.')
        isFloat = true;

      if(c == 'x' || c == 'X')
        isHex = true;

      token.token.push_back(c);
      c = GetChar();
      intc = std::char_traits<char>::to_int_type(c);

    } while(std::isdigit(intc) ||
        (!isFloat && c == '.') ||
        (!isHex && (c == 'X' || c == 'x')) ||
        (isHex && std::isxdigit(intc)));

    if(!isFloat || (c != 'f' && c != 'F'))
      UngetChar();

    token.tokenType = TokenType::kConst;
    if(!isFloat)
    {
      try
      {
        if(isNegated)
        {
          token.int32Const = std::stoi(token.token, 0, 0);
          token.constType = ConstType::kInt32;
        }
        else
        {
          token.uint32Const = std::stoul(token.token, 0, 0);
          token.constType = ConstType::kUInt32;
        }
      }
      catch(std::out_of_range)
      {
        if(isNegated)
        {
          token.int64Const = std::stoll(token.token, 0, 0);
          token.constType = ConstType::kInt64;
        }
        else
        {
          token.uint64Const = std::stoull(token.token, 0, 0);
          token.constType = ConstType::kUInt64;
        }
      }
    }
    else
    {
      token.realConst = std::stod(token.token);
      token.constType = ConstType::kReal;
    }

    return true;
  }
  else if (c == '"' || (angleBracketsForStrings && c == '<'))
  {
    const char closingElement = c == '"' ? '"' : '>';

    c = GetChar();
    while (c != closingElement && std::char_traits<char>::not_eof(std::char_traits<char>::to_int_type(c)))
    {
      if(c == '\\')
      {
        c = GetChar();
        if(!std::char_traits<char>::not_eof(std::char_traits<char>::to_int_type(c)))
          break;
        else if(c == 'n')
          c = '\n';
        else if(c == 't')
          c = '\t';
        else if(c == 'r')
          c = '\r';
        else if(c == '"')
          c = '"';
      }

      token.token.push_back(c);
      c = GetChar();
    }

    if (c != closingElement)
      UngetChar();

    token.tokenType = TokenType::kConst;
    token.constType = ConstType::kString;
    token.stringConst = token.token;

    return true;
  }
  // Symbol
  else
  {
    // Push back the symbol
    token.token.push_back(c);

    #define PAIR(cc,dd) (c==cc&&d==dd) /* Comparison macro for two characters */
    const char d = GetChar();
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
      UngetChar();

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

//--------------------------------------------------------------------------------------------------
bool Tokenizer::GetIdentifier(Token &token)
{
  if(!GetToken(token))
    return false;

  if(token.tokenType == TokenType::kIdentifier)
    return true;

  UngetToken(token);
  return false;
}

//--------------------------------------------------------------------------------------------------
void Tokenizer::UngetToken(const Token &token)
{
  cursorLine_ = token.startLine;
  cursorPos_ = token.startPos;
}

//--------------------------------------------------------------------------------------------------
bool Tokenizer::MatchIdentifier(const char *identifier)
{
  Token token;
  if(GetToken(token))
  {
    if(token.tokenType == TokenType::kIdentifier && token.token == identifier)
      return true;

    UngetToken(token);
  }

  return false;
}

//--------------------------------------------------------------------------------------------------
bool Tokenizer::MatchSymbol(const char *symbol)
{
  Token token;
  if(GetToken(token))
  {
    if(token.tokenType == TokenType::kSymbol && token.token == symbol)
      return true;

    UngetToken(token);
  }

  return false;
}

//--------------------------------------------------------------------------------------------------
void Tokenizer::RequireIdentifier(const char *identifier)
{
  if(!MatchIdentifier(identifier))
    throw;
}

//--------------------------------------------------------------------------------------------------
void Tokenizer::RequireSymbol(const char *symbol)
{
  if(!MatchSymbol(symbol))
    throw;
}
