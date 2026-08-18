// SPDX-FileCopyrightText: 2026 A.D. (PsychoCoderMonkey) <andrew.dixon@rlyeh.dev>
// SPDX-License-Identifier: GPL-3.0-only

#include "RLyehPnakotic/Pnakotic.h"

#include "test_framework.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace {

using namespace RLyeh::Pnakotic;

constexpr std::string_view TEST_EPOCH = "2026-05-31";

constexpr SCMVersionResult COMPILE_TIME_KNOWN_VERSION = EncodeSCMVersion(
  Projects::RLyehPSX, "2026-08-11", "c6ccd1117607ffac251ae29340f778ba72cc7587");
static_assert(COMPILE_TIME_KNOWN_VERSION.valid());
static_assert(COMPILE_TIME_KNOWN_VERSION.version.view() == "141.-422.-5.-149");
static_assert(Projects::RLyehPSX.epoch == TEST_EPOCH);

TEST(PnakoticCodec, GeneratesConstantsDeterministically)
{
  constexpr std::array<std::uint32_t, FEISTEL_ROUNDS> expected = {
    0x31F06F1Bu, 0xE69FC233u, 0x5C211913u, 0xA6D3CF6Fu, 0xBA5AB114u, 0x4BA07C73u,
  };
  EXPECT_EQ(ROUND_CONSTANTS, expected);
  EXPECT_EQ(GenerateRoundConstants(), expected);
}

TEST(PnakoticCodec, EncodesKnownPayloads)
{
  struct Vector
  {
    std::uint64_t payload;
    std::uint64_t pre_feistel_block;
    std::uint64_t encoded_block;
    std::string_view version;
  };

  constexpr std::array vectors = {
    Vector{0x0000000000ull, 0x0000000000ull, 0x24007C302Dull, "144.7.-244.45"},
    Vector{0x0000FFFFFFull, 0x0001FFFFFEull, 0x09EBF6114Cull, "39.-321.388.332"},
    Vector{0x4000000000ull, 0x8000000001ull, 0xFC2EFE1A47ull, "-16.-273.-122.-441"},
    Vector{0x3FFFFFFFFFull, 0x7FFFFFFFFEull, 0x045E967D66ull, "17.489.415.358"},
    Vector{0x0048C6CCD1ull, 0x00918D99A2ull, 0x2365AFEF6Bull, "141.-422.-5.-149"},
  };

  for (const Vector& vector : vectors)
  {
    const EncodeResult result = EncodePayload(vector.payload);
    ASSERT_TRUE(result.valid());
    EXPECT_EQ(result.pre_feistel_block, vector.pre_feistel_block);
    EXPECT_EQ(result.encoded_block, vector.encoded_block);
    EXPECT_EQ(result.version.view(), vector.version);
  }
}

TEST(PnakoticCodec, RoundTripsDeterministicPayloadSample)
{
  std::uint64_t state = 0x1928C0DEC0FFEEull;
  for (std::size_t i = 0; i < 100000; i++)
  {
    state = (state * 6364136223846793005ull) + 1442695040888963407ull;
    const std::uint64_t payload = state & PAYLOAD_MASK;
    const EncodeResult encoded = EncodePayload(payload);
    ASSERT_TRUE(encoded.valid());

    const DecodeResult decoded = DecodePublicVersion(encoded.version.view());
    ASSERT_TRUE(decoded.valid());
    EXPECT_EQ(decoded.logical_payload, payload);

    const EncodeResult reencoded = EncodePayload(decoded.logical_payload);
    ASSERT_TRUE(reencoded.valid());
    EXPECT_EQ(reencoded.version.view(), encoded.version.view());
  }
}

TEST(PnakoticCodec, InvertsFeistelBoundaryAndRepresentativeBlocks)
{
  constexpr std::array<std::uint64_t, 7> blocks = {
    0, 1, BLOCK_MASK, std::uint64_t{1} << 39, (std::uint64_t{1} << 20) - 1, std::uint64_t{1} << 20, 0x1928ABCDEFu,
  };

  for (const std::uint64_t block : blocks)
    EXPECT_EQ(InversePermuteBlock(PermuteBlock(block)), block);
}

TEST(PnakoticCodec, HandlesParitySuccessAndFailure)
{
  const EncodeResult valid = EncodePayload(0x1928ABCDEFu);
  ASSERT_TRUE(valid.valid());
  EXPECT_EQ(DecodePermutedBlock(valid.encoded_block).status, CodecStatus::Valid);

  const std::uint64_t invalid_parity_block = PermuteBlock(1);
  EXPECT_EQ(DecodePermutedBlock(invalid_parity_block).status, CodecStatus::InvalidParity);
  EXPECT_EQ(DecodePublicVersion(FormatPublicVersion(invalid_parity_block).view()).status, CodecStatus::InvalidParity);
}

TEST(PnakoticCodec, AcceptsEveryPublicComponentBoundary)
{
  constexpr std::array<std::int16_t, 4> components = {-512, -1, 0, 511};
  const std::uint64_t block = ComponentsToBlock(components);
  EXPECT_EQ(BlockToComponents(block), components);
  EXPECT_EQ(FormatPublicVersion(block).view(), "-512.-1.0.511");

  const ParsedPublicVersion parsed = ParsePublicVersion("-512.-1.0.511");
  ASSERT_TRUE(parsed.valid());
  EXPECT_EQ(parsed.encoded_block, block);

  const ParsedPublicVersion positive_one = ParsePublicVersion("1.1.1.1");
  ASSERT_TRUE(positive_one.valid());
  EXPECT_EQ(BlockToComponents(positive_one.encoded_block), (std::array<std::int16_t, 4>{1, 1, 1, 1}));
}

