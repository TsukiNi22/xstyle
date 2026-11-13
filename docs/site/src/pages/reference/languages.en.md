+++
title = "Languages"
summary = "Which files xstyle checks, and which rules each language gets."
section = "reference"
order = 3
keywords = "languages files extensions cpp c asm nasm assembly gas python shell rust lua javascript typescript cmake makefile yaml json markdown templates"
+++

## Files and rules {#files}

| Language | Files | Rules |
|---|---|---|
| C++ | `.cpp .cc .cxx .hpp .hh .hxx .ipp .tpp .inl .h` | generic + `CPP-*` + `LU-*` ([libutils installed and used](page:libutils-rules#when)) |
| C | `.c` (+ `.h` of a project without C++ sources) | generic + `C-*`: the layout rules shared with the C++ under their own `C-` code, then the C rules (names, pointer checks, length, nesting) |
| Assembly (NASM x86-64) | `.asm .nasm .s .S` | generic + `ASM-*` (a GAS file, AT&T or `.intel_syntax`: `ASM-HEADER` and the generic rules only) |
| Rust | `.rs` | generic + `RS-*` |
| Python | `.py` | generic + `PY-*` |
| Shell | `.sh .bash` | generic + `SH-*` |
| Lua, JS / TS, CMake | `.lua`, `.js .mjs .ts .tsx .jsx`, `CMakeLists.txt .cmake` | generic (+ `CMAKE-*` for CMake) |
| Makefile, YAML | `Makefile .mk`, `.yml .yaml` | generic (no indentation by 4, Makefile recipe tabs allowed) |
| JSON, Markdown | `.json`, `.md` | generic (Markdown: trailing spaces allowed) |

Any other file is skipped when a directory is scanned; given explicitly, it gets the basic generic rules. `-l cpp,c,asm,py,sh,rs,lua,js,cmake,make,yaml,json,md` keeps some languages, `-t` only the most used one.

## Special cases {#special}

- **C headers**: a `.h` is C when the whole project (its git root, else the working directory) has `.c` sources and no C++ ones, even when the header is checked alone.
- **Ignored result** (`CPP-UNCHECKED-RETURN` / `C-UNCHECKED-RETURN`): the free functions declared in the project are read from its sources once per run, plus a libc / POSIX list; `(void)f(x);` ignores a result on purpose.
- **Assembly**: a local label (`.loop`) opens a block up to its last backward jump (else up to the next label), its content one level more; a function is a code label that is not only jumped to; the static functions (not `global`) start with `_`.
- **Rust tests**: the `#[cfg(test)]` module, `tests/`, `benches/` and `examples/` may `unwrap` and `panic`.
- **Templates**: a file with `{{NAME}}` placeholders is not valid code yet, only the generic rules are checked there.
- **Skipped paths**: hidden folders, `build*`, `cmake-build-*`, `third_party`, `extern`, `vendor`, `node_modules` and the `generated_*` files; `-e` adds more.

The rules of each family are listed in [the rules](page:rules#generic).
