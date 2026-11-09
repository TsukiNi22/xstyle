+++
title = "Everyday use"
summary = "Check some paths, keep only what matters, fix, write a report, use it from an AI."
section = "guide"
order = 2
keywords = "check paths filter severity code ignore lang fix dry-run force report json markdown rtk ai link"
+++

The commands used every day. Every flag is in [the options](page:options#selection).

## Check {#check}

```bash "Terminal"
xstyle                        # the current directory, recursive, every known language
xstyle src/core/Engine.cpp    # one file
xstyle -r src include         # these directories (recursive with -r)
xstyle -e build,*.gen.hpp     # skip more paths (folder name, path prefix or glob)
```

With no path the current directory is scanned recursively. Hidden folders, `build*`, `cmake-build-*`, `third_party`, `extern`, `vendor`, `node_modules` and the `generated_*` files are always skipped.

Each issue is printed with its file, line and column (a terminal link), the rule broken, its [fix level](page:severities#fix-levels) and the fix proposed; the summary counts the issues by severity and by rule.

## Keep only what matters {#filter}

| Command | Keeps |
|---|---|
| `xstyle -s major` | the `major` and `unforgivable` issues |
| `xstyle -c CPP-THIS,LU` | these rules (code, prefix or pattern: `CPP`, `LU-*`, `G-TAB`) |
| `xstyle -i G-TODO,CPP-AUTO` | every rule except these |
| `xstyle -l cpp,rs` | the C++ and Rust files (`-t`: only the most used language) |
| `xstyle -m force,manual` | the issues of these fix levels |
| `xstyle -S` | only the summary |

## Fix {#fix}

```bash "Terminal"
xstyle --fix                           # every safe fix ([auto-fix])
xstyle --fix CPP-NULL,G-TRAILING src   # only these rules, only in src
xstyle -f -n -c CPP -r src/core        # preview (dry run) of the C++ fixes of one directory
xstyle --fix --force                   # also the [force-fix] ones: check with -n first
xstyle --fix --dangerous-force         # also the [danger-fix] ones: guessed, can be wrong
xstyle --fix --header default          # missing file headers with the XARTANIA banner, no question
```

The logic of the code is never touched; review the result with `git diff`. [How the fix loop works](page:fix-loop#idea).

## Reports {#report}

```bash "Terminal"
xstyle -o report.md             # .txt, .md or .json, nothing in the terminal
xstyle -s major -o report.json  # major and above, for a script
```

`--format text|md|json` forces the format when the extension does not tell it.

## From an AI {#rtk}

`xstyle --rtk` (or `XSTYLE_RTK=1`) prints what an assistant needs and nothing else: about 5 times fewer tokens.

```text "xstyle --rtk --r34 -c CPP-THIS a.hpp"
# >file then line:col CODE severity(U/M/m/n) fix(a=auto f=--force d=--dangerous-force q=asked -=manual) ...
>a.hpp
3:33 CPP-THIS m a
* CPP-THIS Member accessed without this-> => int get(void) const {return this->_name;};
= 1 issues in 1 files: U0 M0 m1 n0 | fix a1 f0 d0 q0 -0 | xstyle --fix
```

A legend line, `>file` once, one `line:col CODE severity fix` line per issue, the message and fix of each rule once in a `* CODE` line, the summary on one line. No color, link, source line nor table.

## Links and colors {#links}

| Flag | Effect |
|---|---|
| `--link file` | `file://` links on the locations (default) |
| `--link vscode` | `vscode://` links: open the file at the line in VS Code |
| `--link none` | no link |
| `--no-color` | plain text (also with `NO_COLOR`, or when the output is not a terminal) |
