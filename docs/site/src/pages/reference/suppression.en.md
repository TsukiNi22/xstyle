+++
title = "Suppression and environment"
summary = "Silence a rule on a line, the next line or a whole file, and set the defaults with environment variables."
section = "reference"
order = 4
keywords = "ignore ignore-next ignore-file suppress comment environment variables XSTYLE_CODES XSTYLE_IGNORE XSTYLE_RTK NO_COLOR"
+++

## In a comment {#comments}

| Comment | Silences |
|---|---|
| `xstyle: ignore [CODE,...]` | this line |
| `xstyle: ignore-next [CODE,...]` | the next line |
| `xstyle: ignore-file [CODE,...]` | the whole file |

Without a code, every rule is silenced there; a code can be a prefix or a pattern (`CPP`, `LU-*`). A reason after the codes starts with a lower case word or a parenthesis (upper case words would be read as codes).

```cpp "Example"
std::cout << "\033[1m"; // xstyle: ignore LU-ANSI
// xstyle: ignore-next CPP-AUTO
auto lambda = [](int x) {return x * 2;};
// xstyle: ignore-file CPP-GUARD-MISSING (no guard on purpose)
```

## Environment variables {#environment}

Each one gives the default of its flag; the flag given on the command line wins.

| Variable | Flag |
|---|---|
| `XSTYLE_CODES` | `-c`, `--code` |
| `XSTYLE_IGNORE` | `-i`, `--ignore` |
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
