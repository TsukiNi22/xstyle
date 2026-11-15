+++
title = "Ignorer des règles et des fichiers"
summary = "Faire taire une règle sur une ligne, une zone, un fichier ou un chemin (.xignore, .xconfig, flags), sauter les fichiers ignorés par git, compter ce qui est ignoré, et fixer les valeurs par défaut avec des variables d'environnement."
section = "reference"
order = 4
keywords = "ignore ignore-next ignore-file skip-file off on supprimer commentaire xignore xconfig gitignore ignore-path exclude-from no-gitignore no-inline-ignore show-ignored add-ignore G-UNUSED-IGNORE variables d'environnement XSTYLE_CODES XSTYLE_IGNORE XSTYLE_IGNORE_PATH XSTYLE_RTK NO_COLOR"
+++

## Dans un commentaire {#comments}

| Commentaire | Fait taire |
|---|---|
| `xstyle: ignore [CODE,...]` | cette ligne |
| `xstyle: ignore-next [N] [CODE,...]` | la ligne suivante (les `N` lignes suivantes) |
| `xstyle: off [CODE,...]` ... `xstyle: on [CODE,...]` | les lignes entre les deux (jusqu'à la fin du fichier sans `on` ; `on` seul ferme toutes les zones ouvertes) |
| `xstyle: ignore-file [CODE,...]` | tout le fichier |
| `xstyle: skip-file` | le fichier n'est ni vérifié ni compté |

Sans code, toutes les règles y sont tues ; un code peut être un préfixe ou un motif (`CPP`, `LU-*`). Une raison après les codes commence par un mot en minuscules ou une parenthèse (des mots en majuscules seraient lus comme des codes). En Markdown, une directive ne compte que dans un commentaire `<!-- -->`.

```cpp "Exemple"
std::cout << "\033[1m"; // xstyle: ignore LU-ANSI
// xstyle: ignore-next 2 CPP-AUTO
auto lambda = [](int x) {return x * 2;};
auto other = [](int x) {return x * 3;};
// xstyle: off CPP-ONELINER-ALIGN (table aligned by hand)
// ...
// xstyle: on
// xstyle: ignore-file CPP-GUARD-MISSING (no guard on purpose)
```

Un commentaire qui ne fait plus rien taire est signalé par `G-UNUSED-IGNORE` (mineur, retiré par `--fix` ; une zone `off` perd aussi son `on`). Il est comparé à tous les problèmes du fichier : `-c`, `-s` ou `--diff` ne rendent jamais un commentaire inutile. Les règles désactivées (libutils), `R34` et les commentaires qui nomment `G-UNUSED-IGNORE` ne sont jamais signalés ; seul un commentaire qui le nomme le fait taire.

## Par chemin : .xignore {#xignore}

Un fichier `.xignore` dans n'importe quel dossier du projet, lu de la racine du projet jusqu'au fichier comme un `.gitignore` :

- un motif par ligne, commentaires `#`, jokers `*` / `?` / `**` ;
- un motif avec un `/` est relatif au dossier du `.xignore`, sinon c'est un nom à n'importe quelle profondeur ;
- `nom/` ne correspond qu'aux dossiers, `!motif` réinclut, la dernière ligne qui correspond l'emporte ;
- une ligne finissant par `:CODE,...` n'exclut pas les chemins : elle y ignore ces règles.

```gitignore ".xignore"
third_party_copy/
*.gen.hpp
!keep.gen.hpp
tests/**:CPP-AUTO,G-TODO
```

Les fichiers ignorés par git sont aussi sautés (`git ls-files --others --ignored --exclude-standard`), sauf s'ils sont donnés en argument ; `--no-gitignore` les vérifie.

## Configuration du projet : .xconfig {#xconfig}

Un fichier `.xconfig` (TOML), le plus proche en remontant du dossier courant jusqu'à la racine du projet. Ses clés donnent la valeur par défaut des flags : l'environnement et la ligne de commande l'emportent, `ignore` s'ajoute à `-i`. `exclude` et `ignore-path` sont des motifs relatifs à son dossier (comme une ligne de `.xignore`), les tables `[[path]]` ignorent des règles (`ignore`) ou sautent (`exclude = true`) les chemins qu'elles `match`.

```toml ".xconfig"
severity = "minor"
fail-on = "major"
ignore = ["CPP-AUTO"]
lang = ["cpp", "py"]
gitignore = true
inline-ignore = true
exclude = ["third/**"]
ignore-path = ["examples/*:G-TODO"]

[[path]]
match = ["tests/**", "bench/**"]
ignore = ["CPP-FUNC-LENGTH", "CPP-UNCHECKED-RETURN"]

[[path]]
match = "generated/"
exclude = true
```

| Clé | Flag |
|---|---|
| `code`, `ignore`, `severity`, `fail-on`, `lang`, `mode` | `-c`, `-i`, `-s`, `-F`, `-l`, `-m` |
| `libutils`, `top`, `link`, `header`, `header-desc`, `r34` | `-u`, `-t`, `--link`, `--header`, `--header-desc`, `--r34` |
| `gitignore = false`, `inline-ignore = false` | `--no-gitignore`, `--no-inline-ignore` |
| `exclude`, `ignore-path` | comme une ligne de `.xignore` (relative au `.xconfig`) |

Une clé inconnue ou une valeur fausse arrête xstyle avec `<fichier>:<ligne>`.

## Flags {#flags}

| Flag | Effet |
|---|---|
| `-i CODES` | ces règles ignorées partout |
| `--ignore-path 'GLOB[:CODES];...'` | ces règles ignorées sur ces chemins (relatifs au dossier courant ; sans code : toutes les règles) |
| `-e MOTIFS`, `--exclude-from FICHIER` | ces chemins sautés (nom de dossier, préfixe de chemin ou glob) |
| `--no-gitignore` | les fichiers ignorés par git sont aussi vérifiés |
| `--no-inline-ignore` | audit : ni les commentaires, ni les `.xignore`, ni les tables `[[path]]` ne sont lus (les flags et les autres clés de `.xconfig` restent) |
| `--show-ignored` | les problèmes ignorés sont listés, pas seulement comptés |
| `--add-ignore` | écrit `xstyle: ignore CODE,...` sur la ligne de chaque problème restant |

Les problèmes tus par un commentaire, une règle par chemin ou `-i` sont **comptés** : `Ignored 3 issue(s) silenced (comment 1, path 2)` dans le résumé, `| ignored 3 (...)` en `--rtk`, `ignored` dans les rapports JSON et Markdown.

## Adopter xstyle sur du vieux code {#add-ignore}

`--add-ignore` écrit `xstyle: ignore CODE,...` en fin de ligne de chaque problème restant (après `-c` / `-s` / `-m` / `--diff` ; `-n` prévisualise), dans le commentaire du langage (`//`, `#`, `--`, `;`). Jamais sur une ligne qui finit dans une chaîne, un here-document ou une continuation `\`, une ligne vide, une ligne qui a déjà une directive, ni pour un problème de fichier entier ; un commentaire qui ne ferait rien taire (il supprime lui-même le problème, comme un espace en fin de ligne) est retiré. Les fichiers Markdown, JSON et YAML ne sont pas touchés.

```bash
xstyle --fix                      # d'abord ce qui peut être corrigé
xstyle --add-ignore -s major -n   # aperçu des commentaires des problèmes majeurs restants
xstyle --add-ignore -s major      # les écrire
```

## Variables d'environnement {#environment}

Chacune donne la valeur par défaut de son flag ; le flag donné en ligne de commande l'emporte.

| Variable | Flag |
|---|---|
| `XSTYLE_CODES` | `-c`, `--code` |
| `XSTYLE_IGNORE` | `-i`, `--ignore` |
| `XSTYLE_IGNORE_PATH` | `--ignore-path` |
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
