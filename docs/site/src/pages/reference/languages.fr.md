+++
title = "Langages"
summary = "Quels fichiers xstyle vérifie, et quelles règles chaque langage reçoit."
section = "reference"
order = 3
keywords = "langages fichiers extensions cpp c asm nasm assembleur gas python shell rust lua javascript typescript cmake makefile yaml json markdown templates"
+++

## Fichiers et règles {#files}

| Langage | Fichiers | Règles |
|---|---|---|
| C++ | `.cpp .cc .cxx .hpp .hh .hxx .ipp .tpp .inl .h` | génériques + `CPP-*` + `LU-*` ([libutils installée et utilisée](page:libutils-rules#when)) |
| C | `.c` (+ `.h` d'un projet sans sources C++) | génériques + `C-*` : les règles de mise en page partagées avec le C++ sous leur propre code `C-`, puis les règles du C (noms, pointeurs vérifiés, longueur, imbrication) |
| Assembleur (NASM x86-64) | `.asm .nasm .s .S` | génériques + `ASM-*` (un fichier GAS, AT&T ou `.intel_syntax` : `ASM-HEADER` et les génériques seulement) |
| Rust | `.rs` | génériques + `RS-*` |
| Python | `.py` | génériques + `PY-*` |
| Shell | `.sh .bash` | génériques + `SH-*` |
| Lua, JS / TS, CMake | `.lua`, `.js .mjs .ts .tsx .jsx`, `CMakeLists.txt .cmake` | génériques (+ `CMAKE-*` pour CMake) |
| Makefile, YAML | `Makefile .mk`, `.yml .yaml` | génériques (pas d'indentation par 4, tabulations des recettes Makefile permises) |
| JSON, Markdown | `.json`, `.md` | génériques (Markdown : espaces en fin de ligne permis) |

Tout autre fichier est ignoré quand un dossier est parcouru ; donné explicitement, il reçoit les règles génériques de base. `-l cpp,c,asm,py,sh,rs,lua,js,cmake,make,yaml,json,md` garde certains langages, `-t` seulement le plus présent.

## Cas particuliers {#special}

- **Headers C** : un `.h` est du C quand tout le projet (sa racine git, sinon le dossier de travail) a des sources `.c` et aucune C++, même quand le header est vérifié seul.
- **Résultat ignoré** (`CPP-UNCHECKED-RETURN` / `C-UNCHECKED-RETURN`) : les fonctions libres déclarées dans le projet sont lues dans ses sources une fois par lancement, plus une liste libc / POSIX ; `(void)f(x);` ignore un résultat volontairement.
- **Assembleur** : un label local (`.loop`) ouvre un bloc jusqu'à son dernier saut arrière (sinon jusqu'au label suivant), son contenu un niveau plus loin ; une fonction est un label de code qui n'est pas seulement une cible de saut ; les fonctions statiques (pas `global`) commencent par `_`.
- **Tests Rust** : le module `#[cfg(test)]`, `tests/`, `benches/` et `examples/` peuvent faire `unwrap` et `panic`.
- **Templates** : un fichier avec des placeholders `{{NAME}}` n'est pas encore du code valide, seules les règles génériques y sont vérifiées.
- **Chemins ignorés** : dossiers cachés, `build*`, `cmake-build-*`, `third_party`, `extern`, `vendor`, `node_modules` et les fichiers `generated_*` ; `-e` en ajoute.

Les règles de chaque famille sont dans [les règles](page:rules#generic).
