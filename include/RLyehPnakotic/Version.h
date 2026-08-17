// SPDX-FileCopyrightText: 2026 A.D. (PsychoCoderMonkey) <andrew.dixon@rlyeh.dev>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "RLyehPnakotic/Codec.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace RLyeh::Pnakotic {

// The 39-bit policy payload is [15-bit signed day delta][24-bit hash suffix].
// Fifteen bits provide a range around each project's epoch while preserving
// six hexadecimal characters of commit identity.
inline constexpr std::int32_t MINIMUM_DATE_DELTA = -16384;
inline constexpr std::int32_t MAXIMUM_DATE_DELTA = 16383;
inline constexpr std::uint32_t COMMIT_SUFFIX_MASK = 0xFFFFFFu;

// PolicyStatus reports input and representation failures without requiring
// exceptions, keeping the complete API suitable for constant evaluation.
enum class PolicyStatus
{
  Valid,
  InvalidCommitDate,
  InvalidEpoch,
  InvalidCommitHash,
  DateDeltaOutOfRange,
  MalformedPublicVersion,
  ComponentOutOfRange,
  InvalidParity,
};

// Fixed-size, null-terminated result strings keep decode allocation-free and
// constexpr-capable. Each returned view borrows its containing result object.
struct DateString
{
  std::array<char, 11> characters{};

  [[nodiscard]] constexpr const char* data() const { return characters.data(); }
  [[nodiscard]] constexpr std::string_view view() const { return {characters.data(), 10}; }
};

struct HashSuffixString
{
  std::array<char, 7> characters{};

  [[nodiscard]] constexpr const char* data() const { return characters.data(); }
  [[nodiscard]] constexpr std::string_view view() const { return {characters.data(), 6}; }
};

// Encoding always returns explicit validity. On failure, version contains the
// display-safe 0.0.0.0 fallback, which must never be mistaken for valid merely
// by inspecting its text.
struct SCMVersionResult
{
  PolicyStatus status = PolicyStatus::InvalidCommitDate;
  PublicVersionString version = PublicVersionString::Fallback();
  std::int32_t date_delta = 0;
  std::uint32_t commit_suffix = 0;

  [[nodiscard]] constexpr bool valid() const { return status == PolicyStatus::Valid; }
};

// A decoded locator deliberately contains only the calendar date and rightmost
// six hash characters. Resolving a full commit remains a consumer repository
// operation outside Pnakotic.
struct SCMLocatorResult
{
  PolicyStatus status = PolicyStatus::MalformedPublicVersion;
  DateString commit_date{};
  HashSuffixString commit_suffix{};
  std::int32_t date_delta = 0;

  [[nodiscard]] constexpr bool valid() const { return status == PolicyStatus::Valid; }
};

struct ParsedDate
{
  bool valid = false;
  std::chrono::sys_days day{};
};

[[nodiscard]] constexpr bool IsDecimalDigit(char value)
{
  return value >= '0' && value <= '9';
}

// Accepts the strict YYYY-MM-DD calendar form used by project epochs and SCM
// metadata, rejecting nonexistent dates rather than normalizing them.
[[nodiscard]] constexpr ParsedDate ParseCalendarDate(std::string_view date)
{
  if (date.size() != 10 || date[4] != '-' || date[7] != '-')
    return {};

  constexpr std::array<std::size_t, 8> DATE_DIGIT_POSITIONS = {0, 1, 2, 3, 5, 6, 8, 9};
  for (const std::size_t index : DATE_DIGIT_POSITIONS)
  {
    if (!IsDecimalDigit(date[index]))
      return {};
  }

  const int year_value = ((date[0] - '0') * 1000) + ((date[1] - '0') * 100) + ((date[2] - '0') * 10) + (date[3] - '0');
  const unsigned month_value = static_cast<unsigned>(((date[5] - '0') * 10) + (date[6] - '0'));
  const unsigned day_value = static_cast<unsigned>(((date[8] - '0') * 10) + (date[9] - '0'));

  const std::chrono::year_month_day calendar_date{std::chrono::year{year_value}, std::chrono::month{month_value},
                                                  std::chrono::day{day_value}};
  if (year_value == 0 || !calendar_date.ok())
    return {};

  return {true, std::chrono::sys_days{calendar_date}};
}

[[nodiscard]] constexpr int HexDigitValue(char value)
{
  if (value >= '0' && value <= '9')
    return value - '0';
  if (value >= 'a' && value <= 'f')
    return value - 'a' + 10;
  if (value >= 'A' && value <= 'F')
    return value - 'A' + 10;
  return -1;
}

struct ParsedCommitSuffix
{
  bool valid = false;
  std::uint32_t value = 0;
};

// Validates the entire supplied hexadecimal identity, then extracts its
// rightmost six characters. This is a 24-bit suffix locator, deliberately not
// Git's conventional leading abbreviated hash.
[[nodiscard]] constexpr ParsedCommitSuffix ParseCommitSuffix(std::string_view full_hash)
{
  if (full_hash.size() < 6)
    return {};

  for (const char character : full_hash)
  {
    if (HexDigitValue(character) < 0)
      return {};
  }

  std::uint32_t suffix = 0;
  for (std::size_t i = full_hash.size() - 6; i < full_hash.size(); i++)
    suffix = (suffix << 4) | static_cast<std::uint32_t>(HexDigitValue(full_hash[i]));
  return {true, suffix};
}

