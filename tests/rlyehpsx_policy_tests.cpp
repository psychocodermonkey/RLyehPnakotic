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
  Fixture{"2026-02-18", "0787cfb", "299.-510.153.-500", -102},
  Fixture{"2026-05-31", "29d8ac1", "484.-19.220.-487", 0},
  Fixture{"2026-06-24", "0b2f603", "3.464.222.-400", 24},
  Fixture{"2026-06-27", "18e74b8", "-460.-204.151.209", 27},
  Fixture{"2026-06-30", "c353618", "413.-350.-419.-190", 30},
  Fixture{"2026-07-11", "b06c661", "-324.96.-15.242", 41},
  Fixture{"2026-07-12", "08a286e", "-44.172.164.-106", 42},
  Fixture{"2026-07-14", "3b36a99", "265.70.337.145", 44},
  Fixture{"2026-08-01", "1b5225e", "-42.153.275.-438", 62},
  Fixture{"2026-08-02", "1ca1754", "1.245.20.346", 63},
  Fixture{"2026-08-06", "32eef36", "-437.-353.-282.-230", 67},
  Fixture{"2026-08-09", "10ce90b", "299.-263.273.460", 70},
  Fixture{"2026-08-10", "11065b5", "55.-91.161.404", 71},
  Fixture{"2026-08-11", "c6ccd11", "141.-422.-5.-149", 72},
  Fixture{"2026-08-12", "138cf91", "-296.321.-5.144", 73},
  Fixture{"2026-08-13", "db7d78c", "57.-493.-352.309", 74},
  Fixture{"2026-08-16", "51c0a3a", "-265.-136.477.360", 77},
  Fixture{"2026-08-31", "95a9fea", "-216.295.-34.467", 92},
  Fixture{"2026-09-01", "6b18698", "55.-443.-106.79", 93},
  Fixture{"2026-10-20", "03d2cc2", "-126.396.-169.488", 142},
  Fixture{"2026-10-31", "03a7c93", "-5.205.-195.299", 153},
  Fixture{"2026-11-02", "4663f7f", "-463.-234.463.320", 155},
  Fixture{"2026-11-06", "453216a", "-39.-111.-511.22", 159},
  Fixture{"2026-11-07", "03605c5", "-183.-495.-170.-334", 160},
  Fixture{"2026-11-13", "131eb62", "163.387.221.340", 166},
};

static_assert(FIXTURES.size() == 25);
static_assert(Projects::RLyehPSX.epoch == "2026-05-31");
constexpr SCMVersionResult COMPILE_TIME_KNOWN_VERSION =
  EncodeSCMVersion(Projects::RLyehPSX, "2026-08-11", "c6ccd1117607ffac251ae29340f778ba72cc7587");
static_assert(COMPILE_TIME_KNOWN_VERSION.valid());
static_assert(COMPILE_TIME_KNOWN_VERSION.version.view() == "141.-422.-5.-149");
static_assert(COMPILE_TIME_KNOWN_VERSION.date_delta == 72);
static_assert(COMPILE_TIME_KNOWN_VERSION.commit_prefix == 0xC6CCD1u);

TEST(RLyehPSXPolicy, DefinesProjectPolicy)
{
  EXPECT_EQ(Projects::RLyehPSX.id, ProjectId::RLyehPSX);
  EXPECT_EQ(Projects::RLyehPSX.display_name, "R'Lyeh PSX");
  EXPECT_EQ(Projects::RLyehPSX.epoch, "2026-05-31");
  EXPECT_TRUE(Projects::RLyehPSX.provisional_epoch);
}

TEST(RLyehPSXPolicy, AppearsInSupportedProjects)
{
  bool found = false;
  for (const ProjectPolicy& project : Projects::Supported)
    found |= project.id == ProjectId::RLyehPSX;
  EXPECT_TRUE(found);
}

TEST(RLyehPSXPolicy, EncodesAndDecodesCommitSamples)
{
  for (const Fixture& fixture : FIXTURES)
  {
    const int failures_before = TestFramework::failures;
    const SCMVersionResult encoded = EncodeSCMVersion(Projects::RLyehPSX, fixture.date, fixture.hash);
    EXPECT_TRUE(encoded.valid()) << fixture.hash;
    if (encoded.valid())
    {
      EXPECT_EQ(encoded.version.view(), fixture.version) << fixture.hash;
      EXPECT_EQ(encoded.date_delta, fixture.date_delta) << fixture.hash;

      const SCMLocatorResult decoded = DecodeSCMVersion(Projects::RLyehPSX, encoded.version.view());
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
