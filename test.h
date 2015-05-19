//
// Created by Bas Zalmstra on 19/05/15.
//

#pragma once

#include "test.1.h"

R_ENUM()
enum Numbers
{
  Zero = 0,
  One,
  Two,
  Three = 3
};

enum IgnoredNumbers
{
  Four = 4,
  Five,
};

R_ENUM()
enum class CXX11Numbers : uint8_t
{
  Six,
  Seven,
  Eight,
  Nine,
  Ten = 10
};