TEST(PnakoticCodec, RejectsMalformedPublicVersions)
{
  constexpr std::array<std::string_view, 11> malformed = {
    "", "0", "0.0", "0.0.0", "0.0.0.0.0", "0..0.0", "0.0.a.0", "+1.0.0.0", "--1.0.0.0", "1.2.3.4x", "1 2.3.4.5",
  };

  for (const std::string_view version : malformed)
    EXPECT_EQ(ParsePublicVersion(version).status, CodecStatus::MalformedPublicVersion) << version;
}

TEST(PnakoticCodec, RejectsComponentsOutsideSignedTenBitRange)
{
  EXPECT_EQ(ParsePublicVersion("-513.0.0.0").status, CodecStatus::ComponentOutOfRange);
  EXPECT_EQ(ParsePublicVersion("512.0.0.0").status, CodecStatus::ComponentOutOfRange);
  EXPECT_EQ(ParsePublicVersion("999999.0.0.0").status, CodecStatus::ComponentOutOfRange);
}

TEST(PnakoticPolicy, EncodesAndDecodesKnownLocator)
{
  const SCMVersionResult encoded =
    EncodeSCMVersion(Projects::RLyehPSX, "2026-08-11", "c6ccd1117607ffac251ae29340f778ba72cc7587");
  ASSERT_TRUE(encoded.valid());
  EXPECT_EQ(encoded.date_delta, 72);
  EXPECT_EQ(encoded.commit_prefix, 0xC6CCD1u);
  EXPECT_EQ(encoded.version.view(), "141.-422.-5.-149");

  const SCMLocatorResult decoded = DecodeSCMVersion(Projects::RLyehPSX, encoded.version.view());
  ASSERT_TRUE(decoded.valid());
  EXPECT_EQ(decoded.date_delta, 72);
  EXPECT_EQ(decoded.commit_date.view(), "2026-08-11");
  EXPECT_EQ(decoded.commit_prefix.view(), "c6ccd1");
}

TEST(PnakoticPolicy, HandlesSignedDateDeltaBoundaries)
{
  const SCMVersionResult minimum = EncodeSCMVersion("1981-07-22", "000000", TEST_EPOCH);
  ASSERT_TRUE(minimum.valid());
  EXPECT_EQ(minimum.date_delta, MINIMUM_DATE_DELTA);

  const SCMVersionResult zero = EncodeSCMVersion("2026-05-31", "000000", TEST_EPOCH);
  ASSERT_TRUE(zero.valid());
  EXPECT_EQ(zero.date_delta, 0);

  const SCMVersionResult maximum = EncodeSCMVersion("2071-04-08", "ffffff", TEST_EPOCH);
  ASSERT_TRUE(maximum.valid());
  EXPECT_EQ(maximum.date_delta, MAXIMUM_DATE_DELTA);

  EXPECT_EQ(EncodeSCMVersion("1981-07-21", "000000", TEST_EPOCH).status, PolicyStatus::DateDeltaOutOfRange);
  EXPECT_EQ(EncodeSCMVersion("2071-04-09", "ffffff", TEST_EPOCH).status, PolicyStatus::DateDeltaOutOfRange);
}

TEST(PnakoticPolicy, HandlesCommitPrefixBoundaries)
{
  const SCMVersionResult minimum = EncodeSCMVersion("2026-05-31", "000000", TEST_EPOCH);
  ASSERT_TRUE(minimum.valid());
  EXPECT_EQ(minimum.commit_prefix, 0u);

  const SCMVersionResult maximum = EncodeSCMVersion("2026-05-31", "FFFFFF", TEST_EPOCH);
  ASSERT_TRUE(maximum.valid());
  EXPECT_EQ(maximum.commit_prefix, 0xFFFFFFu);
}

TEST(PnakoticPolicy, HandlesMissingAndInvalidSCMInformation)
{
  const SCMVersionResult missing_date = EncodeSCMVersion("", "abcdef", TEST_EPOCH);
  EXPECT_FALSE(missing_date.valid());
  EXPECT_EQ(missing_date.status, PolicyStatus::InvalidCommitDate);
  EXPECT_EQ(missing_date.version.view(), "0.0.0.0");

  const SCMVersionResult missing_hash = EncodeSCMVersion("2026-05-31", "", TEST_EPOCH);
  EXPECT_FALSE(missing_hash.valid());
  EXPECT_EQ(missing_hash.status, PolicyStatus::InvalidCommitHash);
  EXPECT_EQ(missing_hash.version.view(), "0.0.0.0");

  EXPECT_EQ(EncodeSCMVersion("2026-02-30", "abcdef", TEST_EPOCH).status, PolicyStatus::InvalidCommitDate);
  EXPECT_EQ(EncodeSCMVersion("2026-05-31", "xyzxyz", TEST_EPOCH).status, PolicyStatus::InvalidCommitHash);
  EXPECT_EQ(EncodeSCMVersion("2026-05-31", "abcdef", "not-a-date").status, PolicyStatus::InvalidEpoch);
}

TEST(PnakoticPolicy, ZeroPublicBlockIsNaturallyInvalidByParity)
{
  const ParsedPublicVersion parsed = ParsePublicVersion("0.0.0.0");
  ASSERT_TRUE(parsed.valid());
  EXPECT_EQ(parsed.encoded_block, 0u);

  const DecodeResult decoded = DecodePublicVersion("0.0.0.0");
  EXPECT_EQ(decoded.pre_feistel_block, 0xF1BE38A124ull);
  EXPECT_EQ(decoded.status, CodecStatus::InvalidParity);
}

} // namespace

int main()
{
  return TestFramework::RunAll();
}
