# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

| Scope | Content |
| ----- | ------- |
| `In Progress` | Work that hasn't been completed yet (typically listed under [pre-release]) |
| `Added`       | New features, files, or sections introduced in this release |
| `Removed`     | Existing features, files, or sections that have been deleted |
| `Changed`     | Existing features or sections that were modified (behavior, structure, or content) |
| `Fixed`       | Bugs or issues that have been resolved |

> [!NOTE]
> The `(unofficial)` version are sub-versions (typically fixes) that don't have associated tag.
> A package built at this version is not guaranteed!

---

## [pre-release]
### Changed
- README rules table, documentation site (rules page: C++, C and assembly families; languages page: C headers, ignored results, assembly) regenerated with the C and assembly rules
- Documentation site in the better-html-doc style ([tsukini22.github.io/xstyle/docs](https://tsukini22.github.io/xstyle/docs/)): user guide, concepts (severities and fix levels, fix loop, libutils rules), options and rules generated from `xstyle --help` / `--list-rules`, security, EN / FR with search; sources in `docs/site/` (`docs/scripts/update_rules.py` removed)
- **[MAJOR]** C files no longer get `CPP-*` codes: the layout rules shared with the C++ have their own `C-` code on a C file (`C-GUARD-MISSING`, `C-PTR-REF`, `C-HEADER`... `-c C` / `-i C` select them, `xstyle: ignore C-...`), the `CPP-*` rules are C++ only

### Added
- C rules (`check/c.cpp`): `C-TYPE-NAME` (struct / union / enum `snake_case` with `_s` / `_u` / `_e`, typedef `_t`, enum values `UPPER_SNAKE`), `C-FUNC-NAME` (functions, parameters, variables in `snake_case`), `C-PTR-CHECK` (pointer parameter of a non static function dereferenced before being tested), `C-FUNC-LENGTH` (body over 40 lines of code), `C-NESTING` (more than 4 nested control blocks, a bare `{ }` not counted)
- `CPP-UNCHECKED-RETURN` / `C-UNCHECKED-RETURN`: the result of a function returning a value ignored (`f(x);` alone in its statement): the free functions declared in the project (its sources are read once per run) and a libc / POSIX list (`malloc`, `read`, `write`, `fork`, `pthread_create`...); `(void)f(x);` ignores it on purpose
- Assembly language (NASM x86-64: `.asm`, `.nasm`, `.s`, `.S`; a GAS file only gets its header and the generic rules), `-l asm`: `ASM-HEADER` / `ASM-HEADER-FILE` (the C / C++ header in `;` comments), `ASM-SECTION-ORDER`, `ASM-SYMBOL-DECL` (extern at the top, global right after `section .text`, unused / undefined symbols), `ASM-INDENT` (local blocks up to their last backward jump), `ASM-COMMENT-ALIGN`, `ASM-CASE`, `ASM-LABEL-NAME` (snake_case, static functions `_name`, global ones never), `ASM-LOCAL-LABEL`, `ASM-CONST-NAME`, `ASM-DATA-NAME`, `ASM-REG-COMMENT` (`; in: rdi, rsi | out: rax` written with `--force`), `ASM-FALLTHROUGH`, `ASM-CALLEE-SAVED` (System V), `ASM-FRAME` / `ASM-FRAME-UNUSED` (push rbp / pop rbp on every path, a frame not needed), `ASM-SYSCALL-COMMENT` (the syscall name written by `--fix`), `ASM-ZERO-XOR`

### Fixed
- A `.h` of a C project was checked as C++ when no `.c` was among the files checked (`xstyle include/`, `--staged` on a header only): the whole project is read (its git root, else the working directory) to decide whether its headers are C

## v1.0.0 - 2026-11-09
### Changed
- **[MAJOR]** Own repository [TsukiNi22/xstyle](https://github.com/TsukiNi22/xstyle), moved out of `style/xstyle` of [ai-utils](https://github.com/TsukiNi22/ai-utils) with its history (no longer built by the `setup.sh` of ai-utils)
- Set up from [cpp_project_template](https://github.com/TsukiNi22/cpp_project_template): `CMakeLists.txt` (`SRC` / `SRC_SHARED`, `release` / `install_release` / `package_release` targets, `Debug` / `Asan` / `Optimized` modes), Linux only (no Windows build: POSIX calls)
- The banner font of the file headers is part of the repository (`assets/fonts/ansi_shadow.flf`, embedded at build time) instead of being read from the `cpp-class` skill
- `--libutils-check` finds the reference of the libutils skill at run time (`$XSTYLE_LIBUTILS_SKILL`, `~/.claude/skills/libutils`, then the ai-utils clone) instead of a path fixed at build time

### Added
- RPM / DEB / Arch (pacman) packages `xstyle`, `xstyle-op`, `xstyle-db`, `xstyle-as` (and their `-pre`), signed and published on the `gh-pages` package repository, listed in [my-universe](https://tsukini22.github.io/my-universe/)
- Bash / zsh / fish completion installed by the packages and `cmake --install` (`/usr/share/bash-completion`, `zsh/site-functions`, `fish/vendor_completions.d`)
- `setup.sh`: package repository + GPG key (`--install`, `--pre`, `--no-sudo`)
- Unit tests (GoogleTest: rule catalogue, patterns, names) and command line tests (`ctest`)
- Workflows `Dispatch (CI/CD)`, `Unit Tests (CI)`, `Build (CI)` (with xstyle run on its own sources), `Build - Packages (CI/CD)`, `Release - GitHub (CD)`, `Docs - Pages (CD)`
- Documentation on GitHub Pages (`docs/`: technical page, project graph), xstyle pre-commit hook (`.githooks/pre-commit`)

## v0.4.0 - 2026-10-29 (unofficial)
### Added
- `CPP-ONELINER-ALIGN`: one-liners of a group aligned tight (their `{` right after the longest signature + 1), lengths far apart aligned by levels (auto fix)
- `R34`: Rule 34 reminder shown with the issues, never counted (`Mandatory` severity), hidden by `--r34` / `XSTYLE_R34`
- `CPP-INIT-COLON`: the `:` of an init list glued to the `)` of its constructor (auto fix), `CPP-EMPTY-BODY` joining that way
- `CPP-EMPTY-BODY`: an empty constructor / destructor / operator of a class on one line (`{ /* Nothing */ };`, auto fix)

### Changed
- libutils `v4.1.0`: the libutils checks also read the Windows build (MinGW sysroot of `mingw64-libutils`)

## v0.3.0 - 2026-10-15 (unofficial)
### Changed
- libutils `v4.0.0` required (`find_package(utils 4.0.0)`)

### Added
- `CPP-CLASS-BLOCK`: what each block of a class holds (Pre-Function / Function / Operator / Constructor / Destructor)
- `CPP-PREPRO-INDENT`: the indentation of the `#if` blocks against the code around them

## v0.2.0 - 2026-10-06 (unofficial)
### Added
- `--rtk` compact output for an AI (no color, link, source line nor table; about 5x fewer tokens)
- Changed lines only (`--diff`, `--staged`), commit of the fixes (`--commit`, `--message`), `--libutils-check`, CMake and comment rules
- Fix levels `force`, `ask` and `dangerous` (`--force`, `--dangerous-force`), `-m` / `--mode` to select the issues by fix level, file headers generated (`--header`), template files
- `PY-TYPE-HINTS` adds the hints that are certain
- `-o` writes the report into a file (text, Markdown, JSON) instead of the terminal
- Bash / zsh / fish completion (`--completion`)
- `[auto-fix]` / `[manual]` tag on every issue, Rust support

### Fixed
- `PY-OPEN-ENCODING` reads the arguments up to the matching parenthesis
- `SH-TEST` on mixed lines and test commands, here-documents ignored
- `CPP-EMPTY-SECTION` false positives

## v0.1.0 - 2026-10-06 (unofficial)
### Added
- Coding style checker / fixer (C++ first, libutils aware, Python, shell, generic rules) with severity counters, built and installed by the `setup.sh` of ai-utils

[v1.0.0]: https://github.com/TsukiNi22/xstyle/releases/tag/v1.0.0