[[nodiscard]] constexpr PolicyStatus PolicyStatusFromCodecStatus(CodecStatus status)
{
  switch (status)
  {
    case CodecStatus::Valid:
      return PolicyStatus::Valid;
    case CodecStatus::ComponentOutOfRange:
      return PolicyStatus::ComponentOutOfRange;
    case CodecStatus::InvalidParity:
      return PolicyStatus::InvalidParity;
    case CodecStatus::PayloadOutOfRange:
    case CodecStatus::MalformedPublicVersion:
    default:
      return PolicyStatus::MalformedPublicVersion;
  }
}

// Advanced epoch-explicit entry point used to define and test policy behavior.
// Most consumers should prefer the ProjectPolicy overload in ProjectPolicy.h
// so epochs remain authoritative rather than being duplicated at call sites.
//
// The policy layout is [15-bit signed day delta][24-bit commit suffix]. Input
// failures return a specific status and the 0.0.0.0 fallback representation.
[[nodiscard]] constexpr SCMVersionResult EncodeSCMVersion(std::string_view commit_date, std::string_view full_hash,
                                                          std::string_view epoch)
{
  const ParsedDate parsed_commit_date = ParseCalendarDate(commit_date);
  if (!parsed_commit_date.valid)
    return {PolicyStatus::InvalidCommitDate};

  const ParsedDate parsed_epoch = ParseCalendarDate(epoch);
  if (!parsed_epoch.valid)
    return {PolicyStatus::InvalidEpoch};

  const ParsedCommitSuffix parsed_suffix = ParseCommitSuffix(full_hash);
  if (!parsed_suffix.valid)
    return {PolicyStatus::InvalidCommitHash};

  const auto delta_count = (parsed_commit_date.day - parsed_epoch.day).count();
  if (delta_count < MINIMUM_DATE_DELTA || delta_count > MAXIMUM_DATE_DELTA)
    return {PolicyStatus::DateDeltaOutOfRange};

  const std::int32_t date_delta = static_cast<std::int32_t>(delta_count);
  const std::uint64_t encoded_date = static_cast<std::uint32_t>(date_delta) & 0x7FFFu;
  const std::uint64_t payload = (encoded_date << 24) | parsed_suffix.value;
  const EncodeResult encoded = EncodePayload(payload);
  return {PolicyStatus::Valid, encoded.version, date_delta, parsed_suffix.value};
}

[[nodiscard]] constexpr DateString FormatCalendarDate(std::chrono::sys_days day)
{
  const std::chrono::year_month_day calendar_date{day};
  const int year = static_cast<int>(calendar_date.year());
  const unsigned month = static_cast<unsigned>(calendar_date.month());
  const unsigned day_of_month = static_cast<unsigned>(calendar_date.day());

  DateString output;
  output.characters[0] = static_cast<char>('0' + ((year / 1000) % 10));
  output.characters[1] = static_cast<char>('0' + ((year / 100) % 10));
  output.characters[2] = static_cast<char>('0' + ((year / 10) % 10));
  output.characters[3] = static_cast<char>('0' + (year % 10));
  output.characters[4] = '-';
  output.characters[5] = static_cast<char>('0' + (month / 10));
  output.characters[6] = static_cast<char>('0' + (month % 10));
  output.characters[7] = '-';
  output.characters[8] = static_cast<char>('0' + (day_of_month / 10));
  output.characters[9] = static_cast<char>('0' + (day_of_month % 10));
  return output;
}

[[nodiscard]] constexpr HashSuffixString FormatCommitSuffix(std::uint32_t suffix)
{
  constexpr std::string_view HEX_DIGITS = "0123456789abcdef";
  HashSuffixString output;
  for (std::size_t i = 0; i < 6; i++)
  {
    const unsigned shift = static_cast<unsigned>((5 - i) * 4);
    output.characters[i] = HEX_DIGITS[(suffix >> shift) & 0xFu];
  }
  return output;
}

// Advanced inverse of the epoch-explicit encoder. A syntactically valid
// X.Y.Z.W value is accepted only if inverse permutation also recovers valid
// even parity; parity is a sanity check, not cryptographic authentication.
[[nodiscard]] constexpr SCMLocatorResult DecodeSCMVersion(std::string_view version, std::string_view epoch)
{
  const ParsedDate parsed_epoch = ParseCalendarDate(epoch);
  if (!parsed_epoch.valid)
    return {PolicyStatus::InvalidEpoch};

  const DecodeResult decoded = DecodePublicVersion(version);
  if (!decoded.valid())
    return {PolicyStatusFromCodecStatus(decoded.status)};

  const std::uint16_t encoded_date = static_cast<std::uint16_t>((decoded.logical_payload >> 24) & 0x7FFFu);
  const std::int32_t date_delta = (encoded_date & 0x4000u) != 0 ? static_cast<std::int32_t>(encoded_date) - 0x8000 :
                                                                  static_cast<std::int32_t>(encoded_date);
  const std::uint32_t suffix = static_cast<std::uint32_t>(decoded.logical_payload & COMMIT_SUFFIX_MASK);
  const std::chrono::sys_days commit_day = parsed_epoch.day + std::chrono::days{date_delta};
  const std::chrono::year_month_day calendar_date{commit_day};
  const int year = static_cast<int>(calendar_date.year());
  if (!calendar_date.ok() || year < 1 || year > 9999)
    return {PolicyStatus::DateDeltaOutOfRange};
  return {PolicyStatus::Valid, FormatCalendarDate(commit_day), FormatCommitSuffix(suffix), date_delta};
}

} // namespace RLyeh::Pnakotic
