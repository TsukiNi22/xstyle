+++
title = "Severities and fix levels"
summary = "How bad an issue is, who can fix it, and what is counted in the exit status."
section = "concepts"
order = 1
kind = "explain"
level = "simple"
keywords = "severity unforgivable major minor negligible mandatory r34 fix level auto force danger ask manual fail-on exit"
+++

:::simple tldr
**In one sentence:** every issue has a **severity** (how bad it is) and a **fix level** (who may fix it): the first decides if the run fails, the second what `--fix` is allowed to change.
:::

:::technical tldr
**Definition:** two independent attributes per rule, read from the rule table (`xstyle --list-rules`): `Severity` ∈ {Unforgivable, Major, Minor, Negligible, Mandatory} drives the counters, `--severity` and `--fail-on`; `FixMode` ∈ {Auto, Force, Dangerous, Ask, Manual} drives which fixes `--fix` applies.
:::

## The idea {#idea}

:::simple
Think of a teacher correcting a copy. Some mistakes cost the grade (a wrong answer), some only cost a point (a missing accent). And some the teacher fixes in red, while others are only circled: you must rewrite them yourself.
xstyle does the same: the **severity** is the cost of the mistake, the **fix level** says if xstyle writes the correction or only circles it.
:::

:::technical
Severity and fix level are orthogonal: `CPP-USING-NAMESPACE` is `unforgivable` and `manual`, `G-TRAILING` is `minor` and `auto`. The severity filters the report (`-s`) and the exit status (`--fail-on`); the fix level filters what `--fix` touches (`--force`, `--dangerous-force`) and can filter the report too (`-m`).
:::

## The severity scale {#scale}

| Severity | Letter in `--rtk` | Example |
|---|---|---|
| `unforgivable` | `U` | `CPP-USING-NAMESPACE`: `using namespace` |
| `major` | `M` | `CPP-NULL`: `NULL` instead of `nullptr` |
| `minor` | `m` | `G-TRAILING`: trailing whitespace |
| `negligible` | `n` | `G-EOF-NEWLINE`: no newline at the end of the file |

:::simple
`-s major` shows only the two worst levels. `--fail-on major` makes the run fail only when one of them is left.
:::

:::technical
`-s <level>` is the minimal severity reported (default `negligible`); `-F` / `--fail-on <level>` the minimal severity that makes the exit status `1` (default `negligible`: any issue). Both are cumulative upwards: `major` includes `unforgivable`.
:::

## The mandatory rule R34 {#mandatory}

`R34` is the only rule of the severity `mandatory`, outside the scale: *Rule 34: if it exists, there is porn of it. No exceptions*. It is a reminder, not a style rule.

| | R34 |
|---|---|
| Displayed | once per run, with a link to the [article](https://en.wikipedia.org/wiki/Rule_34); a `! R34 ...` line in `--rtk` |
| Counted | never: not in the totals, the exit status, `--fail-on` nor the hook |
| Hidden by | the integer `34` or `r34` (any case) in a checked file, a file `R34.md` in the directory of the run, `--r34`, `XSTYLE_R34=1` |
| Fix | `xstyle --fix --force` writes `R34.md` (never replaces an existing one) |

## Fix levels {#fix-levels}

| Tag | Fixed by | Meaning |
|---|---|---|
| `[auto-fix]` | `--fix` | safe, the behavior can't change: spaces, includes order, `nullptr`... |
| `[force-fix]` | `--fix --force` | can change the behavior: `set -euo pipefail`, `encoding="utf-8"`, a comment replaced |
| `[danger-fix]` | `--fix --dangerous-force` | guessed, can be wrong: Python types from the usage, `static_cast` of a C pointer cast |
| `[ask-fix]` | `--fix` + a question | the missing file header: banner and description asked per file (`--header` answers) |
| `[manual]` | by hand | a decision a tool can't take: a type, a name, a rewrite with libutils |

:::simple
Start with `--fix` only. Look at what `--force` would do with `-n` (nothing is written), then decide.
:::

:::technical
`--dangerous-force` implies `--force`. Without a terminal (CI) the `[ask-fix]` header is skipped unless `--header none|default|<text>` is given. `-m auto,force,dangerous,ask,manual` keeps only these levels in the display, the counters, the report and `--fix` (`xstyle --fix --force -m auto` applies only the safe fixes).
:::

## Pitfalls {#pitfalls}

- `-s` hides issues but does not change the exit status: use `--fail-on` for that.
- `-s` never hides R34; `-c` / `-i` / `-m` select it like any other rule.
- A `[force-fix]` can change what a script does: `set -euo pipefail` stops it at the first failing command.

## Check yourself {#quiz}

:::html
<details class="quiz"><summary>Does <code>xstyle -s major</code> exit with 0 when only minor issues are left?</summary><p>No: <code>-s</code> only hides them, the exit status still counts them. <code>--fail-on major</code> gives 0.</p></details>
<details class="quiz"><summary>Which flag applies the guessed Python type hints?</summary><p><code>--fix --dangerous-force</code>: they are <code>[danger-fix]</code>.</p></details>
<details class="quiz"><summary>Can R34 make a pre-commit hook refuse a commit?</summary><p>No, a mandatory rule is never counted.</p></details>
:::

## Glossary {#glossary}

| Term | Meaning |
|---|---|
| Severity | how bad an issue is, from `negligible` to `unforgivable` |
| Fix level | who may fix it: `--fix`, `--force`, `--dangerous-force`, a question, or you |
| `--fail-on` | the severity from which the exit status is `1` |
| Mandatory | displayed, never counted (only R34) |

Source: `include/xstyle/XStyleType.hpp` (`Severity`, `FixMode`) and `src/xstyle/Rules.cpp` of the repository.
