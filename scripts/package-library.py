#!/usr/bin/env python3
"""Build and validate a library-only Pnakotic ZIP from the current checkout."""

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import zipfile


REPOSITORY = Path(__file__).resolve().parent.parent
PACKAGE_FILES = {
    "packaging/library/CMakeLists.txt": "CMakeLists.txt",
    "packaging/library/README.md": "README.md",
    "LICENSE": "LICENSE",
    "cmake/RLyehPnakoticLibrary.cmake": "cmake/RLyehPnakoticLibrary.cmake",
    "cmake/RLyehPnakoticConfig.cmake.in": "cmake/RLyehPnakoticConfig.cmake.in",
    "docs/version-format.md": "docs/version-format.md",
    "include/RLyehPnakotic/Codec.h": "include/RLyehPnakotic/Codec.h",
    "include/RLyehPnakotic/Pnakotic.h": "include/RLyehPnakotic/Pnakotic.h",
    "include/RLyehPnakotic/ProjectPolicy.h": "include/RLyehPnakotic/ProjectPolicy.h",
    "include/RLyehPnakotic/Version.h": "include/RLyehPnakotic/Version.h",
}

CONSUMER_SOURCE = """#include <RLyehPnakotic/Pnakotic.h>

using namespace RLyeh::Pnakotic;

constexpr auto codec = EncodePayload(12345);
static_assert(codec.valid());
constexpr auto codec_decoded = DecodePublicVersion(codec.version.view());
static_assert(codec_decoded.valid());
static_assert(codec_decoded.logical_payload == 12345);

constexpr auto encoded = EncodeSCMVersion(Projects::RLyehPnakotic, "2026-08-17", "546bdbb");
static_assert(encoded.valid());
static_assert(encoded.version.view() == "-267.-19.-217.169");
constexpr auto decoded = DecodeSCMVersion(Projects::RLyehPnakotic, encoded.version.view());
static_assert(decoded.valid());
static_assert(decoded.commit_date.view() == "2026-08-17");
static_assert(decoded.commit_prefix.view() == "546bdb");

int main() { return encoded.valid() && decoded.valid() ? 0 : 1; }
"""


def run(*command: str, cwd: Path | None = None) -> None:
    subprocess.run(command, cwd=cwd, check=True)


def output(*command: str, cwd: Path | None = None) -> str:
    return subprocess.check_output(command, cwd=cwd, text=True).strip()


def release_identity(build_dir: Path) -> tuple[str, str]:
    run(
        "cmake", "-S", str(REPOSITORY), "-B", str(build_dir),
        "-DCMAKE_BUILD_TYPE=Release", "-DRLYEH_PNAKOTIC_BUILD_TESTS=OFF",
        "-DRLYEH_PNAKOTIC_BUILD_APP=OFF",
    )
    run("cmake", "--build", str(build_dir), "--target", "RLyehPnakoticPackageVersion")
    version, commit = output(str(build_dir / "bin" / "RLyehPnakoticPackageVersion")).splitlines()
    if not re.fullmatch(r"-?\d+(?:\.-?\d+){3}", version):
        raise RuntimeError("The encoder did not produce a four-component public version")
    if commit != output("git", "rev-parse", "HEAD", cwd=REPOSITORY):
        raise RuntimeError("Generated version metadata does not match HEAD")
    return version, commit


def add_file(archive: zipfile.ZipFile, name: str, contents: bytes) -> None:
    info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
    info.compress_type = zipfile.ZIP_DEFLATED
    info.external_attr = 0o644 << 16
    archive.writestr(info, contents)


def make_zip(destination: Path, package_name: str, version: str, commit: str, dirty: bool) -> None:
    contents = {
        target: (REPOSITORY / source).read_bytes()
        for source, target in PACKAGE_FILES.items()
    }
    source_state = "uncommitted preview" if dirty else "clean commit"
    contents["VERSION.txt"] = (
        f"Public version: {version}\nCommit: {commit}\nSource state: {source_state}\n"
    ).encode("utf-8")

    with zipfile.ZipFile(destination, "w") as archive:
        for target, data in sorted(contents.items()):
            add_file(archive, f"{package_name}/{target}", data)


