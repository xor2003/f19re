# Decompilation guide — how the F-19 reconstruction is done

This documents the actual workflow used to decompile MicroProse's **F-19
Stealth Fighter** (DOS, 1988, built with Microsoft C 5.1) into C + assembly
sources, byte-verified against the original executables. The same process works
for any MSC 5.x game — it grew out of the
[f15se2-re](https://github.com/xor2003/f15se2-re) project (F-15 Strike Eagle II,
same engine, ~25% shared routines).

## 0. Big picture

The original `EGAME.EXE` is reconstructed along two tracks that must agree:

1. **Asm skeleton** (`src/egame.asm`, or regenerated `build/egame.asm`) — a
   complete disassembly-derived assembly source. `make egame` assembles (UASM)
   and links (original LINK 3.65 under emulation) a **byte-identical** copy of
   `EGAME.EXE`; `make verify` byte-compares the load image. This proves the
   listing → asm → link pipeline is faithful before any C exists.
2. **C ports** (`src/*.c`) — routines are rewritten in C one at a time and
   compiled with the *original compiler* (MSC 5.1, running under the kvikdos
   emulator). Each port is verified instruction-for-instruction against the
   original with `tools/portcheck.py` + `mzdiff`. A `MATCH` means the emitted
   instruction stream is identical modulo call targets and data offsets.

The skeleton stays the source of truth for unported routines. C files replace
skeleton routines incrementally; hand-written asm routines (int handlers, the
seg001/seg002 register-convention graphics pipeline, crt0) are never ported.

## 1. Inputs / artifacts

| file | role |
|------|------|
| `EGAME.EXE` | original binary (not committed) |
| `lst/EGAME.EXE.lst` | IDA disassembly listing — **the primary analysis source**. Symbols are IDA-generated: `sub_XXXX` (routine), `word_XXXX`/`byte_XXXX` (data), `loc_XXXX` (branch target), `var_N`/`arg_N` (stack) |
| `lst/egame.inc` | struct definitions the listing references (e.g. `SREGS`) |
| `map/egame.map` | routine extents `name: seg NEAR start-end`, used to locate each routine's bytes in the exe |
| `conf/egame.json` | lst2asm config: segment table, BSS split, per-site encoding fixes, `assume` substitutions |
| `conf/routine_names.txt` | rename registry: `routine_N newname # seg:off notes`. `tools/rename_routines.py` applies it to the map |
| `decompiled/` | Ghidra headless decompile, one `FUN_<para>_<off>.c` per function (451). `tools/ghidra_src.py <name>` dumps one |
| `matched_routines.tsv` | f15se2 ↔ f19 routine correspondences — the single most valuable input for the ~25% shared code |
| `src/stubs.c` | externs/empty bodies for globals + callees not yet ported, so a test exe always links |

## 2. Toolchain

- **MSC 5.1** in `dos/msc510/` (`CL.EXE`, `LINK.EXE`, headers, `mlibce.lib`).
- **kvikdos** (`mzretools/tools/emulators/kvikdos/kvikdos`) — a tiny Linux
  emulator good enough to run the real DOS compiler. CL.EXE runs under it
  directly; object files land in `dos/msc510/bin/`.
- **UASM** for the asm skeleton (`-q -0 -Zm`: quiet, 8086, MASM mode).
- **mzretools** (`mzretools/`): `lst2asm.py` (listing → flat asm with
  config-driven fixes), `lst2ch.py` (listing → C decls), `mzdiff` (instruction-
  stream differ), `mzmap`.
- LINK runs through a response file (`build/portlink.rsp`) because the object
  list exceeds DOS's ~126-char command line.

Typical cl invocation (as used by `portcheck.py`):

```sh
kvikdos --mount=C:dos/msc510/ --mount=D:src/ --mount=E:build/ \
        --env='LIB=C:\lib' --env='INCLUDE=C:\INCLUDE' \
        --env='PATH=C:\bin;C:\BBIN' --env='TMP=D:\' --drive=E \
        'C:\bin\CL.EXE' /AS /Gs /Os /FcD:\X.COD /c 'D:\EGCOMBAT.C'
```

`/FcX.COD` emits the generated-asm listing used for instruction-level
comparison — this is where all codegen analysis happens.

## 3. Per-routine workflow

The loop for one routine:

1. **Pick a routine.** `map/egame.map` lists extents; prefer routines adjacent
   to already-ported ones (same module, shared globals already declared).
   Check `AGENTS.md`'s "do NOT port" list first — register-convention asm
   routines (args in bx/si/di, no C prologue) are skipped.

2. **Read the disassembly.** Find `sub_XXXX proc near` in the .lst. Note:
   - `var_N = word ptr -N` → locals; `arg_N = word ptr N` → params.
   - Every `call sub_XXXX` → look up/cross-reference; every `word_XXXX` → a
     global to declare.
   - Early-exit `jmp loc_` structure → the original's control-flow shape.
   - `sar` vs `shr` on a global → signed `int16` vs `uint16`.
   - `__aNlshl`/`__aNlmul`/`__aNldiv` calls → `(int32)` arithmetic.

3. **Check for an f15 counterpart.** `matched_routines.tsv` maps routine_N →
   f15 name. If present, adapt `f15se2-re/src/*.c` — usually nearly identical;
   constants and a few globals differ. This is where ~25% of ports come from.

4. **Otherwise seed from Ghidra.** `python3 tools/ghidra_src.py <name>` gives a
   rough decompile; FUN_1xxx maps to seg000 (`FUN_1000_585c` = seg000:0x585c).

5. **Name it, declare globals.** Add `routine_N realname # seg:off comment` to
   `conf/routine_names.txt`; run `tools/rename_routines.py` on the map. Add
   `extern` decls for every referenced `word_XXXX` (in the module .c or
   stubs.c) at the same data address the original uses, and prototypes for
   callee `sub_XXXX`s (stubs in stubs.c if unported).

6. **Write C that mirrors the disasm.** Same locals, same types, same
   statement order, same branch polarity. Do *not* "clean up" — the goal is
   the original compiler's exact output, not nice code.

7. **Verify.**
   ```sh
   python3 tools/portcheck.py src/egcombat.c spawnSamThreat
   ```
   This compiles **all** `src/*.c` modules with each module's recorded flag
   set, links a test exe with LINK.EXE, finds `_spawnSamThreat` in the link
   map, and runs `mzdiff` on the routine's byte range vs `map/egame.map`
   extents. `MATCH` = done. `MISMATCH` prints a side-by-side instruction diff.

8. **Iterate on codegen diffs** (the bulk of the work — see §4). Dump the
   generated asm with `/FcD:\t.cod` on a scratch `src/_t.c` containing a tiny
   reproduction; iterate there, it's much faster than full portcheck runs.

9. **Regression-check the module.** Changing module flags or shared decls can
   break siblings — rerun portcheck for every ported routine in the file
   (e.g. adding `/Oa` to egcombat.c required re-verifying all 4 routines).

10. **Commit** with the verified-count bump.

### English-variant overrides (`src_en/`)

The EN binaries reuse the RU sources for ~90% of routines, but a handful
diverge (data-layout aliases, added calls, signedness). Those live in
`src_en/<module>.c` — a full copy of the module with only the divergent
routines changed, leaving `src/` byte-exact against RU. portcheck picks the
override automatically:

```sh
python3 tools/portcheck.py src_en/egcombat.c spawnSamThreat \
    --srcdir src_en --exe /home/xor/games/f19/F19/EGAME.EXE \
    --map map/egame_en.map
```

