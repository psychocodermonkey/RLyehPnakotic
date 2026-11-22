#include "PnakoticBuildVersion.h"

#include <cstdint>
#include <iostream>

int main()
{
  using namespace RLyeh::Pnakotic;

  constexpr auto parsed = ParsePublicVersion(Build::Version.version.view());
  static_assert(parsed.valid());

  // Apple bundle versions allow three nonnegative numeric components. This
  // platform-only value is derived from the authoritative encoded version.
  constexpr std::uint64_t block = parsed.encoded_block;
  constexpr unsigned major = 1u + static_cast<unsigned>(block % 9999u);
  constexpr unsigned minor = static_cast<unsigned>((block / 9999u) % 100u);
  constexpr unsigned patch = static_cast<unsigned>((block / 999900u) % 100u);
  std::cout << major << '.' << minor << '.' << patch;
}
