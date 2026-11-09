+++
title = "Git, hooks et CI"
summary = "Vérifier seulement les lignes modifiées, committer les corrections, refuser un mauvais commit avec le hook pre-commit, faire échouer un job de CI."
section = "guide"
order = 3
keywords = "git diff staged diff-ref commit commit-all pre-commit hook githooks ci workflow fail-on code de sortie"
+++

xstyle lit git pour ne vérifier que ce qui a changé, et son code de sortie arrête un commit ou un job de CI.

## Seulement les lignes modifiées {#diff}

| Commande | Vérifie |
|---|---|
| `xstyle --diff` | les lignes modifiées depuis `HEAD` (indexées ou non, nouveaux fichiers compris) |
| `xstyle --diff-ref main` | les lignes modifiées depuis une branche, un tag ou un commit |
| `xstyle --staged` | seulement les lignes indexées (`git diff --cached`) : le mode du hook pre-commit |

Sans chemin, ces modes prennent les fichiers modifiés eux-mêmes. Un problème qui porte sur tout le fichier est gardé quand le fichier a changé ; `--fix` suit le même filtre.

## Committer les corrections {#commit}

```bash "Terminal"
xstyle --fix --commit                                  # corriger, puis committer les fichiers corrigés
xstyle --fix --commit-all --message "chore(core): ..." # corriger, puis committer toutes les modifications suivies avec
```

Le message par défaut est `chore(style): apply the xstyle fixes (N issues)`, avec une ligne par règle. `--commit` ne prend que les fichiers qui n'avaient **aucune** modification avant les corrections (les autres sont listés et laissés) ; `--commit-all` prend aussi toutes les modifications suivies. Jamais avec `--dry-run`.

## Hook pre-commit {#hook}

Le hook du dépôt (`.githooks/pre-commit`) lance xstyle sur les lignes indexées à chaque `git commit` et refuse un problème `major` (ou pire).

```bash "Terminal"
git config core.hooksPath .githooks   # une fois par clone
git commit --no-verify                # le sauter une fois (ou XSTYLE_HOOK=0 git commit ...)
xstyle --fix --staged                 # corriger ce qui peut l'être, puis git add -u
```

| Réglage en haut du hook | Valeurs (défaut en premier) |
|---|---|
| `SCOPE` | `staged` (lignes indexées) · `files` (fichiers indexés entiers) · `all` (tout le projet) |
| `FAIL_ON` | `major` · `unforgivable` · `minor` · `negligible` · `never` |
| `WARN_ON` | `minor` · `negligible` · `none` |
| `FIX` | `none` · `safe` (corrections auto des fichiers entièrement indexés, ré-ajoutés avant la vérification) |
| `MISSING` | `warn` · `fail` · `ignore` (xstyle non installé) |
| `OUTPUT` | `auto` (compact sous Claude Code / `XSTYLE_RTK`) · `human` · `rtk` |

> [!TIP]
> Dans un autre dépôt, le hook s'installe avec le skill `pre-commit`, ou le `setup.sh` de [ai-utils](https://github.com/TsukiNi22/ai-utils) (`hook`) ; un hook déjà présent est gardé et lancé en premier.

## Dans un job de CI {#ci}

```yaml "étape de workflow"
- name: Style check
  run: xstyle -r src include --fail-on major --r34 --rtk
```

Le code de sortie vaut `0` si tout est propre, `1` s'il reste un problème au niveau de `--fail-on` ou plus, `255` en cas d'erreur. Sur une pull request, `--diff-ref origin/<base>` ne vérifie que ses lignes. Le workflow `Build (CI)` de xstyle le lance ainsi sur ses propres sources.
