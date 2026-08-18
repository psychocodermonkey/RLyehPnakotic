#!/usr/bin/env python3
"""
 Program: Fetch, verify, and build dependencies for R'Lyeh Pnakotic.
    Name: Andrew Dixon            File: build-dependencies.py
    Date: 16 Aug 2026
   Notes:

    R'Lyeh Pnakotic
    Copyright (C) 2026
    A.D. (PsychoCoderMonkey) <andrew.dixon@rlyeh.dev>

    This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
    This is free software, and you are welcome to redistribute it
    under certain conditions; type `show c' for details.

........1.........2.........3.........4.........5.........6.........7.........8.........9.........0.........1.........2.........3..

Basic usage:
  python3 scripts/deps/build-dependencies.py --target darwin --arch universal
  python3 scripts/deps/build-dependencies.py --target linux --arch '*LOCAL'

The required target is one of ``darwin``, ``linux``, ``android``, or ``winnt``.
The architecture may be ``universal``, ``arm64``, ``x86_64``, or ``*LOCAL``;
``universal`` is valid only for Darwin, while ``*LOCAL`` resolves the current
process architecture. ``--deps`` selects the final installation directory and
defaults to the repository's ``deps`` directory.
"""

import argparse
import hashlib
import os
import platform
import shlex
import shutil
import subprocess
import tarfile
import tempfile
import urllib.request
from collections.abc import Mapping, Sequence
from dataclasses import dataclass
from pathlib import Path

# This script intentionally treats deps_versions.py as an external constants
# file. Its __all__ declaration constrains the wildcard import to public pins.
from deps_versions import *  # noqa: F403

# Wildcard-imported pin names are intentionally referenced below.
# ruff: noqa: F405

LOCAL_ARCHITECTURE = '*LOCAL'
SUPPORTED_ARCHITECTURES = {
  'darwin': frozenset({'universal', 'arm64', 'x86_64'}),
  'linux': frozenset({'arm64', 'x86_64'}),
  'android': frozenset({'arm64', 'x86_64'}),
  'winnt': frozenset({'arm64', 'x86_64'}),
}


@dataclass(frozen=True)
class BuildContext:
  """Resolved paths and target information shared by dependency build helpers.

  ``target`` and ``architecture`` have already been validated against
  ``SUPPORTED_ARCHITECTURES``. Path fields are absolute except for no promised
  relationship between the build workspace and the requested install directory.
  """

  repository_root: Path
  deps_dir: Path
  build_root: Path
  downloads_dir: Path
  sources_dir: Path
  target: str
  architecture: str


def main(deps_dir: Path, target: str, arch: str) -> int:
  """Validate the request and build each pinned dependency in prerequisite order.

  ``deps_dir`` is the final installation prefix, relative to the repository root
  unless absolute. ``target`` accepts ``darwin``, ``linux``, ``android``, or
  ``winnt``. ``arch`` accepts ``arm64``, ``x86_64``, Darwin-only ``universal``,
  or ``*LOCAL`` to detect the current process architecture.
  """

  resolved_arch = resolve_local_architecture() if arch == LOCAL_ARCHITECTURE else arch
  try:
    valid_architectures = SUPPORTED_ARCHITECTURES[target]
  except KeyError as error:
    raise ValueError(f'Unsupported target: {target}') from error

  if resolved_arch not in valid_architectures:
    valid_values = ', '.join(sorted(valid_architectures))
    raise ValueError(
      f'Architecture {resolved_arch!r} is not valid for target {target!r}; '
      f'expected one of: {valid_values}'
    )

  validate_dependency_pins(DEPENDENCIES)
  context = create_build_context(deps_dir, target, resolved_arch)
  print(f'Dependency directory: {context.deps_dir}')
  print(f'Build workspace: {context.build_root}')
  print(f'Target: {context.target}')
  print(f'Architecture: {context.architecture}')

  validate_qt_build_context(context)
  download_dependencies(DEPENDENCIES, context)
  verify_dependency_archives(DEPENDENCIES, context)

  # Dependency-specific build calls belong here in explicit prerequisite order.
  build_qtbase(context, QTBASE)
  build_qtshadertools(context, QTSHADERTOOLS)
  build_qtdeclarative(context, QTDECLARATIVE)

  shutil.rmtree(context.build_root)
  print(f'Removed build workspace: {context.build_root}')

  return 0


