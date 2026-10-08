# Progress log

## dosunit differential sweep — full coverage milestone

Final state after the fixture-hardening work:

- **311 ported routines: 0 diff, 0 uncovered**
- 76 dedicated specs + 212 probe-agree + 5 probe-mix + 11 artifact +
  7 incomplete + 0 skipped
- Dedicated suites (~1700 vectors): all AGREE — egame_tac 341,
  egame_combat 209, egame_state 269, egame_state2 226, edge 36,
  start_math 157, egame_scalelod 147, egame_math2 72, start_util 151,
  plus clock/objective/bufpos/select/wrap/su_wrap shards.

### Fixture layers (all in `dosunit/gen_probe.py` + `tools/duspec.py`)

| layer | mechanism |
|---|---|
| slotstub | every oracle driver-ABI `EA` slot -> `xor ax,ax; retf` |
| int_stub | reachable `CD xx` -> `xor ax,ax` (BFS over call/lcall/jmp graph) |
| int_stub_c | same for the candidate image |
| call_stub | oracle calls into cand-side trivial stubs -> nop/`pop ax`/`xor ax,ax` |
| dsss_stub | reachable `ds:=ss` writes nopped (DS==SS under DOS) |
| reloc-drop | MZ relocs overlapping any patched span are removed |

Key invariants learned:

- `_drop_relocs_overlapping` is mandatory on every patched span — the MZ
  loader rewrites relocated words inside nop'd `9A` lcalls and desyncs the
  instruction stream (`90 90` -> `90 a0`).
- `stub_models` peels leading casts — `return (int16)a;` classifies as a
  param-return stub (`sub_15AD2` callers `sub_15460`/`sub_15B68`/
  `setViewOrigin` were unpatched until then).
- `sub_<linaddr>` stub names index linear image offsets; 5-digit hex
  suffixes are legal in `offstubs`.
- `find_ints` dedups scanned instructions (`iseen`) — overlapping callee
  windows otherwise exhaust the 40K budget before reaching deep helpers
  like `resFileOpen`'s int21 path (which does `mov bx,ss; mov ds,bx`).
- `coverage.py` indexes cases globally across shard files — regens
  reshard cases without orphaning out.json results.

### Entry points probed (formerly skipped)

- `gfxInit` 5/5 AGREE, `openBlitClosePic` 5/5 AGREE
- `main` 5/5 AGREE-FAULT, `waitForKeyPress` 5/5 AGREE-FAULT (symmetric
  budget burn on poll loops)
- `runGameSession` incomplete — oracle reaches `device_io_or_halt` inside
  the session loop (real hardware access; sandbox limit, documented)

### Remaining known-partial cells

- probe-mix: `allocBuffer` (cand alloc retry loop on seeded freelist),
  `drawClippedLineEx`/`drawClippedLineRegion` (one-sided div0 on a seeded
  divisor cell), `drawGaugeBar` (oracle writes into declared code
  bytes — SMC guard), `drawStringCentered` (cand fetch outside declared
  code ranges)
- incomplete: `drawStoreIcons`/`loadPicFromFileAt` (loops proportional to
  seeded counts), `sub_16076`/`sub_16486`/`sub_1714C`/`sub_17280`/`runGameSession`
  (in/out hardware io)
- artifact: 11 routines, all MATCH-verified byte-exact — diffs are
  binary-relative DS layouts, stub-callee asymmetry, or obs cross-wiring
  (see `dosunit/coverage.py` ARTIFACTS for per-routine reasons)