`--srcdir` mounts the directory as `F:` in kvikdos; modules not overridden
still compile from `src/` (`D:`). `INCLUDE` must contain `D:\` so `F:` sources
resolve the project headers, and the `dosdir` pick must compare
`os.path.abspath(modpath[mod])` — a relative `modpath` silently falls back to
the RU module and reports stale mismatches (the bug that hid all src_en
results initially).

## 4. MSC 5.1 codegen field guide

Everything below was determined empirically from `.COD` experiments. This is
where porting time actually goes — matching the compiler's register allocation
and stack layout exactly.

### Prologue anatomy

```asm
push bp
mov  bp, sp
sub  sp, N      ; N/2 = number of int16 local slots (incl. register-var homes)
push di         ; di is committed to something, OR
push si         ; si is committed to something
```

Reading `sub sp,N` tells you the local-slot count *exactly* — this is the
first thing to match. Each `push si/di` reveals a register MSC committed a
value to. A `call __chkstk` instead of `sub sp,N` means that module was
compiled *without* `/Gs`.

### Register allocator (greedy, no global CSE reservation)

- MSC 5.1 allocates `si` to the **first** index/CSE expression that wants it,
  then `di`. It does **not** reserve si for a later dominant CSE — so if a map
  lookup `arr[row + col]` is evaluated before the hot index is computed, the
  map row gets `si` even if the hot index wants it later.
- `[bx+si]` vs `[bx+di]` in the original tells you which index register was
  committed first.
- A `register` declaration **reserves si (then di) for that variable** — this
  is the only reliable way to force the allocator off si for transient
  expressions. If the original uses `[bx+di]` while a hot value sits in si,
  the source almost certainly declared a `register` var.
- **But:** a `register` var always gets a separate "home" stack slot (raised
  `sub sp,N` by 2), even if it's never spilled and never memory-referenced.
  MSC never shares a register home with a dead local, and never spills a
  register var to its home for operand reads (`imul reg`, never
  `imul [home]`). `volatile register` does not defeat this.
  The home word is *dead* — nothing ever stores to or loads from it — so
  `mzdiff --loose` tolerates the extra `sub sp,2`: a `register` local is a
  legitimate match even when the original's frame is 2 bytes smaller.
  (EN `spawnSamThreat` matched this way: `register int16 off` binds si to
  `slot*24`, frees `di` for the map index, and only costs the dead home.)
- `v = v;` (self-assignment) evicts MSC's cached copy of `v`, forcing a
  recompute at the next use — used in EN `drawWeaponRadarInfo` to get the
  second `imul weaponIdx,14` where the first `si` value was already dead.
  Phantom `register` params are a codegen device only when callers push the
  extra arg count; when the original callers push fewer args (EN
  `drawWeaponRadarInfo` pushes 2), remove the param and use repeated
  expressions + self-assignment instead.
- So: `imul [bp-var_4]` in the original means the multiplier operand is a
  **plain memory local**; `imul si` means a `register` var held the operand.
- To get `imul [bp-var_4]` *and* a persistent `si` index without paying for a
  local home: declare `off` as a **`register` parameter** — it uses `[bp+N]`
  as its home, so `sub sp,N` is unchanged. Cost: MSC emits `mov reg,[bp+N]`
  in the prologue unless the param is redefined in the entry block (first
  basic block before any branch). A `register` *local* does the same job but
  always adds its own home slot.
- The phantom register param need not correspond to a real argument —
  `drawWeaponRadarInfo(idx,row,register off)` reads `[bp+8]` though callers
  push only 2 args. The `mov si,[bp+8]` init is a dead read (overwritten by
  `off = idx*14` before any use). This is the only way to commit `si` to a
  byte-offset index reused across a call boundary: a plain `off` local maps
  to `bx` (or `sub sp,2` for its home), and MSC recomputes `i*14` per
  statement rather than CSE-ing it into `si` unless two field reads share one
  expression. When the original keeps `si = i*stride` for `[si+base+fieldoff]`
  across calls, expect a phantom register param + the one dead init-load.
- If the original has neither an extra slot nor a param load, the si value is
  a committed *auto-CSE* and the control flow must be shaped so MSC keeps it
  (next bullet).

### Control flow shapes CSE lifetime

- `if (arr[slot].ttl != 0) return;` — an **early-return guard** makes MSC
  compute `slot*24` lazily into `bx` for the `cmp`, then recompute into `si`
  for the body.
- `if (arr[slot].ttl == 0) { ...body... }` — a **positive branch** commits
  `si = slot*24` *before* the guard and keeps it. The original binary tells
  you which polarity was used (`jz` to the end vs `jnz` to a return).
- Any intervening conditional *splits* the auto-CSE: `si` is recomputed after
  the branch unless the body is nested inside the positive `if` too.
- `sub_15689`-style routines show the other pattern: when the hot index is
  computed *before* the map check, si is already committed and the map row
  falls to `di` naturally.
- **`if/else` arm order = which arm falls through.** `if (x >= 0) {A} else
  {B}` emits `jl elseLbl` + A + `jmp end` + B (A in fallthrough position,
  B after the taken branch). `if (x < 0) {B} else {A}` emits `jge elseLbl`
  + B + `jmp end` + A — same semantics, opposite layout. Match the
  original's block placement by picking the comparison sense, not by
  `goto`s (`stepFlightModel`'s seeker `an =` block needed `>= 0`).
- **`jcc +5; jmp L` is jump-span relaxation, not a different control-flow
  shape.** Pre-386 has no long conditional jumps, so when a `jz`/`jnz` target
  lands >127 bytes away MSC emits the inverted `jcc +5` over a near `jmp L`.
  Before restructuring an `if/else` for a `jne;jmp` pair in the original,
  measure the arm: if the target is >0x7f past the jump it is just a relaxed
  conditional — the real divergence is whatever made the arm that big
  (EN `destroyGroundTarget`: the "impossible" layout resolved itself once a
  missing `makeSound(0,2)` call inside the loop's `if` grew the arm past the
  short-jump limit; conversely a source-side `if (x==0) goto else` gets
  *folded* back to `jz else` by MSC's jump threading, so `goto` cannot
  reproduce this pattern).

### Rotated loops: nested `if`s, not `continue`

- A `for` loop whose body is a chain of `if (x) continue;` statements emits
  the loop condition/stride math in a different order than the original's:
  MSC places the continue-guards inline and the back-edge lands on the
  increment, while the original often shows a *rotated* layout — the
  back-edge label preloads a constant (e.g. `mov ax,0xC` record stride)
  before falling into the loop head.
- The fix is the f15-twin shape: `for (i...) { if (rec.x != 0) { ...big
  nested body... } }` — a single positive `if` wrapping the whole body.
  MSC then emits the stride multiply at the loop head and the shared
  tail (`idx < count ? A : B` → `push ax; call`) after it, matching the
  original's block order exactly. `drawHudWorldOverlay`'s bullet-track
  loop needed this; with `continue` the walk diverged at the loop head.

### Shared conditional tails for identical arms

- When two `if`/`else` branches each end with `if (cond) v = 1;` and both
  conditions have the *same* emitted branch sense (two unsigned `>`-form
  tests), MSC 5.1 may merge the two conditional jumps into ONE shared
  `jcc` reached by an unconditional `jmp` from the first arm — the flags
  set by each arm's own `cmp` are consumed by the shared jump. Emitted
  order: `[then cmp] jmp sharedJcc; [else cmp] sharedJcc: jcc merge; flag`.
- The original more often keeps per-arm branches plus a shared *flag
  block*: `jbe→merge; jmp→flag` in one arm, `jnb→merge`+fallthrough in
  the other. To get that layout the two conditions must emit DIFFERENT
  branch senses (e.g. `jbe` vs `jnb`) — MSC then cannot merge them and
  each arm gets its own conditional + a `jmp`/fallthrough into the
  common `v = 1` block. Different senses fall out of the natural
  polarity of each test once the globals' declared signedness is right
  (see below), not from `goto` gymnastics — plain `if`s suffice.

### `/Oa` enables cross-call register CSE (no home slot)

- When the original holds a *computed value* in `si`/`di` across a `call far`
  and there is **no** extra `sub sp` slot and **no** register-param init-load,
  the value is a committed **register CSE**, not a `register` var. Write the
  expression twice (once per block) and let MSC common-subexpression it.
- This only fires under **`/Oa` (assume no aliasing)**: without it MSC
  recomputes the expression in the second block (`sub sp` shrinks, no
  `push si`/`push di`). With `/Oa`, MSC promotes the repeated
  `sxN-clipZ` into callee-saved `si`/`di` (so it survives the call) and spills
  the next two CSEs to anonymous stack temps.
- `drawClippedLineRegion` (seg000:0x8e12) is the canonical case: four
  `g_lineXX = sN-clipZ` stores appear in the page-1 block and again in the
  dual-page block. Under `/Os` alone MSC recomputed them (`sub sp,4`, no
  si/di); under `/Os /Oa` it kept `sx1-clipL`→`si`, `sy1-clipT`→`di`, and
  spilled `sx2-clipL`/`sy2-clipT` to `[bp-6]`/`[bp-8]` — byte-exact.
- This is how the routine needed **no** `register` keyword and **no** named
  coord locals at all: just `int16 clipH, clipW` plus the repeated
  expressions. Adding `/Oa` to a module is safe only after re-verifying every
  sibling (here `egui.c` moved `/Os`→`/Os /Oa`; all siblings still matched).
- `/Oa` also controls **far-pointer liveness across control-flow joins**:
  `main` (seg000:0x10) keeps `es:bx = commData` live from the `cmp
  es:[bx+78h]` guard, through the `jnz` skip edge, AND through the taken
  branch's post-call `les bx` reload — the two edges carry the *same* far
  value, so at the join MSC emits `push es:[bx+1Ah]` with no reload. Without
  `/Oa` the callee is assumed able to clobber the global `commData` pointer
  variable, the two edges can't be proven equal, and MSC inserts an extra
  `les bx` at the join label. If the original skips a `les` where control
  flow merges, try `/Oa` before restructuring.
- Related bool lowering: a `uint8` comparison against `0`/`1` that feeds an
  argument lowers to `sbb ax,ax; neg|inc ax` only when written as `x == 0` /
  `x != 0` — `x < 1`/`x >= 1` produce a `jnb`/`jae` branch instead. And the
  operand must stay in `al` (read the just-stored global under `/Oa`, which
  forwards `al`), with `ax` free for the bool — a `uint8` temp var adds a
  stack slot and shifts the result to `cx`.
- `&&`-chains and nested `if`s keep an index CSE (`si = idx*stride`) live
  across every gated block between the set and the use; splitting the same
  tests into separate `if ... goto label` statements forces MSC to recompute
  `idx*stride` into `bx` at each gate. `updateObjects` needed the gun gates
  and the smoke guards as nested/`&&` conditions for exactly this reason.
- Signed `int32` compares come in two expansions: `jg T / jge F` (jump on
  strictly-greater to the *true* block first) vs `jge T / jge F` (jump on
  greater-or-equal first). MSC picks per statement shape, not per condition:
  a nested `if (A) { if (B) ... }` lowers the inner conjunct to `jge`-first,
  while `if (x < y) goto fail` lowers to `jg $continuation` (i.e. `jg`-first)
  because the *true* target of `x < y` is a real label. `computeAimProjection`
  (seg000:0xc793) needed `if (-y2 >= y0) { if (y0 < y2) goto fail; ... }` —
  the inner `<`-goto emits `jg` into the guarded body, matching the
  original's `cmp dx,[hi]; jg ok; jge ...; cmp ax,[lo]; jnb ...` sequence.
- Signed vs unsigned `>>` on a 32-bit leaf selects `__aNlshr` vs `__aNulshr`;
  a `(uint16)` operand anywhere in the chain flips the leaf to unsigned.
  Zero-extend a word into a *signed* long as `(int32)(uint16)x`.
- `K - longVar` lowers as `sub ax,ax; mov dx,Khi; sub ax,[lo]; sbb dx,[hi]`
  when K's high word is nonzero (`0x1000000L - v`), but `mov ax,K; cwd`
  when K fits in 16 bits (`0x100 - v`). Check the original's expansion to
  pick the constant.
- `x << 5` on a 16-bit field needs `(int32)(uint16)f << 5` to get
  `sub dx,dx` (zero-extend) before `__aNlshl`; a plain `(int32)f` emits
  `cwd` (sign-extend).
- Two-arm selects (`?:` or if/else) get scheduled into deferred blocks:
  which arm merges with the shared tail vs sits in a jump-stub depends on
  the *polarity* of the written condition, not the semantics. If the
  original shows `jg →di0-stub` with the di=1 arm merged into a shared
  call tail, write `cond ? 0 : 1` (jump-on-true to the 0 arm). If/else
  statements whose arms are single calls (`setDrawColor(A)` vs
  `setDrawColor(B)`) merge to one `push;call` tail — same layout rule.

### Small switches: cmp-ax chain + first-case shared tail

- A `switch` with ~4 cases and no `default` lowers to `mov ax,[sel]` plus
  sequential `cmp ax,imm` — NOT `cmp word [sel],imm` per test. An
  `if (v==a) .. else if (v==b) ..` chain keeps reloading `[sel]`; only the
  `switch` form caches it in `ax`.
- When every arm ends in the *same* call (`case k: f(shape_k); break;`),
  MSC tail-merges them into one `push ax; call` block. The **first-declared**
  case stays inline and hosts the tail (`cmp last; jne merge` falls into
  `mov ax,shape; push ax; call`); the remaining arms are deferred as
  `mov ax,shape; jmp tail` stubs emitted in **declaration order**, while the
  compares always emit in ascending case-value order. `renderFrame`
  (seg000:0x33d9) needed declaration order `0x44, 0x41, 0x43, 0x42` to put
  the front-shape arm inline with the tail and defer `rear,right,left` —
  the original's stub order proves the source declaration order.
- A multi-arm `if/else` can likewise get the "jump-on-true" layout
  `jz trueArm; jmp falseArm` when the false arm is emitted deferred far
  away; write `if (!bit) {nearChain} else {deferredArm}` — the inverted
  source condition puts the near chain on the `jz` target and the far arm
  on the fall-through `jmp` (`renderFrame`'s `&0x40` plane arm).
- When the ORIGINAL shows a big arm deferred past the merge (`jcc →farArm`,
  small arm inline), the source form decides which arm defers — not the
  semantics. `if (a>K || b>L) {small} else {big}` emits `jg →small` on the
  first disjunct and `jng →big-deferred` on the last (jump-on-false to the
  deferred else). The equivalent `&&` form (`a<=K && b<=L ? big : small`)
  keeps `big` inline and defers `small` instead — wrong layout. Check the
  f15 twin's written polarity; `updateFrame` needed `||`/`>` form.
- A select that STORES once at the merge — `compute ax; jmp m; sub ax,ax;
  m: mov [v],ax` — is `v = cond ? expr : 0` (value-producing `?:`), not
  `if (cond) v=expr else v=0` (stores per arm).

### Big dispatcher switches: the cold-label queue (keyDispatch, seg000:0xd4c6)

`keyDispatch` (~0xA2B bytes, a 60-case scan-code dispatcher with a gated
waypoint-panel sub-switch) exposes MSC's deferred-body mechanism in full:

- The compare chain emits first, in tree order. **Only the first-declared
  case body lands adjacent** to it (`jz short body`); every other case's
  body is queued and emitted later, past the switch continuation, in
  **declaration order**. Branch polarity in the chain follows placement:
  adjacent → `jz L`, queued → `jnz skip; jmp L`.
- A `case k: goto L;` stub emits NO body — the dispatch `jnz;jmp` jumps
  straight to `L`, and `L` is queued by the goto. Labels that are only
  `goto` targets emit in the cold zone in **first-reference order**
  (which is declaration order of the cases that goto them).
- **`case` labels can nest inside another case's body.** In the original,
  `case 0x5032:` sits inside `case 0x5000`'s body, right after its
  joystick gate: `case 0x5000: if (joy==0) goto done; case 0x5032: wp+=..`
  — the gated case checks `commData->setupUseJoy` and falls into the same
  work body the ungated keypad scancode dispatches to directly. This is
  how gated/ungated pairs share a body with one `jnz` + fall-through.
- The waypoint section is a `switch` on `scanCode` whose bodies are all
  `goto` stubs into a **label farm at the tail of the switch's statement
  list** (after `default:`). Emitted order = stub declaration order:
  `[wpUp+wpRedraw, check5000+case 0x5032+wpDown, check4B00+case 0x4B34+
  wpLeft, ..., tickAdd, tickSub]` — check+work stay contiguous because
  the `case` anchors sit inside the same queued block.
- `if (x != 0) {A...goto L;}` without an `else` may emit `jnz`-inverted
  arms (else inline, then deferred). Writing the `else` explicitly
  restores canonical `jz` + then-inline + else-forward.
- `x = ++x & 3` emits `inc [x]; mov ax,[x]; and ax,3; mov [x],ax` — the
  embedded pre-op defeats the `op [mem],imm` fold. To get the same
  register round-trip WITHOUT an inc/dec, cast the lvalue's read to its
  own type: `x = (int16)x & 3` → `mov ax,[x]; and ax,3; mov [x],ax`
  (verified by isolated probe: every other no-op — `+0`, `|0`, `*1`,
  parens, hex literal — still folds to `and word`).
- `imul`/`inc` ordering inside a condition tracks expression order:
  `scale*0x14 < ++ctr` emits `imul; inc; cmp`; `ctr++; scale*0x14 < ctr`
  emits `inc; imul; cmp`.
- For `call`-tail cleanup merging (`call; jmp shared-addsp`): the anchor
  block is whichever `call`-tail's `add sp,N` gets emitted where the
  shared continuation is wanted — an if/else that calls the same function
  with different args (`if (m!=1) f(1); else f(0);`) produces a merged
  `push ax; call` with `mov ax,K; jmp sharedPush` stubs, and its own
  `add sp,N` then anchors the dedup'd cleanup for neighboring tails.

### Union members vs plain word lvalues

`mov si,[bx+flags]; mov ax,si; test al,4 ... test ax,0x140` — the original
loads a flags word into `si`, copies it to `ax` so the byte test can use
`al`, then reuses `ax` for a later word test. The `mov ax,si` binds `ax` to
the *word lvalue* only when the operand is a plain `int16` lvalue:

- `u.w & K` where `w` is a `union { uint16 w; uint8 b[2]; }` member, and a
  `register` local copy of `u.w`, both keep the second test on `si` — the
  `mov ax,si` copy binds `ax` to the byte-cast node instead.
- A plain `int16 flags` field, or a word lvalue manufactured by dereferencing
  a casted address — `*(int16*)&u.flags` or `*(int16*)&u.flags.b[0]` — binds
  `ax` to the word, so `(uint8)*(int16*)&u.flags & 4` emits `test al,4` and
  `*(int16*)&u.flags & 0x140` emits `test ax,0x140` (byte-exact in
  `updateObjects`'s tail dispatcher). Verified `fireGroundThreat` shows the
  same pattern: `*(uint8*)&g_planeTable[i].flags` on a plain `int16` field.

### Addressing modes

- `g_projectiles[slot]` (struct, size 24) → `mov ax,24; imul [bp-var]; mov si,ax`
  then `[si+fieldoff]` — si holds the *byte offset*.
- `*(int16*)((char*)base + off + f)` — byte-cast indexing folds base+field
  into the displacement → `[si+0x5430]` directly, and keeps `off` (the byte
  offset) register-resident when declared `register`. `g_arr[off]` (int16
  index) instead emits `mov bx,si; [bx]` — the indexing *form* decides
  whether si is used directly.
- A `register` var used for int16 array indexing still produces
  `mov bx,reg; arr[bx]`; byte-cast `*(charptr)` indexing uses the register
  directly (`[si+off]`).
- When the original keeps `si = i*stride` as a **region-wide** index (bound
  once, then reused `[si+base+off]` across calls and blocks), a plain
  `arr[i].f` is not enough: if two uses hit the *same* field, MSC CSEs
  `&arr[i].f` instead (`add si,arr+off`) and the merged recompute sites fall
  back to `bx`. The fix is a **scoped `register` byte-offset local** —
  `{ register int16 base = i*stride; *(int16*)((char*)arr + base + off) }` —
  declared in a nested block so si is freed at its end and the extra home
  slot overlaps the dead range instead of growing `sub sp`. Reassigning it
  (`base = i*stride;` again) re-emits `mov ax,stride; imul var; mov si,ax` —
  the same rematerialization the original performs after a call — while a
  still-live register would have pushed the fresh value to `di`.
  `updateThreatTargeting`'s impact block needed exactly this (test, both
  `weaponIdx` reads, and the post-`hudMessage` recompute all `si`-bound;
  the `cluster_imp` code sits *outside* the register scope so its own
  `scanb*18` gets `si` back).

### Locals & the name hash

- Local stack slots are assigned by **variable-name hash**, not declaration
  order: `bucket = sum(name bytes) % 16`, buckets allocated ascending;
  within a bucket, declaration order maps first-declared to the bucket's
  *deepest* slot and last-declared to its *shallowest*. Reusing f15's exact
  identifier names is the easiest way to reproduce a frame; otherwise pick
  names whose hash buckets land on the right offsets, then fix the order
  inside each multi-var bucket by permuting the declarations
  (`stepFlightModel`, seg000:0x215c — 25 slots).
- Dead stores still get slots and still emit `mov [bp-var],const` — e.g.
  `spawnSamThreat`'s `specIdx = 0x21` is stored at `var_2` then never read.
- **Scope controls slot order.** Vars at function-block scope are pooled at
  entry and ranked by hash; a var declared inside a nested block is allocated
  *lazily* — it gets the next free slot only when its block opens, *after* all
  outer vars. `destroyGroundTarget` (seg000:0x790e) is the case: `eventType`
  stays at `bp-2` only because the loop counter `slot` and `symbol` are
  declared inside the `if(!(flags&0x80))` block — at function scope `slot`
  out-ranks `eventType` for `bp-2` every time. When a genuinely-hot local sits
  at a *higher* offset than a cooler one, suspect it was declared in an inner
  block, not that the hash model broke.
- `x = -y + K` compiles to `neg ax; add ax,K` — write unary minus, not `K-y`.

### Signedness leaks through shifts

- `sar reg,cl` → signed `int16` global; `shr reg,cl` → `uint16`. The original
  routine's shift opcode fixes the global's type (e.g. `g_viewX_`/`g_viewY_`
  are `uint16` where `shr` is used, `int16` where `sar`).

### Signedness steers commutative-add folding

For `posX = mul_result + memleaf`, MSC either folds the leaf
(`add ax,[di+ofs]` — product stays in ax, sum in ax) or materialises it
(`mov cx,[di+ofs]; add cx,ax` — sum in cx). Source order does **not** decide
this — `a*b + m` and `m + a*b` emit identically. The decider is the leaf's
*declared* signedness:

- leaf `int16` → materialised in `cx` (`mov cx,[mem]; add cx,ax`).
- leaf `uint16` → folded (`add ax,[mem]`).

`spawnEnemyAircraft` (seg000:0x6ad2) is the case: `posX = g_northSouthSign*3
+ g_planeTable[objType].mapX` only emits `add ax,[di-0x7f36]` once
`MapTarget.mapX/mapY` are `uint16`. So `add ax,[mem]` after `imul` proves the
added field/global is unsigned even when no `shr`/`sar`/`mul` ever touches it.
(A bare local or global *scalar* uint16/int16 leaf folds either way — the
divergence shows on indexed `[reg+ofs]` operands.)

### Byte-level strength reduction on 16-bit ops

- `w += 0xN00` (low byte zero) lowers to `add byte [w+1], N` — write the word
  add, not a byte-field poke (`((int8*)&w)[1] += 1` emits `inc`, not `add`).
- `x & 0xFFF` lowers to `and ah, 0xF` (high-byte mask), same trick.
- `x << 8` lowers to `mov ah, byte ptr x; sub al, al` when the operand is
  already in a byte-addressable form.

### Expression-level CSE and operand order

- A duplicated 16-bit subexpression inside one `&&` — e.g.
  `(uint16)(a-b)*c >= K && (uint16)(a-b)*c <= L` — CSEs to a single `mul`
  with the product parked in `di` (`mov di,ax; cmp di,K`), no store. A
  `register` local gives `si` instead (wrong); an unnamed temp off a plain
  local stores to `[bp-var]` (wrong). Repeat the expression verbatim.
- `mul` operand order for 16×16: the *left* operand goes to `ax`
  (`mov ax,[op1]; mul [op2]`); casts mark the casted operand composite and
  can flip the choice — write the order that matches, drop unneeded casts.
- `a = b = 0` emits `sub ax,ax; mov b,ax; mov a,ax` (rightmost first).
- For an unsigned compare between two memory globals, the *right* operand
  loads into `ax`: `if (a >= b) goto L` emits `mov ax,[b]; cmp [a],ax;
  jnb L`. This only holds when both globals are *declared* `uint16`;
  casting `int16` decls with `(uint16)` at the site does not flip the
  eval order — MSC still picks its own order (`mov ax,[a]; cmp [b],ax;
  jbe`) no matter how the operands are written. Declared signedness, not
  casts, steers compare emission.
- Compare-operand direction rescue: `a < b` where `b` is the heavier
  expression (call result parked vs a `mul` temp) canonicalizes to
  `cmp a,b; jbe` (the `b > a` body form) no matter how the `<`/`<=`/`>=`
  is written. Writing it as a subtraction test `a - b < 0` makes MSC emit
  the compare in source order: `cmp a,b; jnb` — matching e.g. EN
  `drawHudWorldOverlay`'s `bearingToStore(x) - ((uint16)g_viewZ>>5)*5 < 0`
  (`cmp bx,ax; jnb`). Same bytes come from `(u = f(x)) < y` assignment-in-
  test forms; the subtraction is the simplest spelling.
- `-(x >> 5)` on a signed `int16` decl emits `sar; neg`; EN wants unsigned
  `shr; neg` — cast the *shifted operand*: `-((uint16)g_viewZ >> 5)`. The
  same cast fixed the `* 5` mul site one block earlier.
- `-((uint32)(uint16)x - K)` emits the full 32-bit negation
  `sub dx,dx; sub ax,K; sbb dx,dx; neg ax; adc dx,0; neg dx` — the unsigned
  32-bit subtract form, NOT `cwd` sign-extension.

### 32-bit arithmetic

`(int32)` ops emit the `__aNl*` helpers (`__aNlmul`, `__aNldiv`,
`__aNlshl`, …). For a flat chain `a*b*c/d` MSC pushes **all** leaf operands
onto the stack first, then calls the helpers — so the listing shows a run of
`…cwd; push dx; push ax` blocks followed by the `call __aNl*` run.

```asm
; a * (b<<4) / c   →   push c(cwd); push b(cwd); shl by cl (__aNlshl);
;                      push result; __aNlmul; push; __aNldiv
```

**Push order is *not* raw source order.** MSC schedules the leaves itself:

- The **divisor is pushed first** (for `X/Y` in a chain, `Y` leads the pushes
  even though it is the rightmost operand in the source).
- The remaining numerator leaves follow in an order MSC picks by leaf
  cost/dependency — permuting the source order does *not* change the emitted
  order (verified: all six orderings of a three-factor numerator produce
  identical code). You cannot fix the order by reordering the expression.
- **A `(int32)` cast makes a distinct leaf** — it is a different expression
  node from the bare operand, so MSC reloads it instead of reusing a register.
  But a cast is also a *composite* node that the leaf scheduler ranks
  differently: it can push a leaf later than an equivalent clean leaf.

**Signedness of the whole chain follows the operand types.** MSC 5.1 is
*unsigned-preserving*: a single `uint16` operand anywhere in the `int32`
chain flips every helper to `__aNulmul`/`__aNuldiv`. Sub-typing:

- `int32 - int16`: subtrahend is sign-extended → a register-register
  `mov cx,ax; …; sub ax,cx; sbb dx,bx` pair (the operand is materialised in
  `cx:bx`).
- `int32 - uint16`: subtrahend is zero-extended → the cheap
  `sub ax,[mem]; sbb dx,0` — but the result is `uint32`, i.e. unsigned.
- `(int32)(a - b)`: a 16-bit `sub` *then* `cwd` — different opcode order.

**Case — `computeThreatRangeBearing` (seg000:0x5689).** Target leaf order:
`lethality`(divisor) → `dangerTier+ms*2+1` → `lethality-distance` → `terrain`,
with `lethality` loaded twice (no CSE) and `sub ax,[mem]; sbb dx,0` for the
difference. That push order is what a plain `terrain * ltRange * dt / lt`
chain *should* give (divisor first, then numerator in reverse source order).
The conflict:

- `(int32)lethality - (int16)distance` → register sub (wrong form) but clean
  leaf → correct order.
- `(int32)lethality - (uint16)distance` → correct `sub/sbb` form and order,
  but the leaf is `uint32` → `__aNul*` (unsigned) helpers.
- `(int32)((int32)lethality - (uint16)distance)` → correct form + signed, but
  the inner `(uint16)` cast makes a composite leaf → deferred to last (wrong
  order).

The winning combination: declare `distance` itself as **`uint16`** (not a
cast), and write the leaf `(int32)((int32)lethality - distance)`. The operand
is a clean `uint16` *variable* (zero-extends to `sbb dx,0`), the result is
re-cast to `int32` for signed helpers, and — because the leaf carries no inner
cast node — it stays in the si-group and pushes third, byte-exact. The lesson:
a local's *declared* type and a `(cast)` on its use produce different leaf
nodes that MSC schedules differently; match both the type and the cast
placement to the original's instruction shape.

Match the push order in the disasm to pin down operand order — the dividend
is pushed *last* (top of stack) for `__aNldiv`.

**`__aNlmul` operand order — the `x & x` "heavy rank" trick.** For
`term32 * var32` the compiler is free to pick which operand is arg2
(pushed/evaluated first): it prefers the operand that is *cheap* — a value
already live in `ax` (e.g. a just-stored local) — and evaluates it first.
The original often shows the complex term pushed first instead, with the
variable *reloaded* (`mov ax,[bp-var]; cwd`) even though `ax` still held it.
`(int32)(v & v)` solves this: it folds to a plain `mov ax,[v]` load but
ranks the operand *heavy*, so MSC makes it arg1 — pushed last, evaluated
after `ax` was clobbered by the term — exactly the original's emit
(f15's `speedCalc & speedCalc`; F19 `stepFlightModel` `spdx & spdx`).
An `int * int` with no 32-bit operand anywhere collapses to native
`mul reg16`; the `(int32)` cast on the `&` operand is what keeps the mul in
the long domain. For `((u16term) * (int32)(v & v)) >> K` the product comes
out `uint32`-flavoured, giving `__aNulshr` where the original used it.

**Nested `ldiv`.** `((int32)a - (int32)b) / 0x10 / g` emits
`cwd`-extension of each sub operand + `sub ax,cx; sbb dx,bx` (the second
operand's sign-extension kept in `bx:cx`), then `ldiv(diff, 0x10)` inside
`ldiv(result, g)` — the outer divisor pushed first. A plain `int/int`
division by 16 instead folds to the inline `abs`-shift idiom
(`sub; xor ax,dx; sub ax,dx; sar 4; xor; sub`); when the original shows a
real `call __aNldiv`, cast the operands to `int32`.

### Compiler flags are per-module

`tools/portcheck.py` `MODULE_FLAGS` records what each original module was
built with — determined empirically, since flag choice is visible in codegen:

- `/Os` vs `/Ot`: visible in routines with early returns (shared vs inlined
  epilogue sequence).
- `/Oa` (assume no aliasing): **critical** where a global-derived value stays
  cached in a register across a store to a global. `samCanAcquireTarget` only
  matched under `/Oa` — without it MSC reloads `si` after every store to
  `g_projectiles[slot].field` because it must assume the store may alias the
  index computation. If a routine keeps an index in si across a global store,
  that module was compiled `/Oa`.
- `/Oa` also caches a global **pointer variable** across consecutive stores
  through it: `render3DView` loads `g_viewParams` into `bx` once for four
  `g_viewParams[i] = ...` stores (reloading only after each call). Without
  `/Oa`, every store could alias the pointer variable itself, so MSC reloads
  `bx` per store. No source-level workaround exists — a local pointer copy
  would sit in si/di or `[bp-x]`, never `bx` (bx is scratch, never a register
  var). When one reconstructed file needs `/Oa` but a sibling breaks under it
  (`setupViewport` gains a `push si`), the routines came from different
  original modules — split the file (`egrender.c` = `/Ot /Oa`).
- `/Gs` (no stack checking) vs `__chkstk` prologue.
- `/Od` (no optimization): unconditional `mov ax,N; call __chkstk; push di;
  push si` prologue even for routines with no locals/register vars — END's
  `enworld.c` is `/Od` (`readWorldData` probes 0 bytes yet saves si/di).
- Satellites can share basenames with different flag needs — `MODULE_FLAGS`
  accepts `'<exe>/<file>'` keys (`start/textfmt.c`, `end/textfmt.c` are
  `/Os`; EN `my_itoa` has a `for (k=5; k>0 && num[k]==0; k--)` scan loop
  that gains a stray `nop` pad under `/Ot`).
- Near→far pointer promotion via a named `far` local emits the canonical
  `mov [bp-4],off; mov [bp-2],ds` pair — START's `memAppend` does
  `farptr = ptr; movedata(FP_SEG(farptr), FP_OFF(farptr), ..., len)` where
  the first `push ds` argument is `FP_SEG(farptr)`.
- `uint16` field type vs `(uint32)` cast on an `int16`: only the former
  emits `sub dx,dx` (zero-extension). `(uint32)(int16)x` still sign-extends
  via `cwd` — START's `positionUnit` declares `x`,`y` `uint16` for this.
- Beware which binary is the reference: the repo-root satellite exes are
  the RU builds — EN verification needs `--exe $(F19EN)/<X>.EXE --map
  map/<x>_en.map` (a `--exe` path is used verbatim).

### mzdiff tolerances

`portcheck.py` runs `mzdiff --nocall --loose` and additionally tolerates:

- **data segment offset mapping conflicts** — globals live at different
  addresses in the test exe; the instruction stream still matched.
- **stray `nop`** — MSC aligns jump targets to even boundaries; pad placement
  depends on the routine's offset in the final exe.

- **16-bit wraparound aliases** — an absolute `[0x87B4]` and an indexed
  `[si-0x784C]` encode the same data address mod 64K (0x87B4 ≡ -0x784C).
  `OffsetMap::dataMatch()` canonicalizes both sides `& 0xffff` before
  keying the offset map (patched in `mzretools/src/analysis.cpp`), or the
  two encodings bind as distinct keys and collide on the shared target
  offset. `drawHudWorldOverlay` uses both forms for `g_targetSlots`
  fields within one routine.

- **data offset *collisions* mean variable aliasing, not bad codegen** — when
  the instruction stream matches but mzdiff reports `Data offset mapping
  A->B collides with existing A->C`, the two versions store to different
  globals at that site. EN reuses storage the RU source kept separate:
  `findStoreAtGrid` writes the selection scratch into `g_storeDefs[0]`
  (slot 0; its search loop starts at index 1), and `initStoreData` writes
  `g_nameTab[0]` rather than a separate `g_nameTabBase`. Diff the *operands*
  at the colliding ref to see which global the original aliases.

After the spec'd walk drains, `checkMissedRoutines` re-seeds every unvisited
routine in the ref map, so a `--map` run effectively compares the whole map:
each missed routine resolves its target via opcode-pattern search, and a
false-positive match (e.g. `applyGravityFall` resolving inside
`computeAttitudeAngles` when its true location was already claimed visited)
reports a bogus MISMATCH. Verify the routine itself with a direct range diff:

```sh
mzretools/build/mzdiff EGAME.EXE:0xSTART-0xEND \
    build/TEST.EXE:0xSTART-0xEND --nocall --loose
