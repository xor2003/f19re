# f19re — F-19 Stealth Fighter reconstruction

Decompilation / source reconstruction of the DOS game **F-19 Stealth Fighter**
(MicroProse, 1988), built with Microsoft C 5.1. Companion project to
[f15se2-re](https://github.com/xor2003/f15se2-re) (F-15 Strike Eagle II) — the
two games share a large codebase, and roughly a quarter of F-19's routines have
direct F-15 counterparts.

## Approach

The original `EGAME.EXE` is reconstructed two ways that must agree:

1. **Skeleton** — `src/egame.asm`, a full disassembly-derived assembly source.
   `make egame` assembles and links it into a **byte-identical** copy of the
   original executable; `make verify` checks every byte.
2. **C ports** — `src/*.c` progressively replace skeleton routines with the
   original C, verified instruction-for-instruction against the binary with
   `tools/portcheck.py` (compile the module with MSC 5.1, link a test exe, then
   `mzdiff` the routine's machine code vs. the original — a `MATCH` means the
   emitted instruction stream is identical modulo call targets and data
   offsets).

Routines with hand-written register conventions or inline DOS calls stay in
assembly; the rest are being ported one by one (see `AGENTS.md` for the
verified list and the name-hash trick MSC 5.1 uses for local stack slots).

## Layout

    src/        ported C sources + egame.asm (committed asm skeleton)
    tools/      portcheck.py (per-routine verify), compare_exe.py, IDA helpers
    mzretools/  vendored MZ toolchain: lst2asm.py/lst2ch.py (listing → asm),
                mzdiff/mzmap C++ tools, kvikdos emulator source
    map/        per-exe routine maps (extents + names)
    conf/       lst2asm config (egame.json), routine renames, toolchain.conf
    sig/        f15se2 signature maps used for cross-project routine matching
    ghidra_scripts/  Ghidra headless decompile scripts
    matched_routines.tsv  f15se2 ↔ f19 routine matches

## Requirements (not distributed)

- A DOS toolchain under `dos/`: `dos/msc510/` (Microsoft C 5.1: CL.EXE, LINK.EXE,
  headers/libs) — see `conf/toolchain.conf`.
- `uasm` (or MASM-compatible) for the skeleton.
- Python 3.

## Build

```sh
cd mzretools/tools/emulators/kvikdos && make     # kvikdos DOS emulator
cd ../../../mzretools && ./build.sh             # mzdiff + friends (cmake)
make egame      # assemble + link skeleton → build/egame.exe
make verify     # byte-compare against original EGAME.EXE
```

## Porting a routine

```sh
python3 tools/portcheck.py src/egtacmap.c drawHudViewLine
# or all at once
make port SRC=src/egtacmap.c ROUTINES="setDrawColor drawStringCentered"
```

Per-module MSC 5.1 flag sets (`/Os` vs `/Ot`, `/Oa`) are recorded in
`tools/portcheck.py` — they reproduce the original codegen exactly.
