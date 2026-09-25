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
- Related bool lowering: a `uint8` comparison against `0`/`1` that feeds an
  argument lowers to `sbb ax,ax; neg|inc ax` only when written as `x == 0` /
  `x != 0` — `x < 1`/`x >= 1` produce a `jnb`/`jae` branch instead. And the
  operand must stay in `al` (read the just-stored global under `/Oa`, which
  forwards `al`), with `ax` free for the bool — a `uint8` temp var adds a
  stack slot and shifts the result to `cx`.

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

### Locals & the name hash

- Local stack slots are assigned by **variable-name hash**, not declaration
  order: `bucket = sum(name bytes) % 16`, buckets allocated ascending, same
  bucket prepends (last declared gets the lower slot). Reusing f15's exact
  identifier names is the easiest way to reproduce a frame; otherwise
  brute-force names.
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
- `/Gs` (no stack checking) vs `__chkstk` prologue.

### mzdiff tolerances

`portcheck.py` runs `mzdiff --nocall --loose` and additionally tolerates:

- **data segment offset mapping conflicts** — globals live at different
  addresses in the test exe; the instruction stream still matched.
- **stray `nop`** — MSC aligns jump targets to even boundaries; pad placement
  depends on the routine's offset in the final exe.

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

Lesson: when a port resists, isolate to a `src/_t.c` repro (one function,
minimal externs), compile with `/FcD:\t.cod`, and diff the `.COD` asm against
the .lst — full portcheck cycles are for confirming, not exploring. And when
mzdiff desyncs on a single inserted instruction, fall back to aligning the
`.COD` against the `.lst` by hand — the rest of the stream may already match.

## 6. Naming / bookkeeping

- `conf/routine_names.txt`: `routine_N name # seg:off description`. After
  verifying, `tools/rename_routines.py map/egame.map` writes names into the
  map so later routines see callees by name.
- Keep a verified list (AGENTS.md) and bump the count on commit.
- File the port under the module matching the original source-file split
  (egcombat.c, egframe.c, egtacmap.c, ... — same split as f15se2, since the
  modules compile with different flag sets).

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
