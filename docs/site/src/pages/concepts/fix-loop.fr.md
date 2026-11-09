+++
title = "Comment marche la boucle de correction"
summary = "Pourquoi un seul --fix peut appliquer des corrections qu'aucune vérification n'a montrées, et pourquoi il ne boucle jamais sans fin."
section = "concepts"
order = 2
kind = "explain"
level = "simple"
keywords = "boucle correction passe passes chevauchement dry-run FIX_PASSES problème révélé chaîne"
+++

:::simple tldr
**En une phrase :** `--fix` vérifie, corrige, puis vérifie encore, car une correction peut révéler un nouveau problème, jusqu'à ce qu'il ne reste rien à corriger.
:::

:::technical tldr
**Définition :** `Checker::fix` fait au plus `FIX_PASSES` (32) passes ; chaque passe relance toutes les règles, garde les corrections permises dont les plages de lignes ne se chevauchent pas, les applique, et s'arrête à la première passe sans correction.
:::

## L'idée {#idea}

:::simple
Comme ranger une chambre : tu mets les livres sur l'étagère, puis tu vois qu'il faut une étiquette sur l'étagère. Tu ranges, tu regardes encore, tu ranges encore, jusqu'à ce que tout soit propre.
:::

:::technical
Une correction modifie le fichier, donc les problèmes calculés avant elle sont périmés. Au lieu de prédire l'effet d'une correction, xstyle recalcule : les règles sont la seule source de vérité, et une passe n'applique que des corrections sur des plages de lignes disjointes, pour que deux modifications ne se battent jamais sur les mêmes lignes. Les autres reviennent à la passe suivante.
:::

## Comment ça marche {#how}

:::html
<figure class="stepper" data-steps="6">
  <ol class="captions">
    <li><b>Vérification, passe 1.</b> Les règles tournent sur le fichier : un problème corrigeable, <code>LU-ATTRIBUTE</code> ligne 4 (<code>[[nodiscard]]</code>).</li>
    <li><b>Tri.</b> Son niveau est <code>auto</code>, permis par <code>--fix</code>, et il ne chevauche aucune autre correction de la passe : il est gardé.</li>
    <li><b>Application.</b> La ligne 4 devient <code>_nodiscard double angle(double x);</code>.</li>
    <li><b>Vérification, passe 2.</b> <code>_nodiscard</code> fait partie de la section <code>Attribute</code>, non définie avant <code>utils.hpp</code> : <code>LU-SECTION</code> apparaît.</li>
    <li><b>Application.</b> <code>#define _Attribute</code> est ajouté avant l’include.</li>
    <li><b>Vérification, passe 3.</b> Plus aucune correction : la boucle s’arrête et le fichier est écrit (avec <code>-n</code> : seulement affiché).</li>
  </ol>
  <svg viewBox="0 0 760 300" role="img" aria-label="boucle vérifier, trier, appliquer sur un fichier utilisant libutils">
    <defs><marker id="fr-fx-arr" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto"><path class="arrow" d="M0 0L10 5L0 10z"/></marker></defs>
    <rect id="fr-fx-check" class="box" x="90" y="30" width="170" height="50" rx="8" data-hl="0,3,5"/><text class="lbl" x="175" y="60" text-anchor="middle">vérifier</text>
    <rect id="fr-fx-keep" class="box" x="90" y="125" width="170" height="50" rx="8" data-hl="1"/><text class="lbl" x="175" y="155" text-anchor="middle">trier</text>
    <rect id="fr-fx-apply" class="box" x="90" y="220" width="170" height="50" rx="8" data-hl="2,4"/><text class="lbl" x="175" y="250" text-anchor="middle">appliquer</text>
    <path class="edge" data-from="fr-fx-check:bottom" data-to="fr-fx-keep:top" marker-end="url(#fr-fx-arr)"/>
    <path class="edge" data-from="fr-fx-keep:bottom" data-to="fr-fx-apply:top" marker-end="url(#fr-fx-arr)"/>
    <path id="fr-fx-loop" class="edge-accent" data-from="fr-fx-apply:left" data-to="fr-fx-check:left" data-bend="-0.6" marker-end="url(#fr-fx-arr)"/>
    <rect id="fr-fx-code" class="group" x="300" y="30" width="430" height="180" rx="8"/>
    <text class="lbl" x="320" y="62" style="font-family:ui-monospace,monospace">#define _Math</text>
    <text class="lbl" x="320" y="92" style="font-family:ui-monospace,monospace" data-show="4-" data-hl="4">#define _Attribute</text>
    <text class="lbl" x="320" y="122" style="font-family:ui-monospace,monospace">#include &lt;utils/utils.hpp&gt; // trigo</text>
    <text class="lbl" x="320" y="182" style="font-family:ui-monospace,monospace" data-show="0-1" data-warn="0,1">[[nodiscard]] double angle(double x);</text>
    <text class="lbl" x="320" y="182" style="font-family:ui-monospace,monospace" data-show="2-" data-hl="2">_nodiscard double angle(double x);</text>
    <text class="lbl" x="320" y="250" data-show="0-2">passe 1</text>
    <text class="lbl" x="320" y="250" data-show="3-4">passe 2</text>
    <text class="lbl" x="320" y="250" data-show="5-">passe 3</text>
    <text class="lbl" x="420" y="250" data-show="0-2">LU-ATTRIBUTE</text>
    <text class="lbl" x="420" y="250" data-show="3-4">LU-SECTION</text>
    <text class="lbl" x="420" y="250" data-show="5-">plus de correction</text>
  </svg>
</figure>
:::

## L'exemple {#example}

Le fichier ci-dessus, corrigé avec seulement les règles libutils :

```bash "Terminal"
xstyle --fix -n --libutils on -c LU b.cpp
```

```text "sortie"
Would fix:
b.cpp:4 LU-ATTRIBUTE
    - [[nodiscard]] double angle(double x);
    + _nodiscard double angle(double x);
b.cpp:2 LU-SECTION
    + #define _Attribute
```

`LU-SECTION` n'était pas dans le rapport avant la correction : la deuxième passe l'a trouvé.

## Pourquoi elle s'arrête {#stop}

:::simple
Elle s'arrête dès qu'un tour ne corrige rien, et dans tous les cas après 32 tours.
:::

:::technical
Trois garde-fous : une passe sans correction termine la boucle ; une correction dont les nouvelles lignes sont égales aux anciennes est écartée (un no-op bouclerait sans fin) ; `FIX_PASSES` (32, `include/xstyle/Checker.hpp`) borne le reste.
:::

| Garde-fou | Où |
|---|---|
| aucune correction dans une passe → arrêt | `Checker::fix` |
| correction égale aux lignes actuelles → sautée | `Checker::fix` |
| au plus 32 passes | `FIX_PASSES` |

`Checker::fix` est dans `src/xstyle/Checker.cpp`, `FIX_PASSES` dans `include/xstyle/Checker.hpp`.

## Pièges {#pitfalls}

- `-c` / `-m` limitent chaque passe : la correction d'une autre règle révélée par la première n'est pas appliquée (ici, `-c LU-ATTRIBUTE` seul laisserait le problème `LU-SECTION`).
- Les compteurs après un `--fix -n` sont les problèmes **restants** après les corrections, pas ceux d'avant.
- `--diff` / `--staged` filtrent aussi les corrections : une correction sur une ligne non modifiée n'est pas appliquée.

## Vérifie-toi {#quiz}

:::html
<details class="quiz"><summary>Deux corrections touchent la ligne 10 dans la même passe : que se passe-t-il ?</summary><p>La première est appliquée, la seconde attend la passe suivante, recalculée sur la nouvelle ligne.</p></details>
<details class="quiz"><summary>Pourquoi <code>--fix</code> peut-il signaler une règle que la vérification n'a pas montrée ?</summary><p>Une correction l'a révélée : elle n'existe qu'une fois la première correction écrite.</p></details>
:::

## Glossaire {#glossary}

| Terme | Sens |
|---|---|
| Passe | un passage de toutes les règles suivi des corrections qu'il permet |
| Chevauchement | deux corrections sur les mêmes lignes : une seule par passe |
| Dry run | `-n` : tout est calculé, rien n'est écrit |
