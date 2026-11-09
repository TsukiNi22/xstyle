+++
title = "Paquets et complétions"
summary = "Les paquets de xstyle, leurs variantes, les complétions de shell qu'ils installent et le canal de pré-release."
section = "resources"
order = 1
keywords = "paquets rpm deb arch pacman xstyle-op xstyle-db xstyle-as pre release complétion bash zsh fish gpg"
+++

## Paquets {#packages}

Les mêmes paquets existent en RPM (Fedora), DEB (Debian / Ubuntu) et Arch (pacman, `.pkg.tar.zst`) : mêmes noms, contenu et dépendances. Ils sont signés et publiés sur la branche `gh-pages` ([les installer](page:getting-started#packages)).

| Paquet | Contenu |
|---|---|
| `xstyle` | installe le paquet par défaut (`xstyle-op`) |
| `xstyle-op` | l'exécutable compilé avec les options d'optimisation |
| `xstyle-db` | l'exécutable compilé avec les options de debug (pour le debug seulement) |
| `xstyle-as` | l'exécutable compilé avec AddressSanitizer (pour le debug seulement) |

> [!WARNING]
> Les trois variantes installent le même `/usr/bin/xstyle` : une seule à la fois.

## Pré-release {#pre}

Chaque paquet a une pré-release nommée `<paquet>-pre`, construite depuis un tag `vX.Y.Z-<suffixe>` ou un commit contenant `[build]`. Un paquet `-pre` est rendu obsolète par tout paquet stable de même version ou plus ; seules les 5 dernières versions de chaque `-pre` sont gardées dans le dépôt.

## Complétions de shell {#completions}

Chaque paquet installe la complétion des shells (flags, codes de règles, sévérités, langages, listes à virgules), générée par `xstyle --completion bash|zsh|fish` :

```text "Fichiers"
/usr/share/bash-completion/completions/xstyle
/usr/share/zsh/site-functions/_xstyle
/usr/share/fish/vendor_completions.d/xstyle.fish
```

| Shell | Chargé |
|---|---|
| bash | à la demande par bash-completion |
| zsh | dans les nouveaux shells (`compinit`) |
| fish | à la demande |

## Téléchargements et liens {#links}

| Lien | Contenu |
|---|---|
| [Releases](https://github.com/TsukiNi22/xstyle/releases) | la release GitHub de chaque tag stable, notes tirées du changelog |
| [CHANGELOG.md](https://github.com/TsukiNi22/xstyle/blob/main/CHANGELOG.md) | toutes les versions |
| [my-universe](https://tsukini22.github.io/my-universe/) | le catalogue de tous les paquets de TsukiNi22 |
| [Graphe du projet](graph.html) | fichiers, classes, appels et tests du dépôt |