def write_consumer(directory: Path, package_dir: Path, installed: bool) -> None:
    directory.mkdir(parents=True)
    (directory / "main.cpp").write_text(CONSUMER_SOURCE, encoding="utf-8")
    package_setup = (
        "find_package(RLyehPnakotic CONFIG REQUIRED)"
        if installed else f'add_subdirectory("{package_dir.as_posix()}" pnakotic-build)'
    )
    (directory / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.21)\n"
        "project(PnakoticConsumer LANGUAGES CXX)\n"
        f"{package_setup}\n"
        "add_executable(consumer main.cpp)\n"
        "target_link_libraries(consumer PRIVATE RLyeh::Pnakotic)\n",
        encoding="utf-8",
    )


def configure_consumer(source: Path, build: Path, prefix: Path | None = None) -> None:
    command = [
        "cmake", "-S", str(source), "-B", str(build),
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "-DCMAKE_DISABLE_FIND_PACKAGE_Git=TRUE",
        "-DCMAKE_DISABLE_FIND_PACKAGE_Qt6=TRUE",
    ]
    if prefix is not None:
        command.append(f"-DCMAKE_PREFIX_PATH={prefix}")
    run(*command)
    run("cmake", "--build", str(build))
    if str(REPOSITORY) in (build / "compile_commands.json").read_text(encoding="utf-8"):
        raise RuntimeError("Consumer compilation still refers to the development checkout")
    run(str(build / "consumer"))


def validate_zip(archive_path: Path, package_name: str) -> None:
    with tempfile.TemporaryDirectory(prefix="pnakotic zip check ") as temporary:
        root = Path(temporary)
        with zipfile.ZipFile(archive_path) as archive:
            expected = {
                f"{package_name}/{name}" for name in (*PACKAGE_FILES.values(), "VERSION.txt")
            }
            if set(archive.namelist()) != expected:
                raise RuntimeError("Library ZIP contains missing or unexpected files")
            archive.extractall(root)

        package_dir = root / package_name
        if (package_dir / ".git").exists():
            raise RuntimeError("Extracted package unexpectedly contains Git metadata")

        vendor_source = root / "vendored-consumer"
        write_consumer(vendor_source, package_dir, installed=False)
        configure_consumer(vendor_source, root / "vendored-build")

        package_build = root / "package-build"
        prefix = root / "install-prefix"
        run(
            "cmake", "-S", str(package_dir), "-B", str(package_build),
            "-DCMAKE_DISABLE_FIND_PACKAGE_Git=TRUE",
            "-DCMAKE_DISABLE_FIND_PACKAGE_Qt6=TRUE",
        )
        run("cmake", "--build", str(package_build))
        run("cmake", "--install", str(package_build), "--prefix", str(prefix),
            "--component", "RLyehPnakotic")

        moved_prefix = root / "relocated-prefix"
        shutil.move(prefix, moved_prefix)
        installed_source = root / "installed-consumer"
        write_consumer(installed_source, package_dir, installed=True)
        configure_consumer(installed_source, root / "installed-build", moved_prefix)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, default=REPOSITORY / "build" / "dist")
    parser.add_argument(
        "--allow-dirty", action="store_true",
        help="create a clearly marked local preview from uncommitted sources",
    )
    args = parser.parse_args()

    dirty = bool(output("git", "status", "--porcelain=v1", "--untracked-files=all", cwd=REPOSITORY))
    if dirty and not args.allow_dirty:
        raise RuntimeError("Refusing to make a release ZIP from an uncommitted working tree")

    version, commit = release_identity(REPOSITORY / "build" / "library-package")
    package_name = f"RLyehPnakotic-{version}"
    if dirty:
        package_name += "-uncommitted"

    output_dir = args.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    destination = output_dir / f"{package_name}.zip"
    if destination.exists():
        raise RuntimeError(f"Refusing to overwrite existing ZIP: {destination}")

    with tempfile.TemporaryDirectory(prefix=".pnakotic-stage-", dir=output_dir) as temporary:
        staged_zip = Path(temporary) / destination.name
        make_zip(staged_zip, package_name, version, commit, dirty)
        validate_zip(staged_zip, package_name)
        os.replace(staged_zip, destination)

    print(f"Validated library ZIP: {destination}")


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as error:
        raise SystemExit(str(error)) from error