```

`map/egame.tgt` is the discovered-target-map cache mzdiff rewrites each run —
gitignored; never commit it.

Everything else must be byte-identical: same opcodes, same registers, same
`[bp-N]` displacements, same operand order.

## 5. Case study — `spawnSamThreat` (seg000:0x585c)

A real example of how deep a port can go. Periodic SA-14 spawner:

```asm
sub sp,8 ; push di ; push si
...  mov di,(viewY>>11)<<4 ; mov bx,viewX>>11 ; test [bx+di-79E6h],10h
...  mov ax,tick ; sar ax,4 ; and ax,7 ; mov [var_4],ax   ; slot
...  mov [var_2],21h                                      ; specIdx (dead)
...  mov ax,18h ; imul [var_4] ; mov si,ax                ; si = slot*24
...  cmp [si+5430h],0 ; ...body writes [si+off] fields, calls, 32-bit ttl...
```

What the bytes demand:

- `imul [var_4]` ⇒ `slot` is a **plain memory local** (a `register` var would
  emit `imul reg`).
- `si = slot*24` persists across `randomRange`/`computeBearing`/`rangeApprox`
  calls *and* across the `if (range < threshold)` branch ⇒ committed CSE,
  which requires `/Oa` on the module **and** positive-branch nesting
  (`if (ttl == 0) { if (range >= thr) { ... } }`) so the CSE isn't split.
- `di` for the map row while `si` is committed ⇒ the allocator must see si
  spoken for. With a `register int16 off` (`off = slot*24`, byte-cast field
  access) you get *perfect* codegen — `[bx+di]` map, `imul [var_4]`,
  `mov [var_2],0x21`, `[si+5430h]` — but `sub sp,10` because `off` takes a
  home slot. The original's `sub sp,8` means si is a pure auto-CSE.
- `[bx+di]` vs `[bx+si]`: with si free at map-eval time, MSC gives the row si.
  The original's `di` implies si was globally committed — the hardest part of
  this routine (150+ compiler experiments to characterize: `register` always
  buys a home; `volatile` doesn't defeat it; homes never shared with dead
  locals; positive-branch nesting keeps the auto-CSE).

**Resolution — the `register` *parameter* trick.** A `register` local always
buys a home slot (`sub sp,10`), but a `register` *parameter* uses `[bp+N]` as
its home — no local slot, so `sub sp,8` survives. Declaring
`void spawnSamThreat(register int16 off)` and writing the body through `off`
(byte-cast `*(base+off+N)` field access) produced **instruction-for-instruction
identical code to the original — all ~150 instructions** — except one: MSC
emits `mov si,[bp+4]` in the prologue because `off`'s first store sits after
the guard branches, not in the entry block. The load is a dead read (si is
overwritten by `mov si,ax` before any use), so the routine is semantically
exact. The prologue param-load is skipped only when the param is redefined in
the entry block; here it can't be, and no other construct dedicates si without
a home or a load — so byte-exact is unreachable for this routine under MSC 5.1.

**EN postscript.** The EN binary's `spawnSamThreat` (seg000:0x56ec, the
`duplicate` map entry) is the same routine *without* the phantom param —
callers push no arg. The winning EN form is `register int16 off` as a plain
local: it dedicates si to `slot*24`, pushes di for the map index, emits no
param load, and the only cost is the dead home word (`sub sp,0xa` vs the
original's `sub sp,8`), which `mzdiff --loose` tolerates as literal/frame
remap. So the *instruction stream* is byte-exact for EN even though the RU
binary keeps its one dead-load mismatch; the register-local home is never
read or written, so the runtime stack usage differs by two inert bytes.

Lesson: when a port resists, isolate to a `src/_t.c` repro (one function,
minimal externs), compile with `/FcD:\t.cod`, and diff the `.COD` asm against
the .lst — full portcheck cycles are for confirming, not exploring. And when
mzdiff desyncs on a single inserted instruction, fall back to aligning the
`.COD` against the `.lst` by hand — the rest of the stream may already match.

## 5b. Semantic equivalence via Z3 (`tools/z3check.py`)

For routines that cannot reach byte-exactness (`spawnSamThreat`,
`drawWeaponRadarInfo` — each carries the one extra prologue `mov si,[bp+N]`
described in §5), `portcheck`/mzdiff can never say MATCH. A second, semantic
gate uses the Z3-backed SSA comparator in `~/vextest`:

    python3 tools/z3check.py src/egcombat.c spawnSamThreat

The driver: (1) builds the module test exe via `portcheck.py` machinery,
(2) runs `discover`/`ssa`/`make-mapping`/`compare-ssa` against the oracle
`EGAME.EXE` + `map/egame.map`, then (3) **normalizes linked-layout
differences** in the candidate SSA before comparing — the test exe links only
one module, so every global and string sits at a different data offset than
in the oracle:

- *named globals*: name → (candidate offset from the test exe's LINK map,
  oracle offset from the `@0xXXXX` comment on its C declaration); a candidate
  constant is rewritten to the oracle offset only when the same value does
  **not** also appear as a real literal in that oracle function (a `0x800`
  store was once corrupted by colliding with a global's candidate address).
- *string literals*: candidate constant → NUL-terminated bytes in the test
  exe's data segment must equal oracle bytes at the oracle offset; for
  non-unique contents (`" km"` occurs 5×) a positional pass pairs same-named
  SSA parts in entry order and remaps only when contents match.
- SSA part boundaries still shift where the extra prologue load splits call
  blocks — those parts report `part_boundary_mismatch`/`candidate_ssa_missing`
  and are expected refusals, not diffs.
- `call_target_unproven` refusals are harness artifacts: the callee isn't in
  the module's test exe, so the candidate resolves to a stubs.c stub body
  (≤20 bytes, `558bec`- or `33c0`-headed) while the oracle resolves to the
  real routine. z3check reclassifies these as `call_target_stub` (benign)
  only when the resolved candidate target matches that stub shape — a
  refusal on a real, non-stub callee still counts as DIFFERS.

Result: `spawnSamThreat` **17/17 compared parts proven** (5 boundary refusals);
`drawWeaponRadarInfo` **18 proven, 1 part mis-paired** (oracle's merge-point
`push ax` against the candidate's `" impul"` arm — contents agree), 2 refused.
Verdict `PROVEN(modulo-layout)` means every compared part is equal and any
failures are only `memory_expr_changed` — the layout-artifact class; confirm
against `build/z3cmp/*.compare.json` details before trusting a nonzero count.
`make verify` / `portcheck` remain the byte-exactness gate; z3check only says
the compiled behavior is identical.

## 6. Naming / bookkeeping

- `conf/routine_names.txt`: `routine_N name # seg:off description`. After
  verifying, `tools/rename_routines.py map/egame.map` writes names into the
  map so later routines see callees by name.
- Keep a verified list (AGENTS.md) and bump the count on commit.
- File the port under the module matching the original source-file split
  (egcombat.c, egframe.c, egtacmap.c, ... — same split as f15se2, since the
  modules compile with different flag sets).

### Identifying the non-ported tail (how every `routine_*` was named)

Once all C was ported, ~120 `routine_*` remained. They were classified —
not ported — by these methods:

1. **IDA FLIRT names** — the lst already carries `__cinit`-style sig hits
   for most of the CRT tail; harvest by scanning `proc` heads at routine
   addresses (normalize case: lst addrs are uppercase).
2. **SLIBCE.LIB byte matching** — where FLIRT is silent, parse the MSC 5.1
   library directly: `.LIB` = 32-byte header (dict offset/count at +6/+10),
   then OMF records of size `reclen+3` (4-byte pages, dict stored in the
   LAST page); each module's LEDATA/FIXUPP gives text bytes + relocation
   masks; each pubdef gives name+offset. Relocation-masked byte equality
   against exe routine bytes identifies `_write`'s inner DOS writer,
   `_brkctl`'s segment-table search, `__nfree`/`__nmalloc` (IDA's
   `unknown_libname_1/2`), etc. FIXUPP fields are varints; a truncated
   record must not abort the parse.
3. **f15se2 twin matching** — for seg001/seg002 asm: same toolchain, same
   cluster in `egseg1.asm`/`egseg2.asm`. Far-thunks are disambiguated by
   their direct callee (`sub_2XXXX` linear addr → proc-head map), not by
   mnemonic similarity (all thunks look alike). Spot-check any score<0.9
   match against the f15 body.
4. **All-zero regions are data** — seg004 (~49KB) is entirely zeros in the
   image: its ~55 "routines" are map artifacts in reserved BSS/overlay
   staging, renamed `bss4_*`, never ported.
5. `crtPad0`: a 16-byte zero pad before `_main` (page-aligned init code
   follows the pad — linker/section padding, not a routine).

### Regenerating the f15 cross-reference (`matched_routines.tsv`)

`mzdup` does signature matching between the two binaries:

```
mzretools/build/mzdup sig/f15_egame.sig EGAME.EXE map/egame.map
# -> map/egame.map.dup: `duplicate` flag + "# Routine X ... duplicate of
#    routine Y ..., differs by N instructions" comment per match
```

249 f15 signatures (≥15 instructions, ≤10% distance) → 104 F19 matches
(25% of the image's instructions — the shared engine). Routines under 15
instructions are skipped, so `strcpyFromDot`↔`replaceExtension` (0x2A) came
from the old run's comment. Extract to TSV by pairing each `duplicate` flag
with its preceding comment, then annotate `f15_source` by grepping
`f15se2-re/src/` for the twin name (`.asm` hit ⇒ asm on both sides, keep
asm). Result: 59 dist-0 + 26 dist-1 signature-identical/near matches —
independent confirmation of the seg001/seg002 asm naming.

Beware: `sig/egame.f15.map` is **f15's own map**, not f15-names-in-F19 —
joining by seg:offset across the two binaries misleads (e.g. the seg002
far-thunks sit in swapped order: F19 layoutFar@0x0A/gaugesFar@0x0E vs f15
gaugesFar@0x0A/layoutFar@0x0E).

## 7. Pitfalls

- Don't port the register-convention asm clusters (seg001/seg002 graphics
  pipeline, int handlers, crt0) — they have no C prologue pattern.
- Don't trust Ghidra's types — it invents `undefined`/`uint` that flip
  `sar`/`shr`; check the shift opcode.
- Don't reorder statements to "simplify" — every reordering changes register
  pressure and usually the bytes.
- Don't fight the name hash by reordering declarations — rename vars instead.
- Per-module flags must be re-verified whenever changed: a flag that fixes
  one routine can break its siblings.
- `_t.c`/`T.COD` scratch files are for experiments only — keep them out of
  commits.

## 8. Driver overlays (*GRAPHIC.EXE / *SOUND.EXE)

The graphics/sound drivers are ordinary MZ executables but **not** normal
programs: the load image starts with an `OvlHeader` (see
f15se2-re/src/overlay.c) — an embedded signature string
(`"MGRAPHIC.EXE09-19-88"`), two segment pointers, and a **jump table of
near offsets into the code segment**. Exported routines are reached
through the table only, so `mzmap`'s reachability scan finds just `start`.
`tools/drvmap.py` parses the header and emits an MS-link-format seed file;
feed it with `mzmap --linkmap`:

```sh
python3 tools/drvmap.py MGRAPHIC.EXE map/x.link conf/gfx_slots.txt
mzretools/build/mzmap --overwrite --linkmap map/x.link MGRAPHIC.EXE map/x.map
```

Header layout (load-image offsets): `description[0x18]`,
`code_segment`@0x18 (paragraph of the code seg), `base_segment`@0x1a,
`first_slot`@0x1c, `size1`@0x1e, `size2`@0x20 (code-seg length),
`jump_count`@0x22, then `jump_count` near-offset words @0x24. A slot index
+ `code_segment` gives the far entry point
(`overlay_functionAddress(seg, n)` in f15se2-re).

### Cross-game findings

| driver | f19-en img | f15 img | body match | signature |
|---|---|---|---|---|
| MGRAPHIC | 0x2728 | 0x275a | ~94% of handler bytes | both `MGRAPHIC.EXE09-19-88` |
| TGRAPHIC | 0x2ab4 | 0x2abe | ~96% | both `TGRAPHIC.EXE09-19-88` |
| EGRAPHIC | 0x3bcb | 0x3be9 | ~74% | both `EGRAPHIC.EXE07-27-88` |
| CGRAPHIC | 0x3b3c | 0x3c4f | ~69% | both `CGRAPHIC.EXE07-27-88` |
| ASOUND   | 0x2558 | 0x2bfe | ~25% (different gen) | f19 ` STEALTH.EXE11-16-90`, f15 `F15 II AdLib 3-14-91` |
| ISOUND   | 0x8df  | 0x163e | ~13% | f19 ` IBMSNDS.EXE09-28-88`, f15 ` F15 II IBM 03-22-91` |
| RSOUND   | 0x2a4e | 0x2c6d | ~57% | both Roland MT-32, 11-90 vs 3-90 |

- f19-en and f19-ru **sound drivers are byte-identical**; graphics drivers
  differ en↔ru (~21% for MGRAPHIC — localization touched the code too).
- MGRAPHIC is the **same source revision** in f15 and f19 (identical
  signature date): f15's slot table = f19's table +2 shift on nearly every
  entry; per-slot handler bytes match 86-100%. The f15 `src/slot.h` ABI
  (84 gfx slots) names f19's handlers directly — verified per-slot.
- ASOUND (f19) is the F-117-generation `"STEALTH.EXE"` driver: 9 slots
  `0x64-0x6c` matching f15se2-ex's `asound_model.h` ABI exactly (setup,
  shutdown, dispatch_sound, play_intro, enable/disable drone,
  set_drone_pitch, timer_tick, noise_tick — no 0x6d `play_sample`).
  f15's ASOUND is a different build (10 slots, jump-stub table).
- f15se2-re loads the original MGRAPHIC.EXE as a binary overlay;
  `gfx_impl.c` is an SDL reimplementation, not a port. `asound/` in
  f15se2-ex-integration is a clean-room model of the same ABI.
- No f15 release ships a `*GRAPHIC.EXE`/`*SOUND.EXE` built from source —
  driver reconstruction here would be the first.

### Skeleton emission (`tools/drv2asm.py`)

`drv2asm.py EXE map/x.map out.asm` emits a byte-exact MASM skeleton:
seg000 = OvlHeader fields as `dw seg`/`dw offset` forms + verbatim data
bytes; seg001 = one `proc far` per map routine whose body is `db` bytes +
an ndisasm comment. `make drivers` / `make verify-drivers` reproduce both
drivers with 0 load-image diffs.

### Ada Script listings (`--lst`, `tools/asmfix.py`, `tools/drv2idc.py`)

`~/vextest/ada.py EXE --work-dir W --full --xrefs` produces `W/X.lst` +
`W/X.asm` + `W/X.map`. Unseeded it doesn't know the OvlHeader layout:
header/table bytes decode as instructions and its .asm has ~240
unresolvable labels. `tools/drv2idc.py EXE map/x.map conf/x.idc` fixes
that by emitting an IDC seed script (`ada.py -s conf/x.idc`) that

- redeclares the image as seg000 DATA (header/table) + seg001 CODE,
  renames them, sets `ds` for code refs,
- marks the 0x18 header fields + jump-table words as data items,
- `add_func`/`set_name`/`set_func_flags(..., FUNC_FAR)` every map routine.

With seeds, ada reaches ~88% coverage on MGRAPHIC (vs 57%) and emits
`proc far` handlers plus a mzmap-format `.map` (tools/ada_script/
map_writer.py in vextest — extents from the functions table, NEAR/FAR from
FUNC_FAR/far-xref/retf-or-iret terminators; disjoint tail-chunks are not
attributed). Regenerate everything with `make ada-driver-lsts`.

`drv2asm.py --lst x.lst` instead uses the lst **per instruction** inside
map-defined routine extents, keeping a mnemonic only when it can be
re-emitted exactly. Name resolution:

- `loc_X/sub_X` at emitted code addrs stay symbolic (labels emitted at
  their offsets inside procs).
- ada names in the header/data region (`start`, `loc_100D8`, ...) are
  emitted as `label byte|word` at their image offsets in seg000 —
  symbolic refs keep MASM's relocatable mod=10 disp16 encoding (numeric
  displacement operands get minimized to disp8 by uasm and break).
- `call far ptr 1CDh:NNNNh` (a same-segment overlay call) rewrites to
  `call <routine at seg001:NNNN>` — a far-typed proc, producing the same
  9A ptr32 + segment reloc.
- anything else (out-of-image targets, mid-instruction labels, unknown
  idents, 386+ `jcc near`) becomes `db` + the mnemonic as a comment.

Each emitted mnemonic carries a `; @OFF:LEN` tag. `tools/asmfix.py` then
repairs what still won't assemble exactly, driven by uasm's own `-Fl`
listing (emitted bytes per source line — no link needed, so a bad line
never desyncs the comparison):

1. asm-error lines -> db
2. emitted-length != tag length -> db (these desync the byte stream)
3. emitted bytes != img[OFF:OFF+LEN] -> db
4. link + compare_exe -> 0 diffs required (catches reloc/slot-table issues)

Result: MGRAPHIC = 879 mnemonic lines / ASOUND = 1625 mnemonic lines,
both byte-exact. Committed artifacts: `lst/mgraphic_ada.lst`,
`lst/asound_ada.lst` (IDC-seeded) and `conf/mgraphic.idc`,
`conf/asound.idc`. Regenerate with `make ada-driver-lsts` (needs
`~/vextest`), or manually:

```sh
python3 tools/drv2idc.py /home/xor/games/f19/F19/MGRAPHIC.EXE \
  map/mgraphic_en.map conf/mgraphic.idc
~/vextest/.venv/bin/python ~/vextest/ada.py \
  /home/xor/games/f19/F19/MGRAPHIC.EXE --work-dir /tmp/ada_mg \
  -s conf/mgraphic.idc --full --xrefs    # writes MGRAPHIC.lst/.asm/.map
```

## 9. English START.EXE binary patches (`sub_18F12`)

The shipped English START.EXE was patched after link — the patch is visible
by comparing with the Russian build, which still carries the unpatched code:

- seg000:0x918E — `call sub_10AE8` (menu-select widget, second call site)
  was replaced by `nop nop nop`. The orphaned `push ax` in front of it and
  the trailing `add sp,0xA` remain, so `ax` (still holding literal 0x5036)
  lands in `res` (`[bp-14h]`) — the widget never runs.
- seg000:0x91B9 — the guarded dispatch `mov ax,[choice]; cmp ax,[res];
  jz ACCEPT` became `mov [res],ax; jmp short ACCEPT` (+dead `jmp NEXT`),
  forcing the accept arm: `res = choice` is always taken.

Both patches are equal-size byte swaps, so no MSC source produces them.
The C port (`src_start/stmenu.c`) carries the unpatched semantics —
`if (res == choice) goto ACCEPT; goto NEXT;` — and map/start_en.map marks
both 3-/5-byte patch sites `U` so mzdiff skips them. Effect of the patch:
the briefing screen auto-accepts the randomly chosen mission without
running the selection widget.

## 10. `/Ot` branch-target alignment nops (`sub_12754`)

`src_start/stpanel.c` ports `sub_12754` (seg000:0x2754, 2755B) — the
per-row object/detail panel: `base`=item-table pointer, `row`=record index
(stride 0x32), `pg`=`int16*` into the sprite-param table. Flag byte at
record+0x30 selects panel families (`& 7`); a function-level
`register int16 id` pins `si` (reassigned `id = row*0x32` at each flag
re-read, reused for the f0-selects and the `pad4*14` index), which is why
the last test's bare `row*0x32` temp lands in `di` — two-register model.

The binary has 13 interior `nop` pads — first nop-heavy routine ported.
They are MSC `/Ot` branch-target alignment: every pad is a single 0x90
between a `jmp`/`jcc` and an odd-offset jump-target label (pad → even).
`/Os` suppresses them entirely (verified empirically — same source emits
`jmp` + label back-to-back). The pad is decided at compile time on the
routine's offset within the module's .obj code segment, so it reproduces
only when in-function emission is byte-identical AND the function starts
at an even module offset — sub_12754 as the first function in stpanel.c
(offset 0) reproduces all 13 exactly.

This forced a module split: the routine's original module was `/Ot`
(interior pads), while stmenu.c's other routines are `/Os` (sub_1ACA0's
row loop must NOT emit one). MODULE_FLAGS now has `start/stpanel.c` at
`/AS /Gs /Ot`.

Also learned: `A*B` where both operands share a base symbol
(`*(p+i) * *(p+i+2)`) puts the larger-displacement operand in `ax` for
`imul` regardless of source order; the original used distinct symbols
`word_23E26[si]`/`word_23E28[si]` (the +2 field as its own IDA symbol) —
declared `int16 word_23E28` in ststubs.c to reproduce
`mov ax,[si+base]; imul [si+base+2]`. `uint8 objectActive[]` gives
`sub ah,ah` zero-extension where `int8` gave `cbw`.

`src_start/stobj.c` ports `sub_15460` (seg000:0x5460) and `sub_15B68`
(seg000:0x5b68) — the twin theater/object-data loaders. Both build the
200-entry EGA row/plane table `word_2C7D8` (`(i/4)*160 + ((i&3)<<13)`),
expand 16 packed palette bytes into four nibble tables, unpack big-endian
0x5C-stride attribute records + 0x49-stride name records, delta-decode the
packed sign/hi-bit table into `word_21718`, then run the icon/marker blit
loops and the `word_2B83E` gfx-mode screen-segment switch. sub_15460 reads
via `bufReadFile`/file handle (`resFileOpen`+`sub_149A1` init,
`sub_153F2` advance); sub_15B68 is the stream variant (`seg_2D274`=
arg + `sub_16C0E` refill, `sub_15B22`/`sub_15B02` reads, `sub_16AE7`
instead of `sub_169BE`, no close). Both are `/Ot` (interior nop pads).

Two codegen finds: `sub_15AD2(0xFFFFL)` — the caller pushes a 32-bit const
(`mov ax,-1; sub dx,dx; push dx; push ax`), so declare the param `int32`
even though the callee is `(int16,int16)`. And palette nibble temps reuse
the named `int16 j` slot — written as plain `int16` assignments MSC emits
`sub ah,ah; and ax,imm` word ops (vs byte `and al,imm` for a uint8 temp),
keeping the 12-byte frame exact.

The 0x61a4 wrapper (`sub_161A4`, unnamed in the map) sits inside
sub_15B68's extent but was `/Os` in the original (shared-epilogue
`jmp`/`sub ax,ax` tail that /Ot inlines). Moved to its own module
`stobjb.c` whose basename sorts right after stobj.c — the portcheck test
exe links modules alphabetically, so it lands contiguous and the full
5b68-61cb extent matches.

## 11. English END.EXE batch — menu/debrief cluster + string helpers

`src_end/enbrief.c` (`/AS /Gs /Os`): isPointInRect (0x3ff5), blinkWidget
(0x3f60), selectMenuItem (0x3c2e, 0x332b — f15 selectMenuItem verbatim,
a..o locals bucket-engineered into the 0x2a frame), processDebriefInput
(0x401d, 0x48b — f15 port incl. the 12-entry `jmp cs:[bx+tab]` switch;
map marks the table U), drawMapPixel (0x564f), mapToScreenX/Y (0x54ba/
0x54cf). `src_end/enfile.c` (`/Os`): strcoll (0x43a4 — 4-arg forwarder to
sub_114A9 dosRead), loadFileSection (0x413a — allocBuffer + memsetFar
zero via a FarWords union arg). `src_end/enmain.c` (`/Od`, chkstk frame):
initGraphics (0x0d2a — 8 dead int16 locals + driver jump-table calls).
`src_end/enaward.c` (`/Od`): loadPicFromFileAt (0x6fd8 — open/lseek/
close). `src_end/drawstr.c`: stringWidth (0xa88), drawStringAt, my_ltoa,
my_itoa, allocBuffer, freeBuffer, readWorldData, plotMapPoint,
drawClippedLine(Ex), drawFlightLine, formatFlightTime — all MATCH.

`src_end/enstr.c` (`/Os`): mystrcpy (0x38ba) and mystrcat (0x3923). The
rest of the 0x38ba-0x3b8d cluster is hand-asm — same fingerprints as the
START twins in stutil.c: mystrlen/mystrchr preload the param into ax
pre-loop and keep it live (no C shape reproduces this), mystrchr pushes
si unused; strcpyToFar/farStrcpy/memsetNear/memsetFar/memcpyFromFar are
LES/LDS+LODSB/STOSB+REP; copyBytes uses `loop`, memeq uses byte-stepped
word cmp + `loope`. All stay skeleton. The fused map extents were split
at real proc boundaries (mystrcpy..memeq now individual entries).
Codegen find: `for(; (*d=*s++)!=0; d++)` puts d++ in the for-increment
slot — MSC rotates it to loop head producing the inc-first layout, and
the preceding scan loop emits top-tested (`cmp;je fwd;inc;jmp`) — the
same source as START's verified mystrcat. `do{}while(*d++=*s++)` does
NOT produce this (load-then-inc, bottom-test).

## 12. English END.EXE batch — enmain/enworld /Od cluster + /Gs string drawing (0x398–0xA87)

The early-END region is F-15 lineage from enmain.c+eninput.c+enworld.c
(chkstk `mov ax,N; call __chkstk` frames = /Od, no /Gs) plus a /Gs string
module. Map split + renamed at real proc boundaries (the `db` regions
between routines were mid-function `call far` immediates, not code gaps):
cleanup (0x398), restoreVideoMode/restoreInterrupts (empty /Od stubs
0x3d5/0x3e6), loadWorldStrings (0x3f7), readWorldData (0x45f),
loadWorldData (0x558), setupWorldBufPtr (0x59e), readFromWorldBuf
(0x5c6), writeToWorldBuf (0x605 — extent resplit: 0x644+ is nullsub1),
clearKeybuf (0x655), waitForKeyOrJoy (0x67b), waitForKeyOrJoy2 (0x702),
then /Gs: drawStringAtPos (0x7a0), drawStringAt (0x7c6), drawFarString
(0x7eb), drawWrappedTextFar (0x814 — F19 extra `while(*a==' ')a++`
space-skip vs near variant), drawWrappedText (0x965), stringWidth
(0xa88). All MATCH.

Codegen finds:
- The wait loops are `while (keybuf() != 0 && readJoy(0) == 0)` /
  `while (kb() != 0) serviceTick()` — the &&-chain emits `J<c> next-cond;
  JMP exit` per conjunct; the do/if/break f15 form emits inverted
  `J<c>`-to-tramp under this /Od module and does not match.
- Post-loop `if (keybuf() == 0) key = getkey();` is a PLAIN if — no
  else, no goto: `J<c>` to the getkey arm and the fallthrough jmp shares
  getkey's exit `jmp done` tail (key stays uninitialized on the
  key-pending path — matches `mov [bp-2]` only on the getkey branch).
  The `if(c)goto L` rewrite MSC-cond-forms into `J<c> tramp; JMP next`
  (double tramps), and if/else forms add dead join jmps.
- drawstr.c is /Os (not the /Ot default): /Ot emits a relax-pad nop at
  the wrap loop head; /Os suppresses it.

## 13. English END.EXE batch — file/picture I/O wrappers (0x12C6–0x212B)

File/pic I/O layer split from the fused extents and ported as the thin
C wrapper tier over the hand-asm DOS int21 primitives (openFile 0x13ae,
createFile 0x1405, fileClose 0x145c, readFile1 0x147e, readFile2 0x14a9,
writeFileAtRaw 0x1539 — all stay skeleton):

enfile.c (/Os): loadFileSection (0x12c6), writeFileSection (0x12fe),
  openFileWrapper/createFileWrapper/closeFileWrapper (0x1336–0x1367),
  readFile1Wrapper/readFile2Wrapper/writeFileAtRawWrapper
  (0x1368–0x13ad), readPicStream (0x1a3e — buffered byte reader over
  picStreamRead with the 0x200 refill at picBufPos>0x1ff),
  allocClearBuf (0x20fc — allocBuffer + memsetFar; map was misnamed
  loadFileSection — renamed; it is the same alloc-clear idiom as
  EGAME's loadFileSection but END keeps the real loadFileSection at
  0x12c6, so the late copy got its own name).
enaward.c (/Od): openBlitClosePic (0x15df), openDecodeClosePic
  (0x1615), openDecodePicAt (0x164b — open+lseek(SEEK_SET)+decodePic
  +close; sub_11615 extent split at 0x164b), loadPicFromFileAt
  (0x15a6 — already ported).

picBlit (0x1706) / decodePic (0x17e2) / the pic-stream asm internals
(picReadBlock 0x1500, picStreamRead 0x1521, picReadDataAndMakeDict,
picMakeDict, doPicDecode, dictionaryLookup) remain skeleton asm.
All 14 routines MATCH; stubs.c EXE_END block updated (dosReadFar/
openFileWrapper/closeFileWrapper removed, fileClose/picStreamRead/
decodePic/picBufPos/picStreamBuf added; openBlitClosePic stub guarded).

## 14. English END.EXE batch — record-table cluster (0x281B–0x2E50)

enstr.c (/Gs /Os): seedRandom (0x0cf3 — naked tail-call `srand(biosTickLo)`
  under /Gs; /Od gave it a chkstk frame), randomRange (0x0cfe —
  `(int16)((rand() * (uint16)maxVal) >> 15)`, sar via __aNlshr; extent
  shrunk to 0x0cfe–0x0d16, trailing pad byte belongs between extents).
enbrief.c (/Gs /Os): serviceTick (0x281b — frameless tick-gate over
  tickByte/tickArm, same idiom as START's sub_161F1), tickRecords
  (0x2832 — 0x5C-stride record walk via byte-view externs), parseCmd
  (0x2cc9 — explicit test/body/done labels so the ':' digit loop keeps
  its `> '9'` check reading the temp memory var), clearActiveInRect
  (0x2d8d), resetRecField9 (0x2e27). The fused sub_12CC9 extent split
  into the three real routines.
enfile.c (/Ot): stageAppend (0x212c), readStageStream (0x214c — second
  buffered reader over picStageRefill = sub_13238 rep-movsw stage copy).
drawstr.c (/Gs /Os): drawTextOp (0x2ef2 — initResultFlag==1-only guard
  into graphics-driver text op 0x762) and drawTextModeDispatch (0x2f27 —
  4-way switch on drawTextMode to far driver entries 0x165/0x477/0x762/
  0xa61).

clearActiveInRect detail: the rotated first conditional emitted
`push si` + `cmp [bp+4],si` whenever the arm read `rp->f0` — MSC
dedicates si to a field value shared between eval and arm. Writing the
arm `xmax = recTable[i].rx` (array-indexed, different CSE key than the
eval's `rp->rx`) broke the dedication: eval became `cmp [bx],ax`, arm
`mov ax,[bx]; jmp` into the shared `xmax = ax` store inside the head
x-arm — byte-exact with the original.

All 8 routines MATCH; verify-exes 0 diffs on all four exes.

## 15. English END.EXE batch — map-window + flight-path cluster (0x27F6–0x52F9)

enbrief.c (/Gs /Os): drawMapView (0x27f6 — sets mapViewX1/mapViewY1 =
  the big-map view origin words word_23786/88, dispatches runMapView
  = sub_12192, returns `sel ? ... : ...` via explicit if/return — `?:`
  emitted branchless sbb), drawEventSprite (0x4dde — 12-case jump-table
  switch on flightRecords[i].status&0x3f, non-blink twin of
  processDebriefInput's blink switch; `uint16 rec` param — signed gave
  imul vs ref mul; embedded jump table marked U4f61-4f78), drawFlightPath
  (0x52f9 — route polyline + event-sprite pass; 14 named int16 locals
  reproducing the 0x1e frame per f15 END0.COD decls; bottom-tested loops
  written `while ((flightRecords[++a].status & 0x3f) != 0)` with
  `if (a > max) goto` body checks — the ++a-in-test produces the reg-form
  `mov ax,[a]; mov cx,6; imul cx` scaling the original uses; comma-test
  forms folded the limit check into the test region).

Two distinct map-window global families disambiguated:
- mapViewX1/mapViewY1 (word_23786/88): big-map view origin, drawMapView.
- mapWinX1/Y1/X2/Y2 (word_1DF8E/90/92/94): debrief clip window used by
  plotMapPoint bounds, drawClippedLine(Ex)/drawFlightLine window args,
  and both sprite-switch origin offsets. Prior commits mislabeled the
  1DF8E family as mapViewX1 (portcheck can't see data operands); fixed.
- drawClippedLineEx params renamed cx1/cy1/cx2/cy2 → wx1/wx2/wy1/wy2:
  push order 1DF8E,1DF92,1DF90,1DF94 = X1,X2,Y1,Y2, and its
  w=arg5-arg4/h=arg7-arg6 compute window width/height.

All 21 enbrief.c routines MATCH; verify-exes 0 diffs on all four exes.

### END.EXE drawMenuItem (seg000:0x44a8, 0x935 bytes)

f15 enbrief.c `drawMenuItem` — the debrief detail panel. Type-7 items draw the
mission-complete summary (route replay via drawFlightPath, OVERALL + MISSION
RATING centered, ejectedFlag=1); blink items clear the event panel, print
"MISSION EVENT" + formatted time, then a 12-way jump-table switch on
`flightRecords[cur].status & 0x3f` builds the event string (unit/object names
from worldObjects+worldStrings, plane names, SAM weapon names, weapon-release
names, takeoff/landing text), PRIMARY/SECNDRY objective tags, cumulative
rating, and the "next mission event" prompt.

F19 divergences from f15: no training/career-total/summary blocks in the
type-7 branch (ends at ejectedFlag=1), and `n = stringWidth(...)` +
`drawStringAt(..., 0xe8+(0x57-n)/2, y)` spelled out inline (no
drawStringCentered helper call).

Frame engineering: `sub sp,0x22` — locals p/a/b/d char[2] (hash buckets
0/1/2/4 → slots -2/-4/-6/-0a), fillers c..l (bucket 3..12) + numBuf[4]
(-0x1e), uint16 m (-0x20) + uint16 n (-0x22). `index` is uint16 → `mul`.
Map: renamed sub_144A8→drawMenuItem, jump table split out as U4be1-4bf8.
stubs.c: drawMenuItem stub removed; sub_10E50/sub_15666/sub_15D1B +
drawMenuItem globals stubbed under EXE_END.

All 24 enbrief.c routines MATCH; verify-exes 0 diffs on all four exes.

### END.EXE animateFlightPath (seg000:0x4f7b, 0x37d bytes)

f15 enbrief.c `animateFlightPath` + the F19 campaign gate: records with
status 7/6 under `pilotRec->isCampaignMission == 2` draw a track segment and
loop to `top` (clearRect+title redraw); timestamp events (status 9) run the
TIME: display + score + 5-tick wait and loop to `loop_top`; all other
statuses fall to `done` (decrement + final segment). Frame 0x1a:
`char numBuf[22]` (-0x16), int16 n (-0x18), uint8 evt (-0x1a).

Layout: `if (status&mask) {nonempty-deferred}` (jz gate-adj;jmp
nonempty-far) — gate is the post-if continuation textually AFTER the if,
nonempty defers behind it; inside gate, `if (f3a==2) {evt; if(7||6)
{track}}` uses positive-form nested ifs (jcc adj;jmp done-far). Track XY
uses `if(==0&&==0){first;goto trtail}else{goto trseg}` with trseg/trtail
labels textually between the else-arm and done — pushes both into the
deferred queue after nonempty (FIFO). done emits last; its inner XY else
stays adjacent. `flightRecords[++curRecordIdx]` gives the reg-form
`mov ax,[idx];mov cx,6;imul cx`.

Map: renamed sub_14F7B→animateFlightPath. PilotRecEnd: isCampaignMission
carved out of pad38 at +0x3a. stubs.c: lastDrawX/lastDrawY added.

All 25 enbrief.c routines MATCH; verify-exes 0 diffs on all four exes.

### END.EXE calcMissionScore (seg000:0x5666, 0x6b4 bytes)

f15 enbrief.c `calcMissionScore` — walks `flightRecords[]` per kill event,
dispatching on `unitTypeTable[unitId&0x7f]` flag bits (0x40 friendly, 0x80
civilian, 0x20...) and `flightRecords[i].status` (0x80 primary target,
0x40 secondary) to award awardPrim/awardSec/awardFriendly/awardCivilian/
awardUnit, bumping ms_airKilled/ms_groundKilled/ms_friendly*/ms_civilian/
ms_unauth*. Trailing block applies multTheater/multMission/multDiff/multUnk
`score = tab[*] * score / 8` then result modifiers. `score`/`ms_*` reach
locals/globals; award arrays indexed by `pilotRec->isCampaignMission` (+0x3a).