def build_qtbase(context: BuildContext, dependency: DependencyPin) -> None:
  """Build and install the pinned QtBase source for the requested target."""

  install_dir = qt_install_directory(context, dependency.version)
  source_dir = prepare_source(dependency, context)
  cmake_build(
    context,
    source_dir,
    dependency_build_directory(context, dependency),
    install_dir,
    flags=dependency.flags,
  )


def build_qtshadertools(context: BuildContext, dependency: DependencyPin) -> None:
  """Build Qt Shader Tools against QtBase in the shared Qt prefix."""

  install_dir = qt_install_directory(context, dependency.version)
  flags = dependency.flags + (f'-DQt6_ROOT={install_dir}', f'-DCMAKE_PREFIX_PATH={install_dir}')
  source_dir = prepare_source(dependency, context)
  cmake_build(
    context, source_dir, dependency_build_directory(context, dependency), install_dir, flags=flags
  )


def build_qtdeclarative(context: BuildContext, dependency: DependencyPin) -> None:
  """Build QtDeclarative against the QtBase installation in the same Qt prefix."""

  install_dir = qt_install_directory(context, dependency.version)
  flags = dependency.flags + (f'-DQt6_ROOT={install_dir}', f'-DCMAKE_PREFIX_PATH={install_dir}')
  source_dir = prepare_source(dependency, context)
  cmake_build(
    context, source_dir, dependency_build_directory(context, dependency), install_dir, flags=flags
  )


def validate_qt_build_context(context: BuildContext) -> None:
  """Require a currently implemented Qt host and target combination."""

  if context.target not in {'darwin', 'linux'}:
    raise ValueError(f'Qt builds for target {context.target!r} are not implemented')

  local_target = resolve_local_target()
  if context.target != local_target:
    raise ValueError(f'Native {context.target!r} Qt builds cannot run on host {local_target!r}')

  if context.target == 'linux':
    local_architecture = resolve_local_architecture()
    if context.architecture != local_architecture:
      raise ValueError(
        'Linux cross-architecture Qt builds are not implemented; '
        f'requested {context.architecture!r} on {local_architecture!r}'
      )


def qt_install_directory(context: BuildContext, version: str) -> Path:
  """Return the versioned Qt prefix for one target and architecture."""

  return context.deps_dir / 'Qt' / version / f'{context.target}-{context.architecture}'


def dependency_build_directory(context: BuildContext, dependency: DependencyPin) -> Path:
  """Return an isolated build directory for a dependency and build target."""

  build_name = f'{dependency.name}-{dependency.version}-{context.target}-{context.architecture}'
  return context.build_root / build_name


def resolve_local_target() -> str:
  """Return the current host as ``darwin``, ``linux``, or ``winnt``."""

  system = platform.system().lower()
  aliases = {'darwin': 'darwin', 'linux': 'linux', 'windows': 'winnt'}
  try:
    return aliases[system]
  except KeyError as error:
    raise ValueError(f'Unsupported local target: {system or "unknown"}') from error


def resolve_local_architecture() -> str:
  """Return the current process architecture as ``arm64`` or ``x86_64``."""

  machine = platform.machine().lower()
  aliases = {'aarch64': 'arm64', 'amd64': 'x86_64', 'arm64': 'arm64', 'x86_64': 'x86_64'}

  try:
    return aliases[machine]
  except KeyError as error:
    raise ValueError(f'Unsupported local architecture: {machine or "unknown"}') from error


def parse_architecture(value: str) -> str:
  """Normalize an architecture argument while preserving the ``*LOCAL`` sentinel."""

  return LOCAL_ARCHITECTURE if value.upper() == LOCAL_ARCHITECTURE else value.lower()


def resolve_deps_directory(deps_dir: Path) -> Path:
  """Resolve an absolute install prefix from an absolute or repository-relative path."""

  if deps_dir.is_absolute():
    return deps_dir.resolve()

  repository_root = Path(__file__).resolve().parents[2]
  return (repository_root / deps_dir).resolve()


