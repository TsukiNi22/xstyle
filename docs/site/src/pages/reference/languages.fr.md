+++
title = "Langages"
summary = "Quels fichiers xstyle vérifie, et quelles règles chaque langage reçoit."
section = "reference"
order = 3
keywords = "langages fichiers extensions cpp c python shell rust lua javascript typescript cmake makefile yaml json markdown templates"
+++

## Fichiers et règles {#files}

| Langage | Fichiers | Règles |
|---|---|---|
| C++ | `.cpp .cc .cxx .hpp .hh .hxx .ipp .tpp .inl .h` | génériques + `CPP-*` + `LU-*` ([libutils installée et utilisée](page:libutils-rules#when)) |
| C | `.c` (+ `.h` sans C++ dans le projet) | génériques + les règles `CPP-*` marquées `c` |
| Rust | `.rs` | génériques + `RS-*` |
| Python | `.py` | génériques + `PY-*` |
| Shell | `.sh .bash` | génériques + `SH-*` |
| Lua, JS / TS, CMake | `.lua`, `.js .mjs .ts .tsx .jsx`, `CMakeLists.txt .cmake` | génériques (+ `CMAKE-*` pour CMake) |
| Makefile, YAML | `Makefile .mk`, `.yml .yaml` | génériques (pas d'indentation par 4, tabulations des recettes Makefile permises) |
| JSON, Markdown | `.json`, `.md` | génériques (Markdown : espaces en fin de ligne permis) |

Tout autre fichier est ignoré quand un dossier est parcouru ; donné explicitement, il reçoit les règles génériques de base. `-l cpp,c,py,sh,rs,lua,js,cmake,make,yaml,json,md` garde certains langages, `-t` seulement le plus présent.

## Cas particuliers {#special}

- **Tests Rust** : le module `#[cfg(test)]`, `tests/`, `benches/` et `examples/` peuvent faire `unwrap` et `panic`.
- **Templates** : un fichier avec des placeholders `{{NAME}}` n'est pas encore du code valide, seules les règles génériques y sont vérifiées.
- **Chemins ignorés** : dossiers cachés, `build*`, `cmake-build-*`, `third_party`, `extern`, `vendor`, `node_modules` et les fichiers `generated_*` ; `-e` en ajoute.

Les règles de chaque famille sont dans [les règles](page:rules#generic).
