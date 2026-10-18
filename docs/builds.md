# Build maps

Addresses in this repository are for the **USA** build.
The title is `00050000-10101d00`.
The executable is `code/red-pro2.rpx` from USA 1.3.0 (title version 64).
Those addresses are the *canonical ids* of the game's functions and globals.

| Build | Title | Title version | `red-pro2.rpx` SHA-256 | Map |
| --- | --- | --- | --- | --- |
| USA (canonical) | `00050000-10101d00` | 64 | `ebd147ce…dc8e` | the identity |
| Europe | `00050000-10101e00` | 80 | `0ea59f40…ea51` | [`tools/recomp/builds/eu.json`](../tools/recomp/builds/eu.json) |
| USA + Luigi U | `00050000-1014b700` | 0 | `1031c741…2b05` | [`tools/recomp/builds/nsmbu_nslu_usa.json`](../tools/recomp/builds/nsmbu_nslu_usa.json) |
| Europe + Luigi U | `00050000-1014b800` | 0 | `1faa7ce3…a285` | [`tools/recomp/builds/nsmbu_nslu_eu.json`](../tools/recomp/builds/nsmbu_nslu_eu.json) |

A standalone base disc is still title version 0.
`nsmbu extract` and the installer apply the matching 1.3.0 update folder for USA and Europe, then check the merged RPX hash.
An NSMBU + NSLU pack is already a complete title at version 0.

## Regional and pack builds

Europe is the same program as USA 1.3.0 with a few language-related functions compiled differently.
Each combo pack is that program plus about two thousand NSLU functions inserted in the executable.
A **build map** turns a canonical USA address into the other build's address so one set of hooks and one runtime serve them.

Derive or re-check a map with `tools/recomp/mkbuildmap.py` from your own dumps, then commit the JSON only:

```sh
python3 tools/recomp/mkbuildmap.py usa/code/red-pro2.rpx eur/code/red-pro2.rpx \
    --name EU --title 0005000010101e00 --out tools/recomp/builds/eu.json
python3 tools/recomp/mkbuildmap.py usa/code/red-pro2.rpx combo-usa/code/red-pro2.rpx \
    --name NSMBU_NSLU_USA --title 000500001014b700 --out tools/recomp/builds/nsmbu_nslu_usa.json
python3 tools/recomp/mkbuildmap.py usa/code/red-pro2.rpx combo-eu/code/red-pro2.rpx \
    --name NSMBU_NSLU_EU --title 000500001014b800 --out tools/recomp/builds/nsmbu_nslu_eu.json
```

Japan (`00050000-10101c00`) has no map yet.
When the running build is Europe or a Europe combo pack, its own text files are used.

## How the port uses the registry

- **The recompiler** identifies `red-pro2.rpx` by SHA-256, maps hook addresses, and names every generated function by its canonical address when the function exists in USA.
  Functions only present in a pack build get a distinct symbol id so names never collide.
- **The runtime** uses `GC(…)` / `GD(…)` ([runtime/include/guest_addr.h](../runtime/include/guest_addr.h)).
  For the USA build the map is the identity.
- **A hooks file** may say which builds it is for (`# builds: USA`).
  An entry wrap on a function whose body differs between builds is allowed; the recompiler warns so you can check it.
