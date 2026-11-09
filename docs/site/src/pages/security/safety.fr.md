+++
title = "Ce que xstyle lit, écrit et lance"
summary = "Chaque fichier que xstyle peut modifier, chaque commande qu'il lance, et ce qu'il ne fait jamais."
section = "security"
order = 1
keywords = "sécurité sûreté écriture fichiers fix git commit réseau rapport R34.md lecture seule dry-run"
+++

xstyle est un outil local : il lit des fichiers source, et ne les modifie que sur demande.

## Ce qu'il lit {#reads}

| Lu | Pourquoi |
|---|---|
| les fichiers donnés (ou trouvés sous les chemins) | la vérification elle-même |
| le `CMakeLists.txt` racine, les includes | libutils est-elle utilisée, quelles sections |
| les headers de libutils installés | les règles `LU-*` et les noms de migration |
| `VERSION.md` du skill libutils, le clone de libutils (`$LIBUTILS`, sinon `~/personal_delivery/cpp/libutils`) | seulement `--libutils-check` |

## Ce qu'il écrit {#writes}

Sans `--fix` ni `-o`, xstyle n'écrit rien.

| Écrit | Quand |
|---|---|
| les fichiers vérifiés | `--fix` seulement, jamais avec `-n` / `--dry-run` |
| le fichier de rapport | `-o <fichier>` (`.txt`, `.md`, `.json`) |
| `R34.md` dans le dossier courant | `--fix --force`, jamais par-dessus un fichier existant |
| un commit git | `--fix --commit` / `--commit-all` seulement |

## Ce qu'il lance {#runs}

Seulement `git`, dans le dépôt des fichiers vérifiés, lancé directement (sans shell) :

| Commande | Pour |
|---|---|
| `git rev-parse --show-toplevel` | trouver le dépôt |
| `git diff -U0 --no-color --no-ext-diff --no-renames`, `git ls-files --others --exclude-standard` | `--diff`, `--diff-ref`, `--staged` |
| `git status --porcelain` | les fichiers déjà modifiés avant `--commit` |
| `git add`, `git commit -q -m <message>` | `--commit`, `--commit-all` |
| `git rev-list --count` dans le clone de libutils | `--libutils-check` |

Aucun accès réseau, aucune commande shell construite depuis un fichier, aucun code du projet vérifié exécuté.

## Corrections qui changent le comportement {#fixes}

> [!WARNING]
> `--force` et `--dangerous-force` peuvent changer ce que fait le code : `set -euo pipefail` arrête un script à la première commande en échec, `encoding="utf-8"` fait échouer un fichier dans un autre encodage, un type deviné peut être faux. Les prévisualiser avec `-n`, puis relire le résultat avec `git diff`.

`--commit` ne prend jamais un fichier qui avait des modifications avant les corrections : ses propres modifications finiraient dans le commit sans être vues.
