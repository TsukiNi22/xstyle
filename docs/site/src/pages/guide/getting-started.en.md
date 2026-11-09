+++
title = "Getting started"
summary = "Install xstyle from its packages or its sources, then run the first check."
section = "guide"
order = 1
keywords = "install setup dnf apt pacman my-universe packages first run build cmake"
+++

xstyle is one binary, `xstyle`, with no configuration file: install it, run it in a project.

## Install from the packages {#packages}

The packages are RPM (Fedora), DEB (Debian / Ubuntu) and Arch (pacman), signed with the key `RPM-GPG-KEY-tsukini`.
Every package repository of TsukiNi22 at once, with [my-universe](https://tsukini22.github.io/my-universe/):

```bash "Terminal"
curl -fsSL https://tsukini22.github.io/my-universe/install.sh | bash -s
sudo dnf install xstyle          # or: sudo apt install xstyle / sudo pacman -S xstyle
```

Or only the repository of xstyle, with its `setup.sh` (no clone needed):

```bash "Terminal"
curl -fsSL https://raw.githubusercontent.com/TsukiNi22/xstyle/main/setup.sh | bash -s -- --install
```

| Argument of `setup.sh` | Effect |
|---|---|
| `--install` | Also install the package (by default only the repository and the GPG key are set up) |
| `--pre` | The pre-release package (`xstyle-pre`) |
| `--no-sudo` | Run without `sudo` (already root) |

> [!NOTE]
> An `invalid hash` error of `dnf` / `apt` is fixed by cleaning the cache of the tool, or by running `setup.sh` again.

## Build from the sources {#sources}

Needs `clang++` (C++20), `cmake >= 3.20` and libutils `>= 4.1.0` (`>= 4.2.0` on Arch).

```bash "Terminal"
git clone https://github.com/TsukiNi22/xstyle.git && cd xstyle
cmake -S . -B build -DCMAKE_BUILD_TYPE=Optimized
cmake --build build --parallel $(nproc)                       # ./xstyle at the root
sudo cmake --build build --target install --parallel $(nproc) # /usr/local/bin/xstyle + the completions
cmake --install build --prefix ~/.local                       # or without root: ~/.local/bin/xstyle
```

## First check {#first-run}

1. Go to a project: `cd my-project`.
2. Run `xstyle`: every known file of the current directory is checked, recursively.
3. Read the summary at the end: the counters by severity and by rule.
4. Fix what is safe: `xstyle --fix`, then review it with `git diff`.

```bash "Terminal"
xstyle            # check everything
xstyle -S         # only the summary
xstyle --fix      # apply the safe fixes
```

> [!TIP]
> The first run also shows one **mandatory** reminder, `R34`: it is never counted. [What it is and how to hide it](page:severities#mandatory).

Next: [everyday use](page:everyday-use#check).
