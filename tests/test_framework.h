// SPDX-FileCopyrightText: 2026 A.D. (PsychoCoderMonkey) <andrew.dixon@rlyeh.dev>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstdio>
#include <vector>

namespace TestFramework {

using TestFunction = void (*)();

struct TestCase
{
  const char* suite;
  const char* name;
  TestFunction function;
};

inline std::vector<TestCase>& Tests()
{
  static std::vector<TestCase> tests;
  return tests;
}

struct Registrar
{
  Registrar(const char* suite, const char* name, TestFunction function)
  {
    Tests().push_back({suite, name, function});
  }
};

inline int failures = 0;

inline void Fail(const char* file, int line, const char* expression)
{
  std::fprintf(stderr, "%s:%d: check failed: %s\n", file, line, expression);
  failures++;
}

class Expectation
{
public:
  Expectation(bool passed, const char* file, int line, const char* expression)
  {
    if (!passed)
      Fail(file, line, expression);
  }

  template<typename T>
  Expectation& operator<<(const T&)
  {
    return *this;
  }
};

inline int RunAll()
{
  for (const TestCase& test : Tests())
    test.function();

  if (failures == 0)
  {
    std::printf("All %zu R'Lyeh Pnakotic tests passed.\n", Tests().size());
    return 0;
  }

  std::fprintf(stderr, "%d R'Lyeh Pnakotic checks failed.\n", failures);
  return 1;
}

} // namespace TestFramework

#define TEST(suite, name)                                                                                             \
  static void suite##_##name();                                                                                       \
  static const TestFramework::Registrar suite##_##name##_registrar(#suite, #name, &suite##_##name);                 \
  static void suite##_##name()

#define EXPECT_EQ(left, right)                                                                                        \
  TestFramework::Expectation(((left) == (right)), __FILE__, __LINE__, #left " == " #right)
#define EXPECT_TRUE(expression)                                                                                       \
  TestFramework::Expectation(static_cast<bool>(expression), __FILE__, __LINE__, #expression)
#define EXPECT_FALSE(expression)                                                                                      \
  TestFramework::Expectation(!static_cast<bool>(expression), __FILE__, __LINE__, "!(" #expression ")")
#define ASSERT_TRUE(expression)                                                                                       \
  do                                                                                                                 \
  {                                                                                                                  \
    if (!static_cast<bool>(expression))                                                                              \
    {                                                                                                                \
      TestFramework::Fail(__FILE__, __LINE__, #expression);                                                          \
      return;                                                                                                        \
    }                                                                                                                \
  } while (false)
