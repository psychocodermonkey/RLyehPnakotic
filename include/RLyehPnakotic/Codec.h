// SPDX-FileCopyrightText: 2026 A.D. (PsychoCoderMonkey) <andrew.dixon@rlyeh.dev>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace RLyeh::Pnakotic {

// The codec is the project-independent mathematical layer. It maps an opaque
// 39-bit payload to a checked, permuted 40-bit block and then renders that
// block as X.Y.Z.W. Calendar and SCM meaning are added by Version.h.
//
// Every operation remains constexpr-capable so applications, build metadata,
// tests, and future tools all use this one authoritative implementation.

// R'Lyeh's recurring root value. It deliberately remains visible at each point
// where it participates in the permutation instead of being folded into opaque
// precomputed constants. WEYL and KNUTH are deterministic mixing constants,
// not claims of cryptographic construction or strength.
inline constexpr std::uint32_t ROOT = 1928;
inline constexpr std::uint32_t WEYL = 0x61C88647u;
inline constexpr std::uint32_t KNUTH = 0x9E3779B1u;
inline constexpr std::size_t FEISTEL_ROUNDS = 6;

// The logical payload occupies 39 bits. Even parity expands it to the 40-bit
// block required by the balanced 20/20 Feistel permutation.
inline constexpr std::uint64_t PAYLOAD_MASK = (std::uint64_t{1} << 39) - 1;
inline constexpr std::uint64_t BLOCK_MASK = (std::uint64_t{1} << 40) - 1;
inline constexpr std::uint32_t HALF_MASK = (std::uint32_t{1} << 20) - 1;

// Six rounds are the digital root of Euler's totient of the R'Lyeh root:
// phi(1928) = 960, and 9 + 6 + 0 = 15, 1 + 5 = 6.
static_assert(FEISTEL_ROUNDS == 6);

enum class CodecStatus
{
  Valid,
  PayloadOutOfRange,
  MalformedPublicVersion,
  ComponentOutOfRange,
  InvalidParity,
};

// Fixed-capacity storage avoids allocation and std::string so a formatted
// public version can be produced during constant evaluation. The returned
// string_view borrows this object and must not outlive it.
struct PublicVersionString
{
  // Four instances of "-512", three separators, and a null terminator.
  std::array<char, 20> characters{};
  std::uint8_t length = 0;

  [[nodiscard]] constexpr const char* data() const { return characters.data(); }
  [[nodiscard]] constexpr std::string_view view() const { return {characters.data(), length}; }

  // Text used when policy-level encoding cannot produce a valid version.
  // Validity is always carried separately. 0.0.0.0 is not reserved: its
  // inverse under the established permutation naturally fails parity.
  [[nodiscard]] static constexpr PublicVersionString Fallback()
  {
    PublicVersionString result;
    result.characters[0] = '0';
    result.characters[1] = '.';
    result.characters[2] = '0';
    result.characters[3] = '.';
    result.characters[4] = '0';
    result.characters[5] = '.';
    result.characters[6] = '0';
    result.length = 7;
    return result;
  }
};

// Low-level codec results retain intermediate blocks intentionally. They make
// the bit-level format auditable and allow authoritative tests and diagnostic
// tools to identify whether failure occurred before or after permutation.
struct EncodeResult
{
  CodecStatus status = CodecStatus::PayloadOutOfRange;
  std::uint64_t logical_payload = 0;
  std::uint64_t pre_feistel_block = 0;
  std::uint64_t encoded_block = 0;
  PublicVersionString version{};

  [[nodiscard]] constexpr bool valid() const { return status == CodecStatus::Valid; }
};

struct DecodeResult
{
  CodecStatus status = CodecStatus::MalformedPublicVersion;
  std::uint64_t encoded_block = 0;
  std::uint64_t pre_feistel_block = 0;
  std::uint64_t logical_payload = 0;

  [[nodiscard]] constexpr bool valid() const { return status == CodecStatus::Valid; }
};

struct ParsedPublicVersion
{
  CodecStatus status = CodecStatus::MalformedPublicVersion;
  std::uint64_t encoded_block = 0;

  [[nodiscard]] constexpr bool valid() const { return status == CodecStatus::Valid; }
};

// Unsigned arithmetic gives the required modulo-2^32 wraparound semantics.
// The mixer exists to deterministically diffuse bits and derive constants; it
// is not a cryptographic hash and must not be described as one.
[[nodiscard]] constexpr std::uint32_t Mix(std::uint32_t x)
{
  x += WEYL + ROOT;
  x *= KNUTH;
  x ^= x >> 16;
  x ^= x >> 11;
  return x;
}

// The recurrence is playfully known inside the project as R'Lyeh's Constant.
// It is not an established mathematical term. Derivation keeps the constants
// reproducible and makes each deliberate appearance of 1928 visible.
[[nodiscard]] constexpr std::array<std::uint32_t, FEISTEL_ROUNDS> GenerateRoundConstants()
{
  std::array<std::uint32_t, FEISTEL_ROUNDS> constants{};
  constants[0] = Mix(ROOT);
  for (std::size_t i = 1; i < constants.size(); i++)
    constants[i] = Mix(constants[i - 1] + ROOT);
  return constants;
}

inline constexpr auto ROUND_CONSTANTS = GenerateRoundConstants();

[[nodiscard]] constexpr std::uint32_t RoundFunction(std::uint32_t right, std::uint32_t round_constant)
{
  std::uint32_t x = right ^ round_constant;
  x += ROOT;
  return Mix(x) & HALF_MASK;
}

// Feistel is used because it gives an exact permutation even though the round
// function itself is not invertible. Reversing the round order recovers every
// input block. This reversibility, rather than cryptographic strength, is the
// reason for the construction.
//
// The 40-bit state is split as L = bits 39..20 and R = bits 19..0. The output
// is L6 || R6; there is no additional final swap.
[[nodiscard]] constexpr std::uint64_t PermuteBlock(std::uint64_t block)
{
  std::uint32_t left = static_cast<std::uint32_t>((block >> 20) & HALF_MASK);
  std::uint32_t right = static_cast<std::uint32_t>(block & HALF_MASK);

  for (const std::uint32_t constant : ROUND_CONSTANTS)
  {
    const std::uint32_t next_left = right;
    const std::uint32_t next_right = (left ^ RoundFunction(right, constant)) & HALF_MASK;
    left = next_left;
    right = next_right;
  }

  return (static_cast<std::uint64_t>(left) << 20) | right;
}

[[nodiscard]] constexpr std::uint64_t InversePermuteBlock(std::uint64_t block)
{
  std::uint32_t left = static_cast<std::uint32_t>((block >> 20) & HALF_MASK);
  std::uint32_t right = static_cast<std::uint32_t>(block & HALF_MASK);

  for (std::size_t i = FEISTEL_ROUNDS; i > 0; i--)
  {
    const std::uint32_t previous_right = left;
    const std::uint32_t previous_left = (right ^ RoundFunction(left, ROUND_CONSTANTS[i - 1])) & HALF_MASK;
    left = previous_left;
    right = previous_right;
  }

  return (static_cast<std::uint64_t>(left) << 20) | right;
}

// Signed decimal components make negative values ordinary throughout the
// public representation. They are not errors: each is the two's-complement
// interpretation of one 10-bit field, with the valid range -512..511.
[[nodiscard]] constexpr std::array<std::int16_t, 4> BlockToComponents(std::uint64_t block)
{
  std::array<std::int16_t, 4> components{};
  for (std::size_t i = 0; i < components.size(); i++)
  {
    const unsigned shift = static_cast<unsigned>((3 - i) * 10);
    const std::uint16_t chunk = static_cast<std::uint16_t>((block >> shift) & 0x3FFu);
    components[i] = static_cast<std::int16_t>(chunk >= 0x200u ? static_cast<int>(chunk) - 0x400 : chunk);
  }
  return components;
}

[[nodiscard]] constexpr std::uint64_t ComponentsToBlock(const std::array<std::int16_t, 4>& components)
{
  std::uint64_t block = 0;
  for (const std::int16_t component : components)
    block = (block << 10) | (static_cast<std::uint16_t>(component) & 0x3FFu);
  return block;
}

