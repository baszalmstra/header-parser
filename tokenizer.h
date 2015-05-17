#pragma once

#include <cstdint>
#include <cstdlib>

class Token;

class Tokenizer
{
public:
  Tokenizer();
  ~Tokenizer();

  // Do not allow copy or move
  Tokenizer(const Tokenizer& other) = delete;
  Tokenizer(Tokenizer &&other) = delete;

  /// Reset the parser with the given input text
  void Reset(const char* input, std::size_t startingLine = 1);

  /// Parses a token from the stream
  bool GetToken(Token& token);

protected:
  /**
   * @brief Returns the next character from the stream.
   * @details Returns the next character from the stream while advancing the cursor position.
   */
  char NextChar();

  /// Resets the cursor to the last read character
  void ResetChar();

  /// Returns the next character from the stream but skips comments and white spaces.
  char NextLeadingChar();

  /// Returns the next character from the stream without modifying the cursor position.
  char peek() const;

  /// Returns true if the stream is at the end
  bool is_eof() const;

protected:
  /// The input
  const char *input_;

  /// The length of the input
  std::size_t inputLength_;

  /// Current position in the input
  std::size_t cursorPos_;

  /// Current line of the cursor
  std::size_t cursorLine_;

  /// The cursor position of the last read character
  std::size_t prevCursorPos_;

  /// The cursor line of the the last read character
  std::size_t prevCursorLine_;
};