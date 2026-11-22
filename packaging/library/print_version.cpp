// SPDX-FileCopyrightText: 2026 A.D. (PsychoCoderMonkey) <andrew.dixon@rlyeh.dev>
// SPDX-License-Identifier: GPL-3.0-only

#include "PnakoticBuildVersion.h"

#include <cstdio>

int main()
{
  std::printf("%s\n%s\n", RLyeh::Pnakotic::Build::Version.version.data(), RLyeh::Pnakotic::Build::CommitHash);
}
