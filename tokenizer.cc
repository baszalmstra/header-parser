#include "tokenizer.h"
#include "token.h"
#include <string>
#include <cctype>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <cstdarg>

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
	prevCursorPos_ = cursorPos_;
  prevCursorLine_ = cursorLine_;

  if(is_eof())
	{
		++cursorPos_;	// Do continue so UngetChar does what you think it does
    return EndOfFileChar;
	}
	
  char c = input_[cursorPos_];

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
  if (!comment_.text.empty())
    lastComment_ = comment_;

  comment_.text = "";
  comment_.startLine = cursorLine_;
  comment_.endLine = cursorLine_;

  char c;
  for(c = GetChar(); c != EndOfFileChar; c = GetChar())
  {
    // If this is a whitespace character skip it
    std::char_traits<char>::int_type intc = std::char_traits<char>::to_int_type(c);

    // In case of a new line
    if (c == '\n')
    {
      if (!comment_.text.empty())
        comment_.text += "\n";
      continue;
    }

    if(std::isspace(intc) || std::iscntrl(intc))
      continue;

    // If this is a single line comment
    char next = peek();
    if(c == '/' && next == '/')
    {
      std::vector<std::string> lines;

      size_t indentationLastLine = 0;
      while (!is_eof() && c == '/' && next == '/')
      {
        // Search for the end of the line
        std::string line;
        for (c = GetChar();
          c != EndOfFileChar && c != '\n';
          c = GetChar())
        {
          line += c;
        }
        
        // Store the line
        size_t lastSlashIndex = line.find_first_not_of("/");
        if (lastSlashIndex == std::string::npos)
          line = "";
        else
          line = line.substr(lastSlashIndex);

        size_t firstCharIndex = line.find_first_not_of(" \t");
        if (firstCharIndex == std::string::npos)
          line = "";
        else
          line = line.substr(firstCharIndex);

        if (firstCharIndex > indentationLastLine && !lines.empty())
          lines.back() += std::string(" ") + line;
        else
        {
          lines.emplace_back(std::move(line));
          indentationLastLine = firstCharIndex;
        }

        // Check the next line
        while (!is_eof() && std::isspace(c = GetChar()));

        if (!is_eof())
          next = peek();
      }

      // Unget previously get char
      if (!is_eof())
        UngetChar();

      // Build comment string
      std::stringstream ss;
      for (size_t i = 0; i < lines.size(); ++i)
      {
        if (i > 0)
          ss << "\n";
        ss << lines[i];
      }

      comment_.text = ss.str();
      comment_.endLine = cursorLine_;

      // Go to the next
      continue;
    }

    // If this is a block comment
    if(c == '/' && next == '*')
    {
      // Search for the end of the block comment
      std::vector<std::string> lines;
      std::string line;
      for (c = GetChar(), next = peek();
        c != EndOfFileChar && (c != '*' || next != '/');
        c = GetChar(), next = peek())
      {
        if (c == '\n')
        {
          if (!lines.empty() || !line.empty())
            lines.emplace_back(line);
          line.clear();
        }
        else
        {
          if (!line.empty() || !(std::isspace(c) || c == '*'))
            line += c;
        }
      }

      // Skip past the slash
      if(c != EndOfFileChar)
        GetChar();

      // Skip past new lines and spaces
      while (!is_eof() && std::isspace(c = GetChar()));
      if (!is_eof())
        UngetChar();

      // Remove empty lines from the back
      while (!lines.empty() && lines.back().empty())
        lines.pop_back();

      // Build comment string
      std::stringstream ss;
      for (size_t i = 0; i < lines.size(); ++i)
      {
        if (i > 0)
          ss << "\n";
        ss << lines[i]; 
      }

      comment_.text = ss.str();
      comment_.endLine = cursorLine_;

      // Move to the next character
      continue;
    }

    break;
  }
  
  return c;
}

//--------------------------------------------------------------------------------------------------
bool Tokenizer::GetToken(Token &token, bool angleBracketsForStrings, bool seperateBraces)
{
  // Get the next character
  char c = GetLeadingChar();
  char p = peek();
  std::char_traits<char>::int_type intc = std::char_traits<char>::to_int_type(c);
  std::char_traits<char>::int_type intp = std::char_traits<char>::to_int_type(p);

  if(c == EndOfFileChar)
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
       (!seperateBraces && PAIR('>', '>')) ||
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
bool Tokenizer::GetConst(Token &token)
{
	if (!GetToken(token))
		return false;

	if (token.tokenType == TokenType::kConst)
		return true;

	UngetToken(token);
	return false;
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
  if(GetToken(token, false, std::char_traits<char>::length(symbol) == 1 && symbol[0] == '>'))
  {
    if(token.tokenType == TokenType::kSymbol && token.token == symbol)
      return true;

    UngetToken(token);
  }

  return false;
}

//--------------------------------------------------------------------------------------------------
bool Tokenizer::RequireIdentifier(const char *identifier)
{
  if(!MatchIdentifier(identifier))
    return Error("Missing identifier %s", identifier);
  return true;
}

//--------------------------------------------------------------------------------------------------
bool Tokenizer::RequireSymbol(const char *symbol)
{
  if (!MatchSymbol(symbol))
    return Error("Missing symbol %s", symbol);
  return true;
}

//-------------------------------------------------------------------------------------------------
bool Tokenizer::Error(const char* fmt, ...)
{
  char buffer[512];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, 512, fmt, args);
  va_end(args);
  printf("ERROR: %d:%d: %s", static_cast<int>(cursorLine_), 0, buffer);
  hasError_ = true;
  return false;
}