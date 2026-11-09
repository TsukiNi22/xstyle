+++
title = "Git, hooks and CI"
summary = "Check only the changed lines, commit the fixes, refuse a bad commit with the pre-commit hook, fail a CI job."
section = "guide"
order = 3
keywords = "git diff staged diff-ref commit commit-all pre-commit hook githooks ci workflow fail-on exit status"
+++

xstyle reads git to check only what changed, and its exit status stops a commit or a CI job.

## Only the changed lines {#diff}

| Command | Checks |
|---|---|
| `xstyle --diff` | the lines changed since `HEAD` (staged and not staged, new files included) |
| `xstyle --diff-ref main` | the lines changed since a branch, tag or commit |
| `xstyle --staged` | only the staged lines (`git diff --cached`): the mode of the pre-commit hook |

With no path these modes take the changed files themselves. An issue about the whole file is kept when the file changed; `--fix` follows the same filter.

## Commit the fixes {#commit}

```bash "Terminal"
xstyle --fix --commit                                  # fix, then commit the fixed files
xstyle --fix --commit-all --message "chore(core): ..." # fix, then commit every tracked change with them
```

The default message is `chore(style): apply the xstyle fixes (N issues)`, with one line per rule. `--commit` only takes the files that had **no** change before the fixes (the others are listed and left out); `--commit-all` takes every tracked change too. Never with `--dry-run`.

## Pre-commit hook {#hook}

The hook of the repository (`.githooks/pre-commit`) runs xstyle on the staged lines at each `git commit` and refuses a `major` (or worse) issue.

```bash "Terminal"
git config core.hooksPath .githooks   # once per clone
git commit --no-verify                # skip it once (or XSTYLE_HOOK=0 git commit ...)
xstyle --fix --staged                 # fix what can be, then git add -u
```

| Setting at the top of the hook | Values (default first) |
|---|---|
| `SCOPE` | `staged` (staged lines) · `files` (whole staged files) · `all` (whole project) |
| `FAIL_ON` | `major` · `unforgivable` · `minor` · `negligible` · `never` |
| `WARN_ON` | `minor` · `negligible` · `none` |
| `FIX` | `none` · `safe` (auto fixes of the fully staged files, added again before the check) |
| `MISSING` | `warn` · `fail` · `ignore` (xstyle not installed) |
| `OUTPUT` | `auto` (compact under Claude Code / `XSTYLE_RTK`) · `human` · `rtk` |

> [!TIP]
> In another repository the hook is installed by the `pre-commit` skill, or by the `setup.sh` of [ai-utils](https://github.com/TsukiNi22/ai-utils) (`hook`); a hook already there is kept and run first.

## In a CI job {#ci}

```yaml "workflow step"
- name: Style check
  run: xstyle -r src include --fail-on major --r34 --rtk
```

The exit status is `0` when clean, `1` when an issue at or above `--fail-on` is left, `255` on an error. On a pull request, `--diff-ref origin/<base>` checks only its lines. The `Build (CI)` workflow of xstyle runs it on its own sources this way.
