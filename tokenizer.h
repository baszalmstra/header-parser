#pragma once

#include <cstdint>
#include <cstdlib>

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

protected:
  /// Returns the next character. This methods skips block comments. If inLiteral is false, comments are not parsed.
  char GetChar(bool inLiteral = true);

  /// Returns the next character from the stream without modifying the cursor position.
  char peek() const;

protected:
  /// The input
  const char *input_;

  /// The length of the input
  std::size_t inputLength_;

  /// Current position in the input
  std::size_t cursorPos_;

  /// Current line of the cursor
  std::size_t cursorLine_;
};