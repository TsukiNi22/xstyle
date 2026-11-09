+++
title = "How the fix loop works"
summary = "Why one --fix can apply fixes that no check showed, and why it never loops forever."
section = "concepts"
order = 2
kind = "explain"
level = "simple"
keywords = "fix loop pass passes overlap dry-run FIX_PASSES revealed issue chain"
+++

:::simple tldr
**In one sentence:** `--fix` checks, fixes, then checks again, because a fix can reveal a new issue, until nothing is left to fix.
:::

:::technical tldr
**Definition:** `Checker::fix` runs at most `FIX_PASSES` (32) passes; each pass re-runs every rule, keeps the allowed fixes whose line ranges do not overlap, applies them, and stops at the first pass with no fix.
:::

## The idea {#idea}

:::simple
Like tidying a room: you put the books on the shelf, and then you see the shelf needs a label. You tidy, look again, tidy again, until the room is clean.
:::

:::technical
A fix changes the file, so the issues computed before it are stale. Instead of predicting the effect of a fix, xstyle recomputes: the rules are the only source of truth, and a pass only applies fixes on disjoint line ranges so that two edits never fight over the same lines. The others come back on the next pass.
:::

## How it works {#how}

:::html
<figure class="stepper" data-steps="6">
  <ol class="captions">
    <li><b>Check, pass 1.</b> The rules run on the file: one fixable issue, <code>LU-ATTRIBUTE</code> on line 4 (<code>[[nodiscard]]</code>).</li>
    <li><b>Keep.</b> Its level is <code>auto</code>, allowed by <code>--fix</code>, and it overlaps no other fix of this pass: it is kept.</li>
    <li><b>Apply.</b> Line 4 becomes <code>_nodiscard double angle(double x);</code>.</li>
    <li><b>Check, pass 2.</b> <code>_nodiscard</code> belongs to the <code>Attribute</code> section, not defined before <code>utils.hpp</code>: <code>LU-SECTION</code> appears.</li>
    <li><b>Apply.</b> <code>#define _Attribute</code> is added before the include.</li>
    <li><b>Check, pass 3.</b> No fix left: the loop stops and the file is written (with <code>-n</code>: only shown).</li>
  </ol>
  <svg viewBox="0 0 760 300" role="img" aria-label="check, keep and apply loop on a file using libutils">
    <defs><marker id="en-fx-arr" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto"><path class="arrow" d="M0 0L10 5L0 10z"/></marker></defs>
    <rect id="en-fx-check" class="box" x="90" y="30" width="170" height="50" rx="8" data-hl="0,3,5"/><text class="lbl" x="175" y="60" text-anchor="middle">check</text>
    <rect id="en-fx-keep" class="box" x="90" y="125" width="170" height="50" rx="8" data-hl="1"/><text class="lbl" x="175" y="155" text-anchor="middle">keep</text>
    <rect id="en-fx-apply" class="box" x="90" y="220" width="170" height="50" rx="8" data-hl="2,4"/><text class="lbl" x="175" y="250" text-anchor="middle">apply</text>
    <path class="edge" data-from="en-fx-check:bottom" data-to="en-fx-keep:top" marker-end="url(#en-fx-arr)"/>
    <path class="edge" data-from="en-fx-keep:bottom" data-to="en-fx-apply:top" marker-end="url(#en-fx-arr)"/>
    <path id="en-fx-loop" class="edge-accent" data-from="en-fx-apply:left" data-to="en-fx-check:left" data-bend="-0.6" marker-end="url(#en-fx-arr)"/>
    <rect id="en-fx-code" class="group" x="300" y="30" width="430" height="180" rx="8"/>
    <text class="lbl" x="320" y="62" style="font-family:ui-monospace,monospace">#define _Math</text>
    <text class="lbl" x="320" y="92" style="font-family:ui-monospace,monospace" data-show="4-" data-hl="4">#define _Attribute</text>
    <text class="lbl" x="320" y="122" style="font-family:ui-monospace,monospace">#include &lt;utils/utils.hpp&gt; // trigo</text>
    <text class="lbl" x="320" y="182" style="font-family:ui-monospace,monospace" data-show="0-1" data-warn="0,1">[[nodiscard]] double angle(double x);</text>
    <text class="lbl" x="320" y="182" style="font-family:ui-monospace,monospace" data-show="2-" data-hl="2">_nodiscard double angle(double x);</text>
    <text class="lbl" x="320" y="250" data-show="0-2">pass 1</text>
    <text class="lbl" x="320" y="250" data-show="3-4">pass 2</text>
    <text class="lbl" x="320" y="250" data-show="5-">pass 3</text>
    <text class="lbl" x="420" y="250" data-show="0-2">LU-ATTRIBUTE</text>
    <text class="lbl" x="420" y="250" data-show="3-4">LU-SECTION</text>
    <text class="lbl" x="420" y="250" data-show="5-">no fix left</text>
  </svg>
</figure>
:::

## The worked example {#example}

The file above, fixed with only the libutils rules:

```bash "Terminal"
xstyle --fix -n --libutils on -c LU b.cpp
```

```text "output"
Would fix:
b.cpp:4 LU-ATTRIBUTE
    - [[nodiscard]] double angle(double x);
    + _nodiscard double angle(double x);
b.cpp:2 LU-SECTION
    + #define _Attribute
```

`LU-SECTION` was not in the report before the fix: the second pass found it.

## Why it stops {#stop}

:::simple
It stops as soon as a round fixes nothing, and in any case after 32 rounds.
:::

:::technical
Three guards: a pass with no fix ends the loop; a fix whose new lines equal the old ones is dropped (a no-op would loop forever); `FIX_PASSES` (32, `include/xstyle/Checker.hpp`) bounds the rest.
:::

| Guard | Where |
|---|---|
| no fix in a pass → stop | `Checker::fix` |
| fix equal to the current lines → skipped | `Checker::fix` |
| at most 32 passes | `FIX_PASSES` |

`Checker::fix` is in `src/xstyle/Checker.cpp`, `FIX_PASSES` in `include/xstyle/Checker.hpp`.

## Pitfalls {#pitfalls}

- `-c` / `-m` limit every pass: a fix of another rule revealed by the first one is not applied (here, `-c LU-ATTRIBUTE` alone would leave the `LU-SECTION` issue).
- The counters after a `--fix -n` are the issues **left** after the fixes, not before.
- `--diff` / `--staged` filter the fixes too: a fix on an unchanged line is not applied.

## Check yourself {#quiz}

:::html
<details class="quiz"><summary>Two fixes touch line 10 in the same pass: what happens?</summary><p>The first is applied, the second is left for the next pass, computed again on the new line.</p></details>
<details class="quiz"><summary>Why can <code>--fix</code> report a rule the check did not show?</summary><p>A fix revealed it: it only exists once the first fix is written.</p></details>
:::

## Glossary {#glossary}

| Term | Meaning |
|---|---|
| Pass | one run of every rule followed by the fixes it allows |
| Overlap | two fixes on the same lines: only one per pass |
| Dry run | `-n`: everything computed, nothing written |
