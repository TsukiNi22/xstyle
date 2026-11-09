+++
title = "Development"
summary = "Build modes, tests, the layout of the sources, the workflows and how a release is made."
section = "resources"
order = 2
keywords = "development build debug asan optimized tests ctest gtest sources layout workflows release tag pre-release docs site"
+++

## Build {#build}

```bash "Terminal"
make                                                 # configure & build into build/
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug         # debug build (-g -ggdb3)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Asan          # AddressSanitizer build
cmake --build build --target release                 # Debug, Asan & Optimized builds
cmake --build build --target package_release         # RPM / DEB / Arch packages of every variant
cmake --build build --target get_unregistered_files  # .cpp files missing from SRC / SRC_SHARED
./xstyle --rtk -r src include tests CMakeLists.txt   # xstyle stays clean on itself (checked by the CI)
```

> [!NOTE]
> Every new `.cpp` goes in `SRC` (executable only) or `SRC_SHARED` (executable and tests) of the `CMakeLists.txt`.

## Tests {#tests}

```bash "Terminal"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build --parallel $(nproc)   # ./unit_tests at the root
(cd build && ctest --output-on-failure)   # GoogleTest + command line tests
```

## Sources {#layout}

| Path | Content |
|---|---|
| `src/xstyle/` | `main.cpp` (entry), `Core.cpp` (arguments, run), `Checker.cpp` (rules and fix loop), `Reporter.cpp`, `Rules.cpp` (rule table), `check/` (one file per family) |
| `include/xstyle/` | headers |
| `assets/fonts/ansi_shadow.flf` | banner font of the file headers, embedded at build time |
| `tests/xstyle/` | unit tests (GoogleTest) and command line tests (`ctest`) |
| `docs/site/` | sources of this site (Markdown + `gen/` generators) |

## This site {#docs}

```bash "Terminal"
make                                                                     # gen/ reads ./xstyle
python3 ~/.claude/skills/better-html-doc/scripts/build_site.py --site docs/site --out docs
```

The pages are written in `docs/` and published on `gh-pages` by `Docs - Pages (CD)` at the next push. The rules and options pages are generated from `./xstyle --list-rules` and `./xstyle --help`.

## Workflows and releases {#workflows}

| Workflow | Does |
|---|---|
| `Dispatch (CI/CD)` | on every push of `main` or a tag `v*` (not `*.md` / `*.txt` / `docs/` only): decides what runs |
| `Unit Tests (CI)` | builds and runs the tests, always first |
| `Build (CI)` | build check and xstyle on its own sources (`--fail-on major`), when no package is built |
| `Build - Packages (CI/CD)` | RPM / DEB / Arch packages, signed and published on `gh-pages` |
| `Release - GitHub (CD)` | the GitHub release of a tag, from its section of `CHANGELOG.md` |
| `Docs - Pages (CD)` | copies `docs/` on `gh-pages` |

A tag `vX.Y.Z` (its `## vX.Y.Z - date` section must exist in the changelog) builds the stable packages and the release; a tag `vX.Y.Z-<suffix>` or a commit with `[build]` the `-pre` packages; `[ignore]` skips the CI.
