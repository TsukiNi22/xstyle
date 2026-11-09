+++
title = "FAQ"
summary = "Short answers to the usual questions about xstyle."
section = "resources"
order = 3
keywords = "faq questions r34 configuration file windows exit status libutils rules off"
+++

## Is there a configuration file? {#config}

No. The defaults come from the [environment variables](page:suppression#environment), the exceptions from [comments in the code](page:suppression#comments).

## What is R34, and how do I hide it? {#r34}

A mandatory reminder, never counted. `--r34`, `XSTYLE_R34=1`, a file `R34.md`, or `34` / `r34` in a checked file hide it. [Details](page:severities#mandatory).

## Why are the libutils rules off? {#libutils}

They need libutils installed **and** used by the project (`find_package(utils)`, `utils::utils` or `utils.hpp`). `--libutils on` forces them. [Details](page:libutils-rules#when).

## Why does a run exit with 1 when I only see minor issues? {#exit}

`-s` only hides issues. The exit status follows `--fail-on` (default: any issue). [Details](page:severities#scale).

## Does it work on Windows? {#windows}

No: xstyle uses POSIX calls and is built for Linux only (RPM, DEB, Arch). It checks Windows code (line endings `G-CRLF` included) from Linux.

## How do I check only my changes? {#changes}

`xstyle --diff` (since `HEAD`), `--diff-ref main`, or `--staged`. [Details](page:git-and-ci#diff).
