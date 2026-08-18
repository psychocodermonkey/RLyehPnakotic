// SPDX-FileCopyrightText: 2026 A.D. (PsychoCoderMonkey) <andrew.dixon@rlyeh.dev>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "RLyehPnakotic/Version.h"

#include <array>
#include <string_view>

namespace RLyeh::Pnakotic {

// Project identity is explicit because the generic codec has no calendar
// semantics. Adding a project extends policy data without changing the shared
// permutation or payload format.
enum class ProjectId
{
  RLyehPSX,
};

// A policy binds a R'Lyeh project to its single authoritative epoch. Consumers
// should use these definitions rather than duplicate epoch strings, which could
// make encoding and decoding disagree across applications and tools.
struct ProjectPolicy
{
  ProjectId id;
  std::string_view display_name;
  std::string_view epoch;
  bool provisional_epoch;
};

namespace Projects {

// The epoch remains provisional until the manually assigned R'Lyeh PSX 0.0.0
// release. Its calendar date then replaces this value permanently. That
// three-component release is a separate policy convention and is never emitted
// by the four-component Pnakotic encoder.
inline constexpr ProjectPolicy RLyehPSX = {
  ProjectId::RLyehPSX,
  "R'Lyeh PSX",
  "2026-05-31",
  true,
};

// Provides a constexpr inventory for tools such as the future GUI without
// requiring them to reproduce the set of supported policies.
inline constexpr std::array Supported = {RLyehPSX};

} // namespace Projects

// Preferred policy-level entry point. The consumer supplies SCM values but the
// selected project supplies the authoritative epoch. No repository access or
// Git invocation occurs here.
[[nodiscard]] constexpr SCMVersionResult EncodeSCMVersion(const ProjectPolicy& project, std::string_view commit_date,
                                                          std::string_view full_hash)
{
  return EncodeSCMVersion(commit_date, full_hash, project.epoch);
}

// Reconstructs the date and leading hash prefix under the selected project's
// epoch. It returns a locator, not complete Git provenance or a resolved commit.
[[nodiscard]] constexpr SCMLocatorResult DecodeSCMVersion(const ProjectPolicy& project, std::string_view version)
{
  return DecodeSCMVersion(version, project.epoch);
}

} // namespace RLyeh::Pnakotic