def validate_dependency_pins(dependencies: Sequence[DependencyPin]) -> None:
  """Validate unique, secure pins arranged in explicit prerequisite order."""

  names: set[str] = set()
  filenames: set[str] = set()

  for dependency in dependencies:
    if dependency.name in names:
      raise ValueError(f'Duplicate dependency name: {dependency.name}')
    names.add(dependency.name)

    archive = dependency.archive
    if archive.filename in filenames:
      raise ValueError(f'Duplicate archive filename: {archive.filename}')
    filenames.add(archive.filename)

    if not archive.url.startswith('https://'):
      raise ValueError(f'Dependency {dependency.name} does not use an HTTPS source URL')
    if len(archive.sha256) != 64 or any(
      character not in '0123456789abcdef' for character in archive.sha256.lower()
    ):
      raise ValueError(f'Dependency {dependency.name} has an invalid SHA-256 digest')
    if (
      not archive.source_directory
      or Path(archive.source_directory).name != archive.source_directory
    ):
      raise ValueError(f'Dependency {dependency.name} has an invalid source directory')

  available: set[str] = set()
  for dependency in dependencies:
    missing = set(dependency.dependencies) - available
    if missing:
      missing_names = ', '.join(sorted(missing))
      raise ValueError(
        f'Dependency {dependency.name} is ordered before its prerequisites: {missing_names}'
      )
    available.add(dependency.name)


def sha256_digest(path: Path) -> str:
  """Return the lowercase SHA-256 digest of the file at ``path``."""

  digest = hashlib.sha256()
  with path.open('rb') as source:
    while chunk := source.read(1024 * 1024):
      digest.update(chunk)
  return digest.hexdigest()


def verify_archive(path: Path, expected_sha256: str) -> None:
  """Require ``path`` to match the expected 64-character SHA-256 digest."""

  actual_sha256 = sha256_digest(path)
  if actual_sha256 != expected_sha256.lower():
    raise ValueError(
      f'SHA-256 verification failed for {path.name}: '
      f'expected {expected_sha256.lower()}, got {actual_sha256}'
    )


def dependency_archive_path(dependency: DependencyPin, context: BuildContext) -> Path:
  """Return the cached archive path for a pinned dependency."""

  return context.downloads_dir / dependency.archive.filename


def download_dependencies(dependencies: Sequence[DependencyPin], context: BuildContext) -> None:
  """Download every pinned dependency archive before verification or building."""

  for dependency in dependencies:
    download_archive(dependency.archive, context.downloads_dir)


def verify_dependency_archives(
  dependencies: Sequence[DependencyPin], context: BuildContext
) -> None:
  """Verify every downloaded archive against its pinned SHA-256 digest."""

  for dependency in dependencies:
    archive_path = dependency_archive_path(dependency, context)
    verify_archive(archive_path, dependency.archive.sha256)
    print(f'Verified download: {archive_path.name}')


def download_archive(archive: ArchivePin, downloads_dir: Path) -> Path:
  """Download an archive, or reuse an existing cached copy for later verification."""

  downloads_dir.mkdir(parents=True, exist_ok=True)
  destination = downloads_dir / archive.filename
  if destination.is_file():
    print(f'Using cached download: {destination.name}')
    return destination

  temporary = destination.with_suffix(f'{destination.suffix}.part')
  temporary.unlink(missing_ok=True)
  print(f'Downloading {archive.url}')

  try:
    with urllib.request.urlopen(archive.url) as response, temporary.open('wb') as output:
      shutil.copyfileobj(response, output)
    temporary.replace(destination)
  except Exception:
    temporary.unlink(missing_ok=True)
    raise

  return destination


def extract_archive(archive_path: Path, archive: ArchivePin, sources_dir: Path) -> Path:
  """Safely replace and return the source tree extracted from ``archive_path``."""

  sources_dir.mkdir(parents=True, exist_ok=True)
  destination = sources_dir / archive.source_directory
  if destination.exists():
    shutil.rmtree(destination)

  with tempfile.TemporaryDirectory(
    prefix=f'{archive.source_directory}-', dir=sources_dir
  ) as temporary:
    temporary_path = Path(temporary)
    with tarfile.open(archive_path, mode='r:*') as source_archive:
      source_archive.extractall(temporary_path, filter='data')

    extracted = temporary_path / archive.source_directory
    if not extracted.is_dir():
      raise ValueError(f'{archive_path.name} did not contain {archive.source_directory}')
    extracted.replace(destination)

  return destination


def prepare_source(dependency: DependencyPin, context: BuildContext) -> Path:
  """Extract a previously downloaded and verified dependency archive."""

  archive_path = dependency_archive_path(dependency, context)
  if not archive_path.is_file():
    raise ValueError(f'Dependency archive does not exist: {archive_path}')
  return extract_archive(archive_path, dependency.archive, context.sources_dir)


