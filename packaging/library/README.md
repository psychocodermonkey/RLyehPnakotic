# R'Lyeh Pnakotic library

This ZIP contains the header-only C++20 Pnakotic library. Its public release version and source commit are recorded in
`VERSION.txt`. No Git checkout, Qt installation, or Pnakotic development tools are needed to use the library.

## Vendoring with CMake

Add the extracted directory to your project and link the interface target:

```cmake
add_subdirectory(path/to/RLyehPnakotic)
target_link_libraries(your_target PRIVATE RLyeh::Pnakotic)
```

## Installing with CMake

Configure and install the extracted directory using CMake 3.21 or newer:

```sh
cmake -S path/to/RLyehPnakotic -B path/to/build
cmake --install path/to/build --prefix path/to/install --component RLyehPnakotic
```

Point your consumer's `CMAKE_PREFIX_PATH` at that install prefix, then use:

```cmake
find_package(RLyehPnakotic CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE RLyeh::Pnakotic)
```

Include `<RLyehPnakotic/Pnakotic.h>` for the codec and project-policy APIs. Consumers supply a `YYYY-MM-DD` calendar
date and a full hexadecimal commit hash; the library does not inspect Git or normalize timezones. See
`docs/version-format.md` for the format and date-convention details.

The software is licensed under GPL-3.0-only; see `LICENSE`. The four public headers contain their own copyright notices.
