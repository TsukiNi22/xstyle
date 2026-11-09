+++
title = "Premiers pas"
summary = "Installer xstyle depuis ses paquets ou ses sources, puis lancer la première vérification."
section = "guide"
order = 1
keywords = "installer installation dnf apt pacman my-universe paquets premier lancement compiler cmake"
+++

xstyle est un seul binaire, `xstyle`, sans fichier de configuration : on l'installe, on le lance dans un projet.

## Installer depuis les paquets {#packages}

Les paquets existent en RPM (Fedora), DEB (Debian / Ubuntu) et Arch (pacman), signés avec la clé `RPM-GPG-KEY-tsukini`.
Tous les dépôts de paquets de TsukiNi22 d'un coup, avec [my-universe](https://tsukini22.github.io/my-universe/) :

```bash "Terminal"
curl -fsSL https://tsukini22.github.io/my-universe/install.sh | bash -s
sudo dnf install xstyle          # ou : sudo apt install xstyle / sudo pacman -S xstyle
```

Ou seulement le dépôt de xstyle, avec son `setup.sh` (sans clone) :

```bash "Terminal"
curl -fsSL https://raw.githubusercontent.com/TsukiNi22/xstyle/main/setup.sh | bash -s -- --install
```

| Argument de `setup.sh` | Effet |
|---|---|
| `--install` | Installe aussi le paquet (par défaut seuls le dépôt et la clé GPG sont configurés) |
| `--pre` | Le paquet de pré-release (`xstyle-pre`) |
| `--no-sudo` | Lancer sans `sudo` (déjà root) |

> [!NOTE]
> Une erreur `invalid hash` de `dnf` / `apt` se règle en vidant le cache de l'outil, ou en relançant `setup.sh`.

## Compiler depuis les sources {#sources}

Il faut `clang++` (C++20), `cmake >= 3.20` et libutils `>= 4.1.0` (`>= 4.2.0` sur Arch).

```bash "Terminal"
git clone https://github.com/TsukiNi22/xstyle.git && cd xstyle
cmake -S . -B build -DCMAKE_BUILD_TYPE=Optimized
cmake --build build --parallel $(nproc)                       # ./xstyle à la racine
sudo cmake --build build --target install --parallel $(nproc) # /usr/local/bin/xstyle + les complétions
cmake --install build --prefix ~/.local                       # ou sans root : ~/.local/bin/xstyle
```

## Première vérification {#first-run}

1. Aller dans un projet : `cd mon-projet`.
2. Lancer `xstyle` : tous les fichiers connus du dossier courant sont vérifiés, récursivement.
3. Lire le résumé à la fin : les compteurs par sévérité et par règle.
4. Corriger ce qui est sûr : `xstyle --fix`, puis relire avec `git diff`.

```bash "Terminal"
xstyle            # tout vérifier
xstyle -S         # seulement le résumé
xstyle --fix      # appliquer les corrections sûres
```

> [!TIP]
> Le premier lancement affiche aussi un rappel **obligatoire**, `R34` : il n'est jamais compté. [Ce que c'est et comment le masquer](page:severities#mandatory).

Ensuite : [l'usage courant](page:everyday-use#check).
