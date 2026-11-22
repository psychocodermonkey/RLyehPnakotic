// SPDX-FileCopyrightText: 2026 A.D. (PsychoCoderMonkey) <andrew.dixon@rlyeh.dev>
// SPDX-License-Identifier: GPL-3.0-only

#include "RLyehPnakotic/Pnakotic.h"
#include "test_framework.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace {

using namespace RLyeh::Pnakotic;

struct Fixture
{
  std::string_view date;
  std::string_view hash;
  std::string_view version;
  std::int32_t date_delta;
};

// Real commit identities and supplied calendar dates; no Git lookup occurs in the test.
constexpr std::array FIXTURES = {
  Fixture{"2026-08-16", "015aa43", "-95.-79.418.415", -1},
  Fixture{"2026-08-16", "1219f3b", "56.-192.466.424", -1},
  Fixture{"2026-08-16", "52e95ba", "-376.-141.6.158", -1},
  Fixture{"2026-08-16", "6c8affa", "-65.272.-167.330", -1},
  Fixture{"2026-08-16", "a0ab8d1", "-85.-101.162.294", -1},
  Fixture{"2026-08-16", "e772bda", "496.196.196.106", -1},
  Fixture{"2026-08-17", "0b0f747", "-309.341.-213.384", 0},
  Fixture{"2026-08-17", "546bdbb", "-267.-19.-217.169", 0},
  Fixture{"2026-08-17", "5b05afb", "178.165.-225.-457", 0},
  Fixture{"2026-08-17", "6a75cdd", "3.287.95.-7", 0},
  Fixture{"2026-08-17", "82dc583", "318.117.163.440", 0},
  Fixture{"2026-08-17", "ffe87e2", "48.221.381.293", 0},
  Fixture{"2026-08-18", "100501c", "-140.-110.-271.-119", 1},
  Fixture{"2026-08-18", "187a7e3", "-231.-74.-393.39", 1},
  Fixture{"2026-08-18", "1dc0945", "-309.383.-59.469", 1},
  Fixture{"2026-08-18", "575834a", "211.6.-90.-67", 1},
  Fixture{"2026-08-18", "a5a8876", "-461.-404.-407.430", 1},
  Fixture{"2026-08-18", "b2da447", "193.-158.455.486", 1},
  Fixture{"2026-11-21", "b9bcc9f", "-48.18.63.-69", 96},
  Fixture{"2026-11-22", "7232490", "364.-328.300.-83", 97},
};

static_assert(FIXTURES.size() == 20);
static_assert(Projects::RLyehPnakotic.epoch == "2026-08-17");
constexpr SCMVersionResult COMPILE_TIME_KNOWN_VERSION =
  EncodeSCMVersion(Projects::RLyehPnakotic, "2026-08-17", "546bdbb");
static_assert(COMPILE_TIME_KNOWN_VERSION.valid());
static_assert(COMPILE_TIME_KNOWN_VERSION.version.view() == "-267.-19.-217.169");
static_assert(COMPILE_TIME_KNOWN_VERSION.date_delta == 0);
static_assert(COMPILE_TIME_KNOWN_VERSION.commit_prefix == 0x546BDBu);

TEST(RLyehPnakoticPolicy, DefinesProjectPolicy)
{
  EXPECT_EQ(Projects::RLyehPnakotic.id, ProjectId::RLyehPnakotic);
  EXPECT_EQ(Projects::RLyehPnakotic.display_name, "R'Lyeh Pnakotic");
  EXPECT_EQ(Projects::RLyehPnakotic.epoch, "2026-08-17");
  EXPECT_FALSE(Projects::RLyehPnakotic.provisional_epoch);
}

TEST(RLyehPnakoticPolicy, AppearsInSupportedProjects)
{
  bool found = false;
  for (const ProjectPolicy& project : Projects::Supported)
    found |= project.id == ProjectId::RLyehPnakotic;
  EXPECT_TRUE(found);
}

TEST(RLyehPnakoticPolicy, EncodesAndDecodesCommitSamples)
{
  for (const Fixture& fixture : FIXTURES)
  {
    const int failures_before = TestFramework::failures;
    const SCMVersionResult encoded = EncodeSCMVersion(Projects::RLyehPnakotic, fixture.date, fixture.hash);
    EXPECT_TRUE(encoded.valid()) << fixture.hash;
    if (encoded.valid())
    {
      EXPECT_EQ(encoded.version.view(), fixture.version) << fixture.hash;
      EXPECT_EQ(encoded.date_delta, fixture.date_delta) << fixture.hash;

      const SCMLocatorResult decoded = DecodeSCMVersion(Projects::RLyehPnakotic, encoded.version.view());
      EXPECT_TRUE(decoded.valid()) << fixture.hash;
      if (decoded.valid())
      {
        EXPECT_EQ(decoded.commit_date.view(), fixture.date) << fixture.hash;
        EXPECT_EQ(decoded.commit_prefix.view(), fixture.hash.substr(0, 6)) << fixture.hash;
        EXPECT_EQ(decoded.date_delta, fixture.date_delta) << fixture.hash;
      }
    }

    TestFramework::ReportCommitSample(fixture.date, fixture.hash, TestFramework::failures == failures_before);
  }
}

} // namespace
