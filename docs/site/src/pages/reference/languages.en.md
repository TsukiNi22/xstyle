+++
title = "Languages"
summary = "Which files xstyle checks, and which rules each language gets."
section = "reference"
order = 3
keywords = "languages files extensions cpp c python shell rust lua javascript typescript cmake makefile yaml json markdown templates"
+++

## Files and rules {#files}

| Language | Files | Rules |
|---|---|---|
| C++ | `.cpp .cc .cxx .hpp .hh .hxx .ipp .tpp .inl .h` | generic + `CPP-*` + `LU-*` ([libutils installed and used](page:libutils-rules#when)) |
| C | `.c` (+ `.h` without C++ in the project) | generic + the `CPP-*` rules marked `c` |
| Rust | `.rs` | generic + `RS-*` |
| Python | `.py` | generic + `PY-*` |
| Shell | `.sh .bash` | generic + `SH-*` |
| Lua, JS / TS, CMake | `.lua`, `.js .mjs .ts .tsx .jsx`, `CMakeLists.txt .cmake` | generic (+ `CMAKE-*` for CMake) |
| Makefile, YAML | `Makefile .mk`, `.yml .yaml` | generic (no indentation by 4, Makefile recipe tabs allowed) |
| JSON, Markdown | `.json`, `.md` | generic (Markdown: trailing spaces allowed) |

Any other file is skipped when a directory is scanned; given explicitly, it gets the basic generic rules. `-l cpp,c,py,sh,rs,lua,js,cmake,make,yaml,json,md` keeps some languages, `-t` only the most used one.

## Special cases {#special}

- **Rust tests**: the `#[cfg(test)]` module, `tests/`, `benches/` and `examples/` may `unwrap` and `panic`.
- **Templates**: a file with `{{NAME}}` placeholders is not valid code yet, only the generic rules are checked there.
- **Skipped paths**: hidden folders, `build*`, `cmake-build-*`, `third_party`, `extern`, `vendor`, `node_modules` and the `generated_*` files; `-e` adds more.

The rules of each family are listed in [the rules](page:rules#generic).