Layout: the case-1 ground dispatch drives three MSC idioms at once.
(1) `awardSec[1]` must OWN the shared `cwd;add;adc` tail so the armed arm
merges BACKWARD into it — done by making awardSec a real `goto award_sec`
label (early emission pass) rather than a fallthrough arm. (2) armed is the
in-flow warm arm (`mov bx,es:[bx+58]` head); `score_unarmed` is a deferred
goto-only label so its head emits `les bx,pilotRec`. (3) awardUnit's
`awardUnitChk<0` must reach the shared `unauth_ground_inc` block as `jl` —
a backward `goto` won't fold, so a forward `goto unauth_fw` tramp folds to
`jl`, and `unauth_fw:` `goto`s back to the labeled increment.

Map: renamed sub_15666→calcMissionScore.

All 26 enbrief.c routines MATCH; verify-exes 0 diffs on all four exes.

### END.EXE tickRecAnim (seg000:0x288b, 0x43e bytes)

The record-animation DSL interpreter — `tickRecords` calls it once per active
record per tick. Runs the record's bytecode script (`strOff`) until a sprite
draws, then saves `chanIdx` and returns. Opcodes: digit-runs set
`repeat[chan]`; `:` skips a digit run; `<`/`>`/`^`/`_` wrap-adjust the panning
counters `cntA`/`cntB` against `maxA`/`maxB`; `A`–`Z` blit a sprite tile —
one clipped `sub_12F27` when `cntA|cntB == 0`, else a 2x2 panning tile of four
`sub_12F27`s offset by `cntA`/`cntB`; `(` pushes the channel and picks a
`(alt|alt:N|…)` weighted alternative via `parseCmd` + `randomRange%count`;
`)`/`|` pop back to the saved `strPos` while a repeat remains, else depth-scan
to the matching `)`; `\0` resets to channel 0. Record is a packed `AnimRec`
(0x5C stride) reached via `recOff` byte offset; fields `repeat[]`/`strPos[]`
are `chni`-indexed byte arrays.

