+++
title = "Développement"
summary = "Modes de compilation, tests, organisation des sources, workflows et fabrication d'une release."
section = "resources"
order = 2
keywords = "développement compilation debug asan optimized tests ctest gtest sources organisation workflows release tag pre-release docs site"
+++

## Compiler {#build}

```bash "Terminal"
make                                                 # configurer et compiler dans build/
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug         # build debug (-g -ggdb3)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Asan          # build AddressSanitizer
cmake --build build --target release                 # builds Debug, Asan et Optimized
cmake --build build --target package_release         # paquets RPM / DEB / Arch de chaque variante
cmake --build build --target get_unregistered_files  # .cpp absents de SRC / SRC_SHARED
./xstyle --rtk -r src include tests CMakeLists.txt   # xstyle reste propre sur lui-même (vérifié par la CI)
```

> [!NOTE]
> Chaque nouveau `.cpp` va dans `SRC` (exécutable seul) ou `SRC_SHARED` (exécutable et tests) du `CMakeLists.txt`.

## Tests {#tests}

```bash "Terminal"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build --parallel $(nproc)   # ./unit_tests à la racine
(cd build && ctest --output-on-failure)   # GoogleTest + tests de la ligne de commande
```

## Sources {#layout}

| Chemin | Contenu |
|---|---|
| `src/xstyle/` | `main.cpp` (entrée), `Core.cpp` (arguments, lancement), `Checker.cpp` (règles et boucle de correction), `Reporter.cpp`, `Rules.cpp` (table des règles), `check/` (un fichier par famille) |
| `include/xstyle/` | headers |
| `assets/fonts/ansi_shadow.flf` | police de la bannière des en-têtes, embarquée à la compilation |
| `tests/xstyle/` | tests unitaires (GoogleTest) et tests de la ligne de commande (`ctest`) |
| `docs/site/` | sources de ce site (Markdown + générateurs `gen/`) |

## Ce site {#docs}

```bash "Terminal"
make                                                                     # gen/ lit ./xstyle
python3 ~/.claude/skills/better-html-doc/scripts/build_site.py --site docs/site --out docs
```

Les pages sont écrites dans `docs/` et publiées sur `gh-pages` par `Docs - Pages (CD)` au push suivant. Les pages des règles et des options sont générées depuis `./xstyle --list-rules` et `./xstyle --help`.

## Workflows et releases {#workflows}

| Workflow | Fait |
|---|---|
| `Dispatch (CI/CD)` | à chaque push de `main` ou d'un tag `v*` (pas pour `*.md` / `*.txt` / `docs/` seuls) : décide de ce qui tourne |
| `Unit Tests (CI)` | compile et lance les tests, toujours en premier |
| `Build (CI)` | vérification du build et xstyle sur ses propres sources (`--fail-on major`), quand aucun paquet n'est construit |
| `Build - Packages (CI/CD)` | paquets RPM / DEB / Arch, signés et publiés sur `gh-pages` |
| `Release - GitHub (CD)` | la release GitHub d'un tag, depuis sa section du `CHANGELOG.md` |
| `Docs - Pages (CD)` | copie `docs/` sur `gh-pages` |

Un tag `vX.Y.Z` (sa section `## vX.Y.Z - date` doit exister dans le changelog) construit les paquets stables et la release ; un tag `vX.Y.Z-<suffixe>` ou un commit avec `[build]` les paquets `-pre` ; `[ignore]` saute la CI.
