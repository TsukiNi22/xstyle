+++
title = "Usage courant"
summary = "Vérifier des chemins, ne garder que l'essentiel, corriger, écrire un rapport, l'utiliser depuis une IA."
section = "guide"
order = 2
keywords = "vérifier chemins filtrer sévérité code ignorer langage corriger dry-run force rapport json markdown rtk ia lien"
+++

Les commandes de tous les jours. Tous les flags sont dans [les options](page:options#selection).

## Vérifier {#check}

```bash "Terminal"
xstyle                        # le dossier courant, récursif, tous les langages connus
xstyle src/core/Engine.cpp    # un fichier
xstyle -r src include         # ces dossiers (récursif avec -r)
xstyle -e build,*.gen.hpp     # ignorer d'autres chemins (nom de dossier, préfixe ou glob)
```

Sans chemin, le dossier courant est parcouru récursivement. Les dossiers cachés, `build*`, `cmake-build-*`, `third_party`, `extern`, `vendor`, `node_modules` et les fichiers `generated_*` sont toujours ignorés.

Chaque problème est affiché avec son fichier, sa ligne et sa colonne (un lien du terminal), la règle enfreinte, son [niveau de fix](page:severities#fix-levels) et la correction proposée ; le résumé compte les problèmes par sévérité et par règle.

## Ne garder que l'essentiel {#filter}

| Commande | Garde |
|---|---|
| `xstyle -s major` | les problèmes `major` et `unforgivable` |
| `xstyle -c CPP-THIS,LU` | ces règles (code, préfixe ou motif : `CPP`, `LU-*`, `G-TAB`) |
| `xstyle -i G-TODO,CPP-AUTO` | toutes les règles sauf celles-ci |
| `xstyle -l cpp,rs` | les fichiers C++ et Rust (`-t` : seulement le langage le plus présent) |
| `xstyle -m force,manual` | les problèmes de ces niveaux de fix |
| `xstyle -S` | seulement le résumé |

## Corriger {#fix}

```bash "Terminal"
xstyle --fix                           # toutes les corrections sûres ([auto-fix])
xstyle --fix CPP-NULL,G-TRAILING src   # seulement ces règles, seulement dans src
xstyle -f -n -c CPP -r src/core        # aperçu (dry run) des corrections C++ d'un dossier
xstyle --fix --force                   # aussi les [force-fix] : vérifier avec -n d'abord
xstyle --fix --dangerous-force         # aussi les [danger-fix] : devinées, peuvent être fausses
xstyle --fix --header default          # en-têtes manquants avec la bannière XARTANIA, sans question
```

La logique du code n'est jamais touchée ; relire le résultat avec `git diff`. [Comment marche la boucle de correction](page:fix-loop#idea).

## Rapports {#report}

```bash "Terminal"
xstyle -o report.md             # .txt, .md ou .json, rien dans le terminal
xstyle -s major -o report.json  # major et plus, pour un script
```

`--format text|md|json` force le format quand l'extension ne le dit pas.

## Depuis une IA {#rtk}

`xstyle --rtk` (ou `XSTYLE_RTK=1`) n'affiche que ce dont un assistant a besoin : environ 5 fois moins de tokens.

```text "xstyle --rtk --r34 -c CPP-THIS a.hpp"
# >file then line:col CODE severity(U/M/m/n) fix(a=auto f=--force d=--dangerous-force q=asked -=manual) ...
>a.hpp
3:33 CPP-THIS m a
* CPP-THIS Member accessed without this-> => int get(void) const {return this->_name;};
= 1 issues in 1 files: U0 M0 m1 n0 | fix a1 f0 d0 q0 -0 | xstyle --fix
```

Une ligne de légende, `>fichier` une fois, une ligne `ligne:col CODE sévérité fix` par problème, le message et la correction de chaque règle une seule fois dans une ligne `* CODE`, le résumé sur une ligne. Ni couleur, ni lien, ni ligne de source, ni tableau.

## Liens et couleurs {#links}

| Flag | Effet |
|---|---|
| `--link file` | liens `file://` sur les positions (par défaut) |
| `--link vscode` | liens `vscode://` : ouvre le fichier à la ligne dans VS Code |
| `--link none` | pas de lien |
| `--no-color` | texte brut (aussi avec `NO_COLOR`, ou quand la sortie n'est pas un terminal) |