Layout: three codegen idioms drove the match. (1) The letter range check is
`jge`/`jle` continue-on-true + far `jmp groupops` — only produced when the
letter body is the INLINE then-arm of `if(opb>='A'&&opb<='Z'){...}` and the
~0x180-byte draw region pushes `groupops` past ±127 so `jl`/`jg` can't reach.
(2) The four blits' last call shares `loc_12B94`'s `push word_1BAD4;call` tail
with the single-blit arm via a `goto drawdone` merge-skip. (3) The `)`-pop is
`if(repeat[chni]==0){skip}else{pop}` — `jne` targets the forward pop block and
the `==0` then-arm reuses `si=chni+recOff` for `strPos[chni]` (`[si+0x17]`),
while the `)`-scan is `depth += (strm[sp]=='(')` (a `strm+sp` temp read twice,
NOT a mutating `strm+=sp`). Local-name buckets: `num opb chni sp fn depth strm
tmpf dnf accb` + byte `nxt` land the 11-word frame.

Map: renamed sub_1288B→tickRecAnim.

All 27 enbrief.c routines MATCH; verify-exes 0 diffs on all four exes.

### END.EXE loadMapView (seg000:0x1a8a, 0x671 bytes) — new module enbrief2.c

The map-view resource loader, called from `runMapView` (sub_12192). Clears
init flags, builds the 200-entry scanline→plane-offset LUT `word_2351E`
(`(i/4)*0xA0 + (i&3)<<13`), opens the PIC stream (`openFileWrapper` +
`picStreamRead`), pulls a resource block via `sub_120FC(0xFFFFL)` (a long arg —
`mov ax,0xffff; sub dx,dx; push dx; push ax`), and conditionally calls the far
gfx-driver slot `9D9:13FF` (`gfx_drvMode`) when `initResultFlag==2`. Then reads
a 0x20-byte header and nibble-expands it into four 16-entry palette tables
(`byte_2089D/AD/BD/CD`), reads the record count/w/h, and fills all `AnimRec`
records (0x5C stride @0x295E) plus a parallel `ChanRec` array (0x49 stride
@0x3426) — record headers 0xB bytes, `strOff` set by the script-append helper
`sub_11A1C`, per-channel 7-byte records. A bit-pack decode merges packed values
into `word_1BAE0`/`word_1BAE2`; finally a `switch(initResultFlag)` writes the
display-seg slots (`word_1BAD0/1BACE/1BAD2/1BAD4`) and calls the mode blitters
(`sub_133BA/133BD/133EC/13436`).

