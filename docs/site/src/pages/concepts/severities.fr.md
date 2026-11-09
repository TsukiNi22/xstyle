+++
title = "Sévérités et niveaux de fix"
summary = "La gravité d'un problème, qui peut le corriger, et ce qui compte dans le code de sortie."
section = "concepts"
order = 1
kind = "explain"
level = "simple"
keywords = "sévérité unforgivable major minor negligible mandatory obligatoire r34 niveau de fix auto force danger ask manual fail-on sortie"
+++

:::simple tldr
**En une phrase :** chaque problème a une **sévérité** (sa gravité) et un **niveau de fix** (qui a le droit de le corriger) : la première décide si le lancement échoue, le second ce que `--fix` peut changer.
:::

:::technical tldr
**Définition :** deux attributs indépendants par règle, lus dans la table des règles (`xstyle --list-rules`) : `Severity` ∈ {Unforgivable, Major, Minor, Negligible, Mandatory} pilote les compteurs, `--severity` et `--fail-on` ; `FixMode` ∈ {Auto, Force, Dangerous, Ask, Manual} pilote les corrections que `--fix` applique.
:::

## L'idée {#idea}

:::simple
Pense à un prof qui corrige une copie. Certaines fautes coûtent la note (une mauvaise réponse), d'autres un point (un accent oublié). Et certaines, le prof les corrige en rouge, d'autres il les entoure seulement : à toi de les réécrire.
xstyle fait pareil : la **sévérité** est le coût de la faute, le **niveau de fix** dit si xstyle écrit la correction ou l'entoure seulement.
:::

:::technical
Sévérité et niveau de fix sont orthogonaux : `CPP-USING-NAMESPACE` est `unforgivable` et `manual`, `G-TRAILING` est `minor` et `auto`. La sévérité filtre le rapport (`-s`) et le code de sortie (`--fail-on`) ; le niveau de fix filtre ce que `--fix` touche (`--force`, `--dangerous-force`) et peut filtrer le rapport aussi (`-m`).
:::

## L'échelle des sévérités {#scale}

| Sévérité | Lettre dans `--rtk` | Exemple |
|---|---|---|
| `unforgivable` | `U` | `CPP-USING-NAMESPACE` : `using namespace` |
| `major` | `M` | `CPP-NULL` : `NULL` au lieu de `nullptr` |
| `minor` | `m` | `G-TRAILING` : espaces en fin de ligne |
| `negligible` | `n` | `G-EOF-NEWLINE` : pas de retour à la ligne en fin de fichier |

:::simple
`-s major` n'affiche que les deux pires niveaux. `--fail-on major` ne fait échouer le lancement que s'il en reste un.
:::

:::technical
`-s <niveau>` est la sévérité minimale affichée (par défaut `negligible`) ; `-F` / `--fail-on <niveau>` la sévérité minimale qui met le code de sortie à `1` (par défaut `negligible` : n'importe quel problème). Les deux incluent les niveaux au-dessus : `major` comprend `unforgivable`.
:::

## La règle obligatoire R34 {#mandatory}

`R34` est la seule règle de sévérité `mandatory`, hors de l'échelle : *Rule 34: if it exists, there is porn of it. No exceptions*. C'est un rappel, pas une règle de style.

| | R34 |
|---|---|
| Affichée | une fois par lancement, avec un lien vers l'[article](https://en.wikipedia.org/wiki/Rule_34) ; une ligne `! R34 ...` en `--rtk` |
| Comptée | jamais : ni dans les totaux, ni le code de sortie, ni `--fail-on`, ni le hook |
| Masquée par | l'entier `34` ou `r34` (toute casse) dans un fichier vérifié, un fichier `R34.md` dans le dossier du lancement, `--r34`, `XSTYLE_R34=1` |
| Correction | `xstyle --fix --force` écrit `R34.md` (ne remplace jamais un fichier existant) |

## Niveaux de fix {#fix-levels}

| Tag | Corrigé par | Sens |
|---|---|---|
| `[auto-fix]` | `--fix` | sûr, le comportement ne peut pas changer : espaces, ordre des includes, `nullptr`... |
| `[force-fix]` | `--fix --force` | peut changer le comportement : `set -euo pipefail`, `encoding="utf-8"`, un commentaire remplacé |
| `[danger-fix]` | `--fix --dangerous-force` | deviné, peut être faux : types Python tirés de l'usage, `static_cast` d'un cast de pointeur C |
| `[ask-fix]` | `--fix` + une question | l'en-tête de fichier manquant : bannière et description demandées par fichier (`--header` répond) |
| `[manual]` | à la main | une décision qu'un outil ne peut pas prendre : un type, un nom, une réécriture avec libutils |

:::simple
Commence par `--fix` seul. Regarde ce que ferait `--force` avec `-n` (rien n'est écrit), puis décide.
:::

:::technical
`--dangerous-force` implique `--force`. Sans terminal (CI), l'en-tête `[ask-fix]` est sauté sauf si `--header none|default|<texte>` est donné. `-m auto,force,dangerous,ask,manual` ne garde que ces niveaux dans l'affichage, les compteurs, le rapport et `--fix` (`xstyle --fix --force -m auto` n'applique que les corrections sûres).
:::

## Pièges {#pitfalls}

- `-s` masque des problèmes mais ne change pas le code de sortie : c'est le rôle de `--fail-on`.
- `-s` ne masque jamais R34 ; `-c` / `-i` / `-m` la sélectionnent comme toute autre règle.
- Un `[force-fix]` peut changer ce que fait un script : `set -euo pipefail` l'arrête à la première commande en échec.

## Vérifie-toi {#quiz}

:::html
<details class="quiz"><summary><code>xstyle -s major</code> sort-il avec 0 quand il ne reste que des problèmes minor ?</summary><p>Non : <code>-s</code> les masque seulement, le code de sortie les compte encore. <code>--fail-on major</code> donne 0.</p></details>
<details class="quiz"><summary>Quel flag applique les types Python devinés ?</summary><p><code>--fix --dangerous-force</code> : ce sont des <code>[danger-fix]</code>.</p></details>
<details class="quiz"><summary>R34 peut-elle faire refuser un commit par le hook pre-commit ?</summary><p>Non, une règle obligatoire n'est jamais comptée.</p></details>
:::

## Glossaire {#glossary}

| Terme | Sens |
|---|---|
| Sévérité | la gravité d'un problème, de `negligible` à `unforgivable` |
| Niveau de fix | qui peut le corriger : `--fix`, `--force`, `--dangerous-force`, une question, ou toi |
| `--fail-on` | la sévérité à partir de laquelle le code de sortie vaut `1` |
| Obligatoire | affichée, jamais comptée (seulement R34) |

Source : `include/xstyle/XStyleType.hpp` (`Severity`, `FixMode`) et `src/xstyle/Rules.cpp` du dépôt.
