+++
title = "The libutils rules"
summary = "When the LU rules are on, and what they check: includes, attributes, migrations, code libutils already gives."
section = "concepts"
order = 3
keywords = "libutils LU include section attribute migration exception argparser ansi socket syscall thread base64 verbose libutils-check skill"
+++

The `LU-*` rules push a C++ project that uses [libutils](https://github.com/TsukiNi22/libutils) to use it the way it is meant to be used.

## When they are on {#when}

| Condition | Read from |
|---|---|
| libutils is **installed** | the headers in `/usr/include`, `/usr/local/include`, `~/.local/include`, `CPATH`, else the MinGW sysroot of `mingw64-libutils` |
| the project **uses** it | `find_package(utils)` / `utils::utils` in the root `CMakeLists.txt`, or `utils.hpp` included |

Both are needed (`--libutils auto`, the default); `--libutils on|off` (or `XSTYLE_LIBUTILS`) forces them. Inside libutils itself they are off. The summary says what was found: `libutils  v4.3.1 installed, used by the project - libutils rules on`.

## Includes and sections {#includes}

One root `<utils/utils.hpp>`, preceded by the `#define _Section` of every section the file uses, never a libutils header one by one.

```cpp "Expected"
#define _Math
#define _Attribute
#include <utils/utils.hpp> // trigo, _nodiscard
```

| Rule | Issue |
|---|---|
| `LU-INCLUDE` | a libutils header included directly |
| `LU-BARE-INCLUDE` | `<utils/utils.hpp>` with no `#define _Section` before it |
| `LU-SECTION` | a section used (its `utils::<section>::` names, its attribute macros) without its `#define` |

## Attributes and migrations {#migrations}

- `LU-ATTRIBUTE`: `[[nodiscard]]`, `[[maybe_unused]]`, `alignas(...)`... become the libutils macros (`_nodiscard`, `_unused`, `_alignas`...).
- `LU-MIGRATION`: a deprecated name becomes the new one, read from the `_migration` aliases of the **installed** headers, plus the names libutils v4.0.0 removed (`isloaded()` → `isLoaded()`, `utils::network::socket::is_ip` → `utils::network::is_ip`, `setStyle` → `set_style`, `fixed_string` → `FixedString`).

Both are `[auto-fix]`.

## Code libutils already gives {#replace}

These are `[manual]`: the hint names the libutils class to use.

| Rule | Written by hand | Use |
|---|---|---|
| `LU-EXCEPTION` | a standard exception thrown | `utils::exception::ErrorException` |
| `LU-ARGS` | `argv` parsed by hand | `utils::arguments::ArgParser` |
| `LU-ANSI` | raw ANSI sequences | `utils::iomanip` |
| `LU-SOCKET` | BSD sockets | `utils::network` (`TCPSocket`, `Server`, `Client`) |
| `LU-SYSCALL` | `pipe` / `dup` / `fork` / `epoll` / `shm_open` / `dlopen` | `utils::encapsulation` |
| `LU-THREADS` | a thread pool | `utils::pool::Cluster`, `utils::system::Scheduler` |
| `LU-ANGLE` | degree / radian conversions | `utils::math::trigo` |
| `LU-BASE64` | base64 | `utils::smanip::codec::Base64Codec` |
| `LU-VERBOSE` | a verbose flag | `utils::verbose` |
| `LU-DISTANCE` | a string distance | `utils::algorithms::c2dmp` |

## Is the reference up to date? {#check}

`xstyle --libutils-check` compares the installed libutils with the reference of the libutils skill of [ai-utils](https://github.com/TsukiNi22/ai-utils): exit `0` when up to date, `1` when not. The skill is found at `$XSTYLE_LIBUTILS_SKILL`, else `~/.claude/skills/libutils`, else `~/personal_delivery/other/ai-utils/libutils/libutils`.
