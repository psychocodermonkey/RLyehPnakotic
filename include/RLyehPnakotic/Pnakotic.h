// SPDX-FileCopyrightText: 2026 A.D. (PsychoCoderMonkey) <andrew.dixon@rlyeh.dev>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

// Public umbrella header for the complete Pnakotic format and policy API.
//
// Pnakotic turns a project policy, commit calendar date, and commit identity
// into R'Lyeh's reversible four-component public version. It deliberately
// owns no Git or filesystem integration: consumers obtain SCM metadata and
// pass the raw values to this constexpr-capable implementation.
//
// This format is deterministic obfuscation and project identity, not
// cryptography. Its permutation and parity check provide neither secrecy nor
// security against deliberate modification.
#include "RLyehPnakotic/Codec.h"
#include "RLyehPnakotic/ProjectPolicy.h"
#include "RLyehPnakotic/Version.h"
