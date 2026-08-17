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

__all__ = ('ArchivePin', 'DependencyPin', 'QTBASE', 'QTDECLARATIVE', 'DEPENDENCIES')


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
  flags: tuple[str, ...] = ()


# Pin records and their dependency-specific flags are exported through __all__.
# Platform behavior belongs in build-dependencies.py, not in the source manifest.
QTBASE = DependencyPin(
  name='qtbase',
  version='6.11.1',
  archive=ArchivePin(
    filename='qtbase-everywhere-src-6.11.1.tar.xz',
    url=(
      'https://download.qt.io/official_releases/qt/6.11/6.11.1/submodules/'
      'qtbase-everywhere-src-6.11.1.tar.xz'
    ),
    sha256='d9594a31228aa23ad6b531719a29b45f0f3989fe6c136d45767ea179f233c1ac',
    source_directory='qtbase-everywhere-src-6.11.1',
  ),
  flags=('-DQT_BUILD_EXAMPLES=OFF', '-DQT_BUILD_TESTS=OFF', '-DQT_INSTALL_CONFIG_INFO_FILES=OFF'),
)

QTDECLARATIVE = DependencyPin(
  name='qtdeclarative',
  version='6.11.1',
  archive=ArchivePin(
    filename='qtdeclarative-everywhere-src-6.11.1.tar.xz',
    url=(
      'https://download.qt.io/official_releases/qt/6.11/6.11.1/submodules/'
      'qtdeclarative-everywhere-src-6.11.1.tar.xz'
    ),
    sha256='52e670f670b0304f534b24f98c47ceb8a41bb710464414ebc9527ec71cc86aa4',
    source_directory='qtdeclarative-everywhere-src-6.11.1',
  ),
  dependencies=('qtbase',),
  flags=('-DQT_BUILD_EXAMPLES=OFF', '-DQT_BUILD_TESTS=OFF', '-DQT_INSTALL_CONFIG_INFO_FILES=OFF'),
)

DEPENDENCIES: tuple[DependencyPin, ...] = (QTBASE, QTDECLARATIVE)