Module split: the routine is byte-exact only under `/Ot` — ref emits a
branch-target alignment `nop` at the per-channel loop head (`loc_11C36`,
even-aligned by pad) that `/Os` suppresses. The rest of enbrief.c is genuinely
`/Os`, so loadMapView lives in a separately-flagged `src_end/enbrief2.c`
(`/AS /Gs /Ot`, MODULE_FLAGS `end/enbrief2.c`) — it was a different original
module. `sub_11A1C` (1a1c-1a3d) is an unmapped code island (no proc label), so
it stays a callee stub. Map: renamed sub_11A8A→loadMapView.

All 27 enbrief.c + 1 enbrief2.c routines MATCH; verify-exes 0 diffs on all
four exes.

### END.EXE sub_15D1B extent check (seg000:0x5d1b-0x8531, 0x2816 bytes)

Verified the map extent is REAL: `sub_15D1B` is a single monolithic proc
(~10KB) — the END debrief/campaign mega-dispatcher (170 `call sub_13923`,
97 `call far`, 18 `sub_138BA`, …). No embedded `db`/`dw` data and no
swallowed sub-prologues inside (the 9 mid-range `push bp` are register
saves, not procs). Clean `pop si; mov sp,bp; pop bp; retn` epilogue at
0x8531; `sub_18532` proc begins adjacent at 0x8532 — no boundary overlap.
Candidate for skeleton (a ~10KB dispatcher is impractical to C-port
byte-exact); left as `sub_15D1B` asm skeleton.

