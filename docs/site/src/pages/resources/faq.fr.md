+++
title = "FAQ"
summary = "Des réponses courtes aux questions habituelles sur xstyle."
section = "resources"
order = 3
keywords = "faq questions r34 fichier de configuration windows code de sortie règles libutils désactivées"
+++

## Y a-t-il un fichier de configuration ? {#config}

Non. Les valeurs par défaut viennent des [variables d'environnement](page:suppression#environment), les exceptions des [commentaires dans le code](page:suppression#comments).

## Qu'est-ce que R34, et comment la masquer ? {#r34}

Un rappel obligatoire, jamais compté. `--r34`, `XSTYLE_R34=1`, un fichier `R34.md`, ou `34` / `r34` dans un fichier vérifié la masquent. [Détails](page:severities#mandatory).

## Pourquoi les règles libutils sont-elles désactivées ? {#libutils}

Il faut que libutils soit installée **et** utilisée par le projet (`find_package(utils)`, `utils::utils` ou `utils.hpp`). `--libutils on` les force. [Détails](page:libutils-rules#when).

## Pourquoi un lancement sort-il avec 1 quand je ne vois que des problèmes minor ? {#exit}

`-s` masque seulement. Le code de sortie suit `--fail-on` (par défaut : n'importe quel problème). [Détails](page:severities#scale).

## Ça marche sous Windows ? {#windows}

Non : xstyle utilise des appels POSIX et n'est compilé que pour Linux (RPM, DEB, Arch). Il vérifie du code Windows (fins de ligne `G-CRLF` comprises) depuis Linux.

## Comment ne vérifier que mes modifications ? {#changes}

`xstyle --diff` (depuis `HEAD`), `--diff-ref main`, ou `--staged`. [Détails](page:git-and-ci#diff).
