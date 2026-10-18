# Build maps

Addresses in this repository are for the **USA** build.
The title is `00050000-10101d00`.
The executable is `code/red-pro2.rpx` from USA 1.3.0 (title version 64).
Those addresses are the *canonical ids* of the game's functions and globals.

| Build | Title | Title version | `red-pro2.rpx` SHA-256 | Map |
| --- | --- | --- | --- | --- |
| USA (canonical) | `00050000-10101d00` | 64 | `ebd147ce…dc8e` | the identity |
| Europe | `00050000-10101e00` | 80 | `0ea59f40…ea51` | [`tools/recomp/builds/eu.json`](../tools/recomp/builds/eu.json) |

A base disc is still title version 0.
`nsmbu extract` and the installer apply the matching 1.3.0 update folder, then check the merged RPX hash.

## Regional builds

Europe uses the same program as USA 1.3.0 with a few language-related functions compiled differently.
A **build map** turns a canonical USA address into the Europe address so one set of hooks and one runtime serve both.

Derive or re-check a map with `tools/recomp/mkbuildmap.py` from your own dumps, then commit the JSON only:

```sh
python3 tools/recomp/mkbuildmap.py usa/code/red-pro2.rpx eur/code/red-pro2.rpx \
    --name EU --title 0005000010101e00 --out tools/recomp/builds/eu.json
python3 tools/recomp/mkbuildmap.py --check tools/recomp/builds/eu.json \
    usa/code/red-pro2.rpx eur/code/red-pro2.rpx
```

Japan (`00050000-10101c00`) has no map yet.
Language packs can still lend European or Japanese text to the USA build
([language-packs.md](language-packs.md)).
When the running build is Europe, its own text packs are used; a language source is not needed for that install.

The NSMBU + NSLU combo pack (`00050000-1014b700` and related) is a different program and is not supported by this map.

## How the port uses the registry

- **The recompiler** identifies `red-pro2.rpx` by SHA-256, maps hook addresses, and names every generated function by its canonical address.
- **The runtime** uses `GC(…)` / `GD(…)` ([runtime/include/guest_addr.h](../runtime/include/guest_addr.h)).
  For the USA build the map is the identity.
- **A hooks file** may say which builds it is for (`# builds: USA`).
  An entry wrap on a function whose body differs between builds is allowed; the recompiler warns so you can check it.
