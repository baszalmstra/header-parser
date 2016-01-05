#pragma once

#include <string>
#include <vector>

struct Options
{
  std::string classNameMacro;
  std::string functionNameMacro;
  std::string enumNameMacro;
  std::string propertyNameMacro;
  std::vector<std::string> customMacros;
};