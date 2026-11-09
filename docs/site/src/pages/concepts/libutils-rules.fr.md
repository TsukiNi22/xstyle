+++
title = "Les règles libutils"
summary = "Quand les règles LU sont actives, et ce qu'elles vérifient : includes, attributs, migrations, code que libutils fournit déjà."
section = "concepts"
order = 3
keywords = "libutils LU include section attribut migration exception argparser ansi socket syscall thread base64 verbose libutils-check skill"
+++

Les règles `LU-*` poussent un projet C++ qui utilise [libutils](https://github.com/TsukiNi22/libutils) à s'en servir comme prévu.

## Quand elles sont actives {#when}

| Condition | Lue depuis |
|---|---|
| libutils est **installée** | les headers dans `/usr/include`, `/usr/local/include`, `~/.local/include`, `CPATH`, sinon le sysroot MinGW de `mingw64-libutils` |
| le projet l'**utilise** | `find_package(utils)` / `utils::utils` dans le `CMakeLists.txt` racine, ou `utils.hpp` inclus |

Il faut les deux (`--libutils auto`, par défaut) ; `--libutils on|off` (ou `XSTYLE_LIBUTILS`) les force. Dans libutils elle-même, elles sont désactivées. Le résumé dit ce qui a été trouvé : `libutils  v4.3.1 installed, used by the project - libutils rules on`.

## Includes et sections {#includes}

Un seul `<utils/utils.hpp>` racine, précédé du `#define _Section` de chaque section utilisée par le fichier, jamais un header de libutils un par un.

```cpp "Attendu"
#define _Math
#define _Attribute
#include <utils/utils.hpp> // trigo, _nodiscard
```

| Règle | Problème |
|---|---|
| `LU-INCLUDE` | un header de libutils inclus directement |
| `LU-BARE-INCLUDE` | `<utils/utils.hpp>` sans aucun `#define _Section` avant |
| `LU-SECTION` | une section utilisée (ses noms `utils::<section>::`, ses macros d'attributs) sans son `#define` |

## Attributs et migrations {#migrations}

- `LU-ATTRIBUTE` : `[[nodiscard]]`, `[[maybe_unused]]`, `alignas(...)`... deviennent les macros de libutils (`_nodiscard`, `_unused`, `_alignas`...).
- `LU-MIGRATION` : un nom déprécié devient le nouveau, lu dans les alias `_migration` des headers **installés**, plus les noms supprimés par libutils v4.0.0 (`isloaded()` → `isLoaded()`, `utils::network::socket::is_ip` → `utils::network::is_ip`, `setStyle` → `set_style`, `fixed_string` → `FixedString`).

Les deux sont `[auto-fix]`.

## Code que libutils fournit déjà {#replace}

Ces règles sont `[manual]` : l'indice donne la classe de libutils à utiliser.

| Règle | Écrit à la main | À utiliser |
|---|---|---|
| `LU-EXCEPTION` | une exception standard levée | `utils::exception::ErrorException` |
| `LU-ARGS` | `argv` analysé à la main | `utils::arguments::ArgParser` |
| `LU-ANSI` | séquences ANSI brutes | `utils::iomanip` |
| `LU-SOCKET` | sockets BSD | `utils::network` (`TCPSocket`, `Server`, `Client`) |
| `LU-SYSCALL` | `pipe` / `dup` / `fork` / `epoll` / `shm_open` / `dlopen` | `utils::encapsulation` |
| `LU-THREADS` | un pool de threads | `utils::pool::Cluster`, `utils::system::Scheduler` |
| `LU-ANGLE` | conversions degrés / radians | `utils::math::trigo` |
| `LU-BASE64` | base64 | `utils::smanip::codec::Base64Codec` |
| `LU-VERBOSE` | un flag verbose | `utils::verbose` |
| `LU-DISTANCE` | une distance entre chaînes | `utils::algorithms::c2dmp` |

## La référence est-elle à jour ? {#check}

`xstyle --libutils-check` compare la libutils installée avec la référence du skill libutils de [ai-utils](https://github.com/TsukiNi22/ai-utils) : sortie `0` si à jour, `1` sinon. Le skill est cherché dans `$XSTYLE_LIBUTILS_SKILL`, sinon `~/.claude/skills/libutils`, sinon `~/personal_delivery/other/ai-utils/libutils/libutils`.
