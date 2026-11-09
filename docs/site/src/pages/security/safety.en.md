+++
title = "What xstyle reads, writes and runs"
summary = "Every file xstyle may change, every command it starts, and what it never does."
section = "security"
order = 1
keywords = "security safety write files fix git commit network report R34.md read only dry-run"
+++

xstyle is a local tool: it reads source files, and changes them only when asked.

## What it reads {#reads}

| Read | Why |
|---|---|
| the files given (or found under the paths) | the check itself |
| the root `CMakeLists.txt`, the includes | is libutils used, which sections |
| the installed libutils headers | the `LU-*` rules and the migration names |
| `VERSION.md` of the libutils skill, the libutils clone (`$LIBUTILS`, else `~/personal_delivery/cpp/libutils`) | only `--libutils-check` |

## What it writes {#writes}

Without `--fix` and `-o`, xstyle writes nothing.

| Written | When |
|---|---|
| the checked files | `--fix` only, never with `-n` / `--dry-run` |
| the report file | `-o <file>` (`.txt`, `.md`, `.json`) |
| `R34.md` in the working directory | `--fix --force`, never over an existing file |
| a git commit | `--fix --commit` / `--commit-all` only |

## What it runs {#runs}

Only `git`, in the repository of the checked files, started directly (no shell):

| Command | For |
|---|---|
| `git rev-parse --show-toplevel` | find the repository |
| `git diff -U0 --no-color --no-ext-diff --no-renames`, `git ls-files --others --exclude-standard` | `--diff`, `--diff-ref`, `--staged` |
| `git status --porcelain` | the files already changed before `--commit` |
| `git add`, `git commit -q -m <message>` | `--commit`, `--commit-all` |
| `git rev-list --count` in the libutils clone | `--libutils-check` |

No network access, no shell command built from a file, no code of the checked project executed.

## Fixes that change behavior {#fixes}

> [!WARNING]
> `--force` and `--dangerous-force` can change what the code does: `set -euo pipefail` stops a script at the first failing command, `encoding="utf-8"` makes a file in another encoding fail, a guessed type can be wrong. Preview them with `-n`, then review the result with `git diff`.

`--commit` never takes a file that had changes before the fixes: its own changes would end up in the commit unseen.
