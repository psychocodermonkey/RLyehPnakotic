"""
 Program: Pinned dependency source information for R'Lyeh Pnakotic.
    Name: Andrew Dixon            File: deps_versions.py
    Date: 16 Aug 2026
   Notes:

    R'Lyeh Pnakotic
    Copyright (C) 2026
    A.D. (PsychoCoderMonkey) <andrew.dixon@rlyeh.dev>

    This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
    This is free software, and you are welcome to redistribute it
    under certain conditions; type `show c' for details.

........1.........2.........3.........4.........5.........6.........7.........8.........9.........0.........1.........2.........3..
"""

from dataclasses import dataclass

__all__ = ('ArchivePin', 'DependencyPin', 'DEPENDENCIES')


@dataclass(frozen=True)
class ArchivePin:
  filename: str
  url: str
  sha256: str
  source_directory: str


@dataclass(frozen=True)
class DependencyPin:
  name: str
  version: str
  archive: ArchivePin
  dependencies: tuple[str, ...] = ()


# Pin records are added here and exported through __all__. Build flags and
# platform behavior belong in build-dependencies.py, not in the source manifest.
DEPENDENCIES: tuple[DependencyPin, ...] = ()
