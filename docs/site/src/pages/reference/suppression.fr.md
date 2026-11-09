+++
title = "Suppression et environnement"
summary = "Faire taire une règle sur une ligne, la ligne suivante ou tout un fichier, et fixer les valeurs par défaut avec des variables d'environnement."
section = "reference"
order = 4
keywords = "ignore ignore-next ignore-file supprimer commentaire variables d'environnement XSTYLE_CODES XSTYLE_IGNORE XSTYLE_RTK NO_COLOR"
+++

## Dans un commentaire {#comments}

| Commentaire | Fait taire |
|---|---|
| `xstyle: ignore [CODE,...]` | cette ligne |
| `xstyle: ignore-next [CODE,...]` | la ligne suivante |
| `xstyle: ignore-file [CODE,...]` | tout le fichier |

Sans code, toutes les règles y sont tues ; un code peut être un préfixe ou un motif (`CPP`, `LU-*`). Une raison après les codes commence par un mot en minuscules ou une parenthèse (des mots en majuscules seraient lus comme des codes).

```cpp "Exemple"
std::cout << "\033[1m"; // xstyle: ignore LU-ANSI
// xstyle: ignore-next CPP-AUTO
auto lambda = [](int x) {return x * 2;};
// xstyle: ignore-file CPP-GUARD-MISSING (no guard on purpose)
```

## Variables d'environnement {#environment}

Chacune donne la valeur par défaut de son flag ; le flag donné en ligne de commande l'emporte.

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

| Autre variable | Effet |
|---|---|
| `XSTYLE_LIBUTILS_SKILL` | dossier du skill libutils lu par `--libutils-check` |
| `XSTYLE_HOOK=0` | le [hook pre-commit](page:git-and-ci#hook) ne fait rien |
| `XSTYLE_BIN` | le binaire lancé par le hook pre-commit |

Tous les flags : [les options](page:options#selection).