def run_command(
  command: Sequence[str | Path], *, cwd: Path, environment: Mapping[str, str] | None = None
) -> None:
  """Run ``command`` in ``cwd`` with an optional complete process environment."""

  arguments = [str(argument) for argument in command]
  print(f'Running: {shlex.join(arguments)}', flush=True)
  subprocess.run(arguments, cwd=cwd, env=environment, check=True)


def apply_patch(source_dir: Path, patch_path: Path) -> None:
  """Apply a required unified patch to ``source_dir`` using ``patch -p1``."""

  if not patch_path.is_file():
    raise ValueError(f'Patch file does not exist: {patch_path}')
  run_command(('patch', '--batch', '--forward', '-p1', '-i', patch_path.resolve()), cwd=source_dir)


def cmake_build(
  context: BuildContext,
  source_dir: Path,
  build_dir: Path,
  install_dir: Path,
  *,
  flags: tuple[str, ...] = (),
  patch_path: Path | None = None,
) -> None:
  """Configure, build, and install a CMake dependency with Ninja.

  ``context`` supplies the target compiler and architecture. ``source_dir``
  contains the dependency source, ``build_dir`` is recreated for a clean
  out-of-tree build, and ``install_dir`` is the dependency installation prefix.
  ``flags`` contains dependency-specific CMake configuration arguments such as
  ``-DFEATURE_x=OFF``. When supplied, ``patch_path`` is applied before configure.
  """

  if patch_path is not None:
    apply_patch(source_dir, patch_path)

  if build_dir.exists():
    shutil.rmtree(build_dir)
  build_dir.mkdir(parents=True)
  install_dir.mkdir(parents=True, exist_ok=True)

  cmake_flags = ('-DCMAKE_C_COMPILER=clang', '-DCMAKE_CXX_COMPILER=clang++')
  if context.target == 'darwin':
    architectures = {'arm64': 'arm64', 'x86_64': 'x86_64', 'universal': 'arm64;x86_64'}
    cmake_flags += (f'-DCMAKE_OSX_ARCHITECTURES={architectures[context.architecture]}',)
  cmake_flags += flags

  run_command(
    (
      'cmake',
      '-S',
      source_dir,
      '-B',
      build_dir,
      '-G',
      'Ninja',
      '-DCMAKE_BUILD_TYPE=Release',
      f'-DCMAKE_INSTALL_PREFIX={install_dir}',
      *cmake_flags,
    ),
    cwd=source_dir,
  )
  parallel_jobs = os.cpu_count() or 1
  run_command(('cmake', '--build', build_dir, '--parallel', str(parallel_jobs)), cwd=source_dir)
  run_command(('cmake', '--install', build_dir), cwd=source_dir)


def create_build_context(deps_dir: Path, target: str, architecture: str) -> BuildContext:
  """Create the path layout for an already validated target and architecture."""

  repository_root = Path(__file__).resolve().parents[2]
  build_root = repository_root / 'deps-build'
  return BuildContext(
    repository_root=repository_root,
    deps_dir=resolve_deps_directory(deps_dir),
    build_root=build_root,
    downloads_dir=build_root / 'downloads',
    sources_dir=build_root / 'sources',
    target=target,
    architecture=architecture,
  )


# If the scripts/deps/build-dependencies.py is run (instead of imported as a module),
# call the main() function:
if __name__ == '__main__':
  # Setup the arg parser to import and parse arguments.
  parser = argparse.ArgumentParser()

  parser.add_argument(
    '--deps',
    '-d',
    type=Path,
    default=Path('deps'),
    help='Specify the path to the final dependency directory.',
  )

  parser.add_argument(
    '--target',
    '-t',
    type=str.lower,
    choices=tuple(SUPPORTED_ARCHITECTURES),
    required=True,
    help='Specify the build/dependency target.',
  )

  parser.add_argument(
    '--arch',
    '-a',
    type=parse_architecture,
    choices=(LOCAL_ARCHITECTURE, 'universal', 'arm64', 'x86_64'),
    default=LOCAL_ARCHITECTURE,
    help='Specify the target architecture; *LOCAL detects the current machine.',
  )

  args = parser.parse_args()

  try:
    raise SystemExit(main(args.deps, args.target, args.arch))

  except ValueError as error:
    parser.error(str(error))
