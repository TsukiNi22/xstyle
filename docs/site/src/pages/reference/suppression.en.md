+++
title = "Ignoring rules and files"
summary = "Silence a rule on a line, a region, a file or a path (.xignore, .xconfig, flags), skip the files ignored by git, count what is ignored, and set the defaults with environment variables."
section = "reference"
order = 4
keywords = "ignore ignore-next ignore-file skip-file off on suppress comment xignore xconfig gitignore ignore-path exclude-from no-gitignore no-inline-ignore show-ignored add-ignore G-UNUSED-IGNORE environment variables XSTYLE_CODES XSTYLE_IGNORE XSTYLE_IGNORE_PATH XSTYLE_RTK NO_COLOR"
+++

## In a comment {#comments}

| Comment | Silences |
|---|---|
| `xstyle: ignore [CODE,...]` | this line |
| `xstyle: ignore-next [N] [CODE,...]` | the next line (the next `N` lines) |
| `xstyle: off [CODE,...]` ... `xstyle: on [CODE,...]` | the lines between (up to the end of the file without `on`; `on` alone closes every open region) |
| `xstyle: ignore-file [CODE,...]` | the whole file |
| `xstyle: skip-file` | the file is not checked nor counted |

Without a code, every rule is silenced there; a code can be a prefix or a pattern (`CPP`, `LU-*`). A reason after the codes starts with a lower case word or a parenthesis (upper case words would be read as codes). In Markdown, a directive only counts inside a `<!-- -->` comment.

```cpp "Example"
std::cout << "\033[1m"; // xstyle: ignore LU-ANSI
// xstyle: ignore-next 2 CPP-AUTO
auto lambda = [](int x) {return x * 2;};
auto other = [](int x) {return x * 3;};
// xstyle: off CPP-ONELINER-ALIGN (table aligned by hand)
// ...
// xstyle: on
// xstyle: ignore-file CPP-GUARD-MISSING (no guard on purpose)
```

A comment that silences nothing any more is reported by `G-UNUSED-IGNORE` (minor, removed by `--fix`; an `off` region loses its `on` too). It is compared to every issue of the file, so `-c`, `-s` or `--diff` never make a comment unused. The rules that are off (libutils), `R34` and the comments naming `G-UNUSED-IGNORE` are never reported; only a comment naming it silences it.

## By path: .xignore {#xignore}

A `.xignore` file in any directory of the project, read from the project root down to the file like a `.gitignore`:

- one pattern per line, `#` comments, `*` / `?` / `**` jokers;
- a pattern with a `/` is relative to the directory of the `.xignore`, else it is a name at any depth;
- `name/` matches directories only, `!pattern` includes again, the last matching line wins;
- a line ending with `:CODE,...` does not exclude the paths: it ignores these rules there.

```gitignore ".xignore"
third_party_copy/
*.gen.hpp
!keep.gen.hpp
tests/**:CPP-AUTO,G-TODO
```

The files ignored by git are skipped too (`git ls-files --others --ignored --exclude-standard`), unless given as argument; `--no-gitignore` checks them.

## Project configuration: .xconfig {#xconfig}

A `.xconfig` file (TOML), the closest one from the working directory up to the project root. Its keys give the default of the flags: the environment and the command line win, `ignore` is added to `-i`. `exclude` and `ignore-path` are patterns relative to its directory (like a `.xignore` line), the `[[path]]` tables ignore rules (`ignore`) or skip (`exclude = true`) the paths they `match`.

```toml ".xconfig"
severity = "minor"
fail-on = "major"
ignore = ["CPP-AUTO"]
lang = ["cpp", "py"]
gitignore = true
inline-ignore = true
exclude = ["third/**"]
ignore-path = ["examples/*:G-TODO"]

[[path]]
match = ["tests/**", "bench/**"]
ignore = ["CPP-FUNC-LENGTH", "CPP-UNCHECKED-RETURN"]

[[path]]
match = "generated/"
exclude = true
```

| Key | Flag |
|---|---|
| `code`, `ignore`, `severity`, `fail-on`, `lang`, `mode` | `-c`, `-i`, `-s`, `-F`, `-l`, `-m` |
| `libutils`, `top`, `link`, `header`, `header-desc`, `r34` | `-u`, `-t`, `--link`, `--header`, `--header-desc`, `--r34` |
| `gitignore = false`, `inline-ignore = false` | `--no-gitignore`, `--no-inline-ignore` |
| `exclude`, `ignore-path` | like a `.xignore` line (relative to the `.xconfig`) |

An unknown key or a wrong value stops xstyle with `<file>:<line>`.

## Flags {#flags}

| Flag | Effect |
|---|---|
| `-i CODES` | these rules ignored everywhere |
| `--ignore-path 'GLOB[:CODES];...'` | these rules ignored on these paths (relative to the working directory; no codes: every rule) |
| `-e PATTERNS`, `--exclude-from FILE` | these paths skipped (folder name, path prefix or glob) |
| `--no-gitignore` | the files ignored by git are checked too |
| `--no-inline-ignore` | audit: the comments, the `.xignore` files and the `[[path]]` tables are not read (the flags and the other keys of `.xconfig` stay) |
| `--show-ignored` | the issues ignored are listed, not only counted |
| `--add-ignore` | writes `xstyle: ignore CODE,...` on the line of every issue left |

The issues silenced by a comment, a rule by path or `-i` are **counted**: `Ignored 3 issue(s) silenced (comment 1, path 2)` in the summary, `| ignored 3 (...)` in `--rtk`, `ignored` in the JSON and Markdown reports.

## Adopting xstyle on old code {#add-ignore}

`--add-ignore` writes `xstyle: ignore CODE,...` at the end of the line of every issue left (after `-c` / `-s` / `-m` / `--diff`; `-n` previews), in the comment of the language (`//`, `#`, `--`, `;`). Never on a line ending in a string, a here-document or a `\` continuation, an empty line, a line that already has a directive, nor for a whole file issue; a comment that would silence nothing (it removes the issue itself, like a trailing whitespace) is taken back. Markdown, JSON and YAML files are left as they are.

```bash
xstyle --fix                      # what can be fixed first
xstyle --add-ignore -s major -n   # preview the comments of the major issues left
xstyle --add-ignore -s major      # write them
```

## Environment variables {#environment}

Each one gives the default of its flag; the flag given on the command line wins.

| Variable | Flag |
|---|---|
| `XSTYLE_CODES` | `-c`, `--code` |
| `XSTYLE_IGNORE` | `-i`, `--ignore` |
| `XSTYLE_IGNORE_PATH` | `--ignore-path` |
| `XSTYLE_SEVERITY` | `-s`, `--severity` |
| `XSTYLE_MODES` | `-m`, `--mode` |
| `XSTYLE_EXCLUDE` | `-e`, `--exclude` |
| `XSTYLE_LIBUTILS` | `-u`, `--libutils` |
| `XSTYLE_HEADER` | `--header` |
| `XSTYLE_REPORT` | `-o`, `--report` |
| `XSTYLE_LINK` | `--link` |
| `XSTYLE_RTK=1` | `--rtk` |
| `XSTYLE_R34=1` | `--r34` |
| `NO_COLOR` | `--no-color` |

| Other variable | Effect |
|---|---|
| `XSTYLE_LIBUTILS_SKILL` | folder of the libutils skill read by `--libutils-check` |
| `XSTYLE_HOOK=0` | the [pre-commit hook](page:git-and-ci#hook) does nothing |
| `XSTYLE_BIN` | the binary run by the pre-commit hook |

Every flag: [the options](page:options#selection).
