//
// Created by Bas Zalmstra on 19/05/15.
//

#pragma once

#include "test.1.h"

R_ENUM()
enum Numbers
{
  Zero,
  One,
  Two,
  Three =0,
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

template<typename T, std::enable_if<has reflection>>
void Serialize(Stream, const T& obj)
{
  // Use reflection

}

template<>
void Serialize<Matrix>(stream, const Matrix &obj)
{
  // Custom code
}