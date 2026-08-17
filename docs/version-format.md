# R'Lyeh Version Encoding

R'Lyeh SCM versions are deterministic, reversible obfuscations of a Git locator. They are not encryption and provide no
cryptographic security. Their four signed components deliberately do not communicate chronological ordering.

The manually assigned first public release is `0.0.0`, exactly three components. It is separate from the four-component
SCM format described here.

## Design intent

Pnakotic exists to give R'Lyeh projects a reproducible public development version without exposing an immediately
readable date or build sequence and without suggesting semantic-version precedence. Its intentionally strange output is
still a compact source locator: a project policy supplies the calendar context, and the decoded date plus hash prefix
narrows the corresponding repository search.

The format favors one authoritative, reversible implementation over convenience shortcuts. In particular:

- the C++ API remains `constexpr` so build-generated SCM strings can be encoded during compilation;
- project epochs live in Pnakotic so consumers cannot silently disagree about calendar interpretation;
- Git discovery, full provenance, tags, branches, and updater behavior remain consumer responsibilities;
- parity rejects some accidental malformed inputs but provides no security or authentication;
- the Feistel network hides the payload's obvious layout while preserving every possible checked block exactly; and
- signed components are normal values chosen to make the result visibly unlike a chronological semantic version.

These properties are format decisions. Changing field widths, constants, round count, signed rendering, parity
placement, or the hash-prefix convention creates a different format even if the replacement appears simpler.

## Inputs and epoch

The SCM encoder uses the checked-out `HEAD` commit's `%cs` committer calendar date and the first six hexadecimal
characters of its full hash. Dirty working-tree state does not affect the encoded value. Branch, describe string, full
hash, and full ISO-8601 committer timestamp remain available as separate build metadata.

R'Lyeh Pnakotic owns the project policy and its single authoritative epoch definition. The R'Lyeh PSX development epoch
is `2026-05-31`; it remains provisional until the public `0.0.0` release, when it is changed to that release's calendar
date and then frozen permanently.

## Logical payload

The logical payload is exactly 39 bits:

```text
bits 38..24  signed 15-bit two's-complement day delta from the epoch
bits 23..0   numeric value of the first six commit-hash characters
```

The valid date range is `-16384..16383` days and the hash prefix range is `0x000000..0xFFFFFF`.

One even-parity bit is appended as the least-significant bit:

```text
pre_feistel_block = (payload << 1) | parity
```

The parity bit is selected so the complete 40-bit block has an even number of set bits. Decode rejects an odd-parity
inverse result in a controlled manner.

## R'Lyeh permutation

The 40-bit block is split into unsigned 20-bit halves, `L0 = bits 39..20` and `R0 = bits 19..0`, and permuted through a
balanced Feistel network. The root value is intentionally `1928`.

There are exactly six rounds. The deliberately ridiculous derivation is:

```text
1928 = 2^3 * 241
EulerTotient(1928) = 960
digitalRoot(960) = digitalRoot(15) = 6
```

For rounds `i = 0..5`:

```text
L(i+1) = Ri
R(i+1) = Li XOR F(Ri, Ci)
```

The result is `L6 || R6`, without an additional final swap. Inversion uses `C5..C0` and recovers:

```text
Ri = L(i+1)
Li = R(i+1) XOR F(L(i+1), Ci)
```

### Mixer and R'Lyeh's Constant

All mixer arithmetic is unsigned modulo `2^32`:

```text
ROOT  = 1928
WEYL  = 0x61C88647
KNUTH = 0x9E3779B1

mix(x):
    x = x + WEYL + ROOT
    x = x * KNUTH
    x = x XOR (x >> 16)
    x = x XOR (x >> 11)
```

This combines Weyl-style additive mixing with Knuth-style multiplicative hashing. The round-constant recurrence is
internally known as **R'Lyeh's Constant**:

```text
C0 = mix(ROOT)
Cn = mix(C(n-1) + ROOT)
```

Exactly `C0..C5` are generated. The 20-bit round function is:

```text
F(R, C) = mix((R XOR C) + ROOT) & 0xFFFFF
```

## Public representation

The permuted block is split into four 10-bit chunks:

```text
bits 39..30  X
bits 29..20  Y
bits 19..10  Z
bits  9..0   W
```

Each chunk is rendered independently as a signed 10-bit two's-complement decimal integer in `-512..511`, producing
`X.Y.Z.W`. Decode parses exactly four in-range components, reconstructs their 10-bit patterns, inverses the Feistel
network, validates parity, and extracts the locator.

`0.0.0.0` is not explicitly reserved or remapped. Under the current constants, however, its unique inverse-Feistel
preimage is `0xF1BE38A124`, which has 19 set bits and therefore fails even parity. It is consequently unreachable from a
valid payload as an emergent property of this format. It remains the safe fallback text for unavailable or invalid SCM
input, and validity is always represented separately rather than inferred from the text.

## Consumer integration boundary

Pnakotic does not inspect source-control repositories or invoke Git. Each consumer owns repository discovery and the
collection of its commit date and full commit hash. A typical build integration writes those raw strings to a generated
header and passes them, together with a Pnakotic-owned project policy, to `EncodeSCMVersion`:

```cpp
constexpr auto public_version = RLyeh::Pnakotic::EncodeSCMVersion(
  RLyeh::Pnakotic::Projects::RLyehPSX,
  BuildMetadata::CommitDate,
  BuildMetadata::CommitHash);

static_assert(public_version.valid());
```

This keeps source-control interrogation and build-metadata generation in the consuming project while leaving one
authoritative encoder and decoder in Pnakotic. Because the API is `constexpr`, consumers such as R'Lyeh PSX can compute
their public version during compilation without a runtime dependency or a second implementation of the format.

Decoding returns only the commit calendar date and six-digit hash prefix. Resolving that locator to a complete commit,
and displaying any other repository metadata such as branch, tag, describe string, or dirty state, remain consumer
responsibilities outside Pnakotic.