### SU.EXE suutil.c cluster (seg000:0x971-0xe9d) — new src_su/ module, 9 MATCH

First SU.EXE C ports. The fused `sub_10971` map extent (0x971-0xe9d) was
re-split into its 9 real routines — verified byte-exact by `make
verify-exes` after the map split (extents tile contiguously; the trailing
`nop` folds into sub_10E84). All are the satellite's field-format /
word-wrap / delay helper library; none are reachable by direct `call`
(dispatched via a function-pointer table), so ada had lumped them into one
extent. Ported in `src_su/suutil.c` (`/AS /Gs /Os`):

- `sub_10971` (0971-0999) — 2-call wrapper; far call `2FC:0A3F`
  (`suFieldPrint`) into the driver segment.
- `sub_1099A` (099a-0aea) — FAR-pointer word-wrap (`char far*`,
  `les bx`/`es:[bx]` reads, signed `cbw`); sibling of sub_10AEB plus an
  extra `while(*wgt==' ')wgt++` leading-space skip and far copy
  `sub_11732(dst,far src,n)`.
- `sub_10AEB` (0aeb-0c0d) — near-pointer word-wrap (`uint8*`,
  `sub ah,ah` zero-ext).
- `sub_10C0E` (0c0e-0c51) — string pixel-width (`uint8*`).
- `sub_10C52` (0c52-0d88) — 32-bit `long` comma-itoa via `aNlrem` +
  `pascal sub_11D56(divisor,&val)` in-place divide.
