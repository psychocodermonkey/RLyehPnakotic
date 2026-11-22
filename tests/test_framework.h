// SPDX-FileCopyrightText: 2026 A.D. (PsychoCoderMonkey) <andrew.dixon@rlyeh.dev>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstdio>
#include <string_view>
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
    : passed_(passed)
  {
    if (!passed)
      Fail(file, line, expression);
  }

  Expectation& operator<<(std::string_view context)
  {
    if (!passed_)
      std::fprintf(stderr, "  case: %.*s\n", static_cast<int>(context.size()), context.data());
    return *this;
  }

  template<typename T>
  Expectation& operator<<(const T&)
  {
    return *this;
  }

private:
  bool passed_;
};

inline void ReportCommitSample(std::string_view date, std::string_view hash, bool passed)
{
  std::printf("    [ %s ] [ CASE ] %.*s %.*s\n", passed ? "PASS" : "FAIL", static_cast<int>(date.size()), date.data(),
              static_cast<int>(hash.size()), hash.data());
  std::fflush(stdout);
}

inline int RunAll()
{
  for (const TestCase& test : Tests())
  {
    const int failures_before = failures;
    test.function();
    std::printf("[ %s ] %s.%s\n", failures == failures_before ? "PASS" : "FAIL", test.suite, test.name);
    std::fflush(stdout);
  }

  if (failures == 0)
  {
    std::printf("All %zu registered R'Lyeh Pnakotic tests passed.\n", Tests().size());
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
