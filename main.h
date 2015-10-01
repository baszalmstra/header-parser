//
// Created by Bas Zalmstra on 19/05/15.
//

#pragma once

#include "main.h"

R_ENUM()
enum Numbers
{
  Zero,
  One,
  Two,
  Three =0,
};

R_CLASS()
class HenkClass : Foo, public Bar
{
public:
  R_CLASS()
  class SubClass
  {

  };

  class AnonymousClass
  {

  };

  R_FUNCTION()
  inline virtual String const & member_method(const String& name, bool enable = true) const = 0;

  R_ENUM()
  enum Numbers
  {
    Zero,
    One = 1,
    Two,
    Three =0,
  };
};

enum IgnoredNumbers
{
  Four = 4,
  Five,
};

namespace test {

  R_ENUM()
  enum class CXX11Numbers : uint8_t
  {
    Six,
    Seven,
    Eight,
    Nine,
    Ten = 10
  };

}