- `sub_10D89` (0d89-0e5e) — 16-bit `idiv` comma-itoa.
- `sub_10E5F` (0e5f-0e78) — tick delay helper.
- `sub_10E79` (0e79-0e83) — frameless near helper.
- `sub_10E84` (0e84-0e9d) — regarg `_aNlshr`-backed fixed-point scale.

Codegen notes for the word-wrap twins (the hard pair): the scan loop is a
bottom-tested `while(tw<lim)` — a plain `while` won't rotate while a
`break` is present, so the exit is written `goto scanned` which makes MSC
rotate AND keeps `bx` cached across `cch=*cp`→`*cp++`. `lim` must be
`uint16` (unsigned `jb`). The `scanned` tail polarity is
`if(tw>=lim)goto backdec; goto chkspace` → `jb chkspace; jmp backdec`.
The backtrack is `do{chkbrk;backdec;chkspace}while(cch!=0x20)` with the
two entry gotos landing mid-body — MSC keeps `chkbrk` a separate
memory-reading block (`cmp byte[cch]`) instead of fusing it into
chkspace. In sub_1099A the bound check `cp<=st` on `char far*` emits only
the offset-word `cmp` (both halves of `st` are loaded but seg is unused).
Local frames are name-hash engineered: sub_10AEB 0x3fa frame
(pa/wgt/cp/cnt/dum/st/ag/tw + buf,cch), sub_1099A 0x20e frame with three
4-byte far* locals.

All 9 MATCH; verify-exes 0 diffs on all four exes.
