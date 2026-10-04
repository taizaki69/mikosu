# mikosu

**An osu! client in the making: osu!stable's layout, with a clean, modern look.** It's built on [neomod](https://github.com/neomodnet/neomod), the actively developed descendant of [McOsu](https://github.com/McKay42/McOsu).

> **Status: early development.** Nothing is released yet, and what's here today is neomod with new build tooling. Progress and the plan are in [`docs/renovation/`](docs/renovation/), starting with [STATUS](docs/renovation/STATUS.md) and the [PLAN](docs/renovation/PLAN.md).

## Goals

- **Finds your library by itself** on Linux and Windows: osu!stable and osu!lazer libraries, scores, replays, skins, collections and settings. It only reads them; your files are never changed.
- **Star rating, pp and scoring that match the official clients,** checked against the official calculator.
- **Official leaderboards and your profile** through the public osu!api, plus in-game map downloads from community mirrors. mikosu never logs in to the official game servers.
- **Keeps what McOsu players love:** very low input latency, uncapped frame rates, practice tools (scrubbing, live speed and AR/CS/OD/HP), FPoSu, experimental mods and the console.

## Building (Linux)

Install the prerequisites listed in [CLAUDE.md](CLAUDE.md#build), then:

```
tools/build.sh linux       # -> build/dist/bin-x86_64/
tools/build.sh windows     # Windows x64 cross-build with llvm-mingw -> build-win64/dist/bin-x86_64/
```

The first build downloads and builds its dependencies, which takes a few minutes. Windows-native builds (MSVC) are under `cmake-win/`, inherited from neomod.

## Credits and license

GPL-3.0. See [LICENSE](LICENSE) and [CREDITS.md](CREDITS.md). mikosu exists thanks to McKay (McOsu, McEngine) and the neomod authors (kiwec, spectator and contributors).

mikosu isn't affiliated with or endorsed by ppy Pty Ltd. "osu!" is their trademark.
