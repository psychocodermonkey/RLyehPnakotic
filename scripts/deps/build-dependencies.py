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
"""

import argparse
import hashlib
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
  repository_root: Path
  deps_dir: Path
  build_root: Path
  downloads_dir: Path
  sources_dir: Path
  target: str
  architecture: str


def main(deps_dir: Path, target: str, arch: str) -> int:
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

  # Dependency-specific build calls belong here in explicit prerequisite order.
  # For example:
  # build_qtbase(context, QTBASE)
  # build_qtdeclarative(context, QTDECLARATIVE)
  if not DEPENDENCIES:
    print('No dependencies are currently pinned.')

  return 0


def resolve_local_architecture() -> str:
  machine = platform.machine().lower()
  aliases = {'aarch64': 'arm64', 'amd64': 'x86_64', 'arm64': 'arm64', 'x86_64': 'x86_64'}

  try:
    return aliases[machine]
  except KeyError as error:
    raise ValueError(f'Unsupported local architecture: {machine or "unknown"}') from error


def parse_architecture(value: str) -> str:
  return LOCAL_ARCHITECTURE if value.upper() == LOCAL_ARCHITECTURE else value.lower()


def resolve_deps_directory(deps_dir: Path) -> Path:
  if deps_dir.is_absolute():
    return deps_dir.resolve()

  repository_root = Path(__file__).resolve().parents[2]
  return (repository_root / deps_dir).resolve()


def validate_dependency_pins(dependencies: Sequence[DependencyPin]) -> None:
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
  digest = hashlib.sha256()
  with path.open('rb') as source:
    while chunk := source.read(1024 * 1024):
      digest.update(chunk)
  return digest.hexdigest()


def verify_archive(path: Path, expected_sha256: str) -> None:
  actual_sha256 = sha256_digest(path)
  if actual_sha256 != expected_sha256.lower():
    raise ValueError(
      f'SHA-256 verification failed for {path.name}: '
      f'expected {expected_sha256.lower()}, got {actual_sha256}'
    )


def download_archive(archive: ArchivePin, downloads_dir: Path) -> Path:
  downloads_dir.mkdir(parents=True, exist_ok=True)
  destination = downloads_dir / archive.filename
  if destination.is_file():
    verify_archive(destination, archive.sha256)
    print(f'Using verified download: {destination.name}')
    return destination

  temporary = destination.with_suffix(f'{destination.suffix}.part')
  temporary.unlink(missing_ok=True)
  print(f'Downloading {archive.url}')

  try:
    with urllib.request.urlopen(archive.url) as response, temporary.open('wb') as output:
      shutil.copyfileobj(response, output)
    verify_archive(temporary, archive.sha256)
    temporary.replace(destination)
  except Exception:
    temporary.unlink(missing_ok=True)
    raise

  return destination


def extract_archive(archive_path: Path, archive: ArchivePin, sources_dir: Path) -> Path:
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
  archive_path = download_archive(dependency.archive, context.downloads_dir)
  return extract_archive(archive_path, dependency.archive, context.sources_dir)


def run_command(
  command: Sequence[str | Path], *, cwd: Path, environment: Mapping[str, str] | None = None
) -> None:
  arguments = [str(argument) for argument in command]
  print(f'Running: {shlex.join(arguments)}', flush=True)
  subprocess.run(arguments, cwd=cwd, env=environment, check=True)


def apply_patch(source_dir: Path, patch_path: Path) -> None:
  if not patch_path.is_file():
    raise ValueError(f'Patch file does not exist: {patch_path}')
  run_command(('patch', '--batch', '--forward', '-p1', '-i', patch_path.resolve()), cwd=source_dir)


def cmake_build(
  source_dir: Path,
  build_dir: Path,
  install_dir: Path,
  *,
  flags: Sequence[str] = (),
  patch_path: Path | None = None,
) -> None:
  if patch_path is not None:
    apply_patch(source_dir, patch_path)

  if build_dir.exists():
    shutil.rmtree(build_dir)
  build_dir.mkdir(parents=True)
  install_dir.mkdir(parents=True, exist_ok=True)

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
      *flags,
    ),
    cwd=source_dir,
  )
  run_command(('cmake', '--build', build_dir, '--parallel'), cwd=source_dir)
  run_command(('cmake', '--install', build_dir), cwd=source_dir)


def create_build_context(deps_dir: Path, target: str, architecture: str) -> BuildContext:
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