constexpr void AppendComponent(PublicVersionString& output, std::int16_t component)
{
  int value = component;
  if (value < 0)
  {
    output.characters[output.length++] = '-';
    value = -value;
  }

  std::array<char, 3> reversed{};
  std::size_t digits = 0;
  do
  {
    reversed[digits++] = static_cast<char>('0' + (value % 10));
    value /= 10;
  } while (value != 0);

  while (digits > 0)
    output.characters[output.length++] = reversed[--digits];
}

[[nodiscard]] constexpr PublicVersionString FormatPublicVersion(std::uint64_t block)
{
  PublicVersionString output;
  const auto components = BlockToComponents(block);
  for (std::size_t i = 0; i < components.size(); i++)
  {
    if (i != 0)
      output.characters[output.length++] = '.';
    AppendComponent(output, components[i]);
  }
  return output;
}

// Parses exactly four period-separated decimal components. Syntax and range
// errors are distinguished from the parity validation performed only after
// inverse permutation.
[[nodiscard]] constexpr ParsedPublicVersion ParsePublicVersion(std::string_view version)
{
  std::array<std::int16_t, 4> components{};
  std::size_t position = 0;

  for (std::size_t component_index = 0; component_index < components.size(); component_index++)
  {
    if (position >= version.size())
      return {};

    bool negative = false;
    if (version[position] == '-')
    {
      negative = true;
      position++;
    }

    if (position >= version.size() || version[position] < '0' || version[position] > '9')
      return {};

    int value = 0;
    while (position < version.size() && version[position] >= '0' && version[position] <= '9')
    {
      value = (value * 10) + (version[position] - '0');
      if (value > 512)
        return {CodecStatus::ComponentOutOfRange, 0};
      position++;
    }

    value = negative ? -value : value;
    if (value < -512 || value > 511)
      return {CodecStatus::ComponentOutOfRange, 0};
    components[component_index] = static_cast<std::int16_t>(value);

    if (component_index + 1 < components.size())
    {
      if (position >= version.size() || version[position] != '.')
        return {};
      position++;
    }
  }

  if (position != version.size())
    return {};

  return {CodecStatus::Valid, ComponentsToBlock(components)};
}

// Encodes an opaque value that must fit the 39-bit logical payload. Policy
// code is responsible for assigning meaning to those bits.
[[nodiscard]] constexpr EncodeResult EncodePayload(std::uint64_t payload)
{
  if (payload > PAYLOAD_MASK)
    return {};

  // The parity bit is one exactly when the payload has odd population count,
  // making the complete 40-bit pre-Feistel block even parity. This is a small
  // sanity check for malformed or mistyped versions, not security or robust
  // integrity protection. It is appended before permutation so it participates
  // in the same reversible transformation as every payload bit.
  const std::uint64_t parity = std::popcount(payload) & 1u;
  const std::uint64_t pre_feistel_block = (payload << 1) | parity;
  const std::uint64_t encoded_block = PermuteBlock(pre_feistel_block);
  return {CodecStatus::Valid, payload, pre_feistel_block, encoded_block, FormatPublicVersion(encoded_block)};
}

// Inverts a 40-bit encoded block and accepts it only when the recovered checked
// block has even parity. No public value, including zero, is special-cased.
[[nodiscard]] constexpr DecodeResult DecodePermutedBlock(std::uint64_t encoded_block)
{
  if (encoded_block > BLOCK_MASK)
    return {};

  const std::uint64_t pre_feistel_block = InversePermuteBlock(encoded_block);
  if ((std::popcount(pre_feistel_block) & 1u) != 0)
    return {CodecStatus::InvalidParity, encoded_block, pre_feistel_block, 0};

  return {CodecStatus::Valid, encoded_block, pre_feistel_block, pre_feistel_block >> 1};
}

[[nodiscard]] constexpr DecodeResult DecodePublicVersion(std::string_view version)
{
  const ParsedPublicVersion parsed = ParsePublicVersion(version);
  if (!parsed.valid())
    return {parsed.status, 0, 0, 0};
  return DecodePermutedBlock(parsed.encoded_block);
}

} // namespace RLyeh::Pnakotic
