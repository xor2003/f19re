# F-19 Stealth Fighter reconstruction

Goal: recover F19 (MicroProse, MSC 5.1) as C source for eventual merge into
/home/xor/games/f15se2-re. Approach: byte-exact asm skeleton + incremental C
ports verified against the original binary.

Scope: EGAME.EXE (the flight sim proper, 364 routines) is fully classified —
183 byte-exact C ports + 2 Z3-proven + ~179 asm/CRT (skeleton-reproduced).
All four EN exes rebuild byte-exact as asm skeletons (`make verify-exes`):
SU.EXE, START.EXE, END.EXE, EGAME.EXE via `tools/exe2asm.py` +
`lst/<x>_en_ada.lst` + `map/<x>_en.map`. EN-EGAME C porting is done:
144/152 candidate routines MATCH via `src_en/` override modules
(egmain/egtarget/egcombat/egframe/egflight/egtacmap/egui.c) — the 8
non-matching are documented hand-asm/CRT routines that stay skeleton
(openFile closeFile picBlit openBlitClosePic installCBreakHandler
setInt9Handler fillSpanRect projectSceneObject). Port EN routines with
`python3 tools/portcheck.py src_en/<mod>.c <name> --srcdir src_en
--exe /home/xor/games/f19/F19/EGAME.EXE --map map/egame_en.map`.
The EN install at /home/xor/games/f19/F19 is now authoritative (differs
substantially from RU — the repo-root START/END/SU.EXE are the RU
binaries, do NOT verify satellite ports against them): SU.EXE (101 rtns,
map/su_en.map — 24 egame sig matches), END.EXE (180, map/end_en.map — 27),
START.EXE (251, map/start_en.map — 41). Maps and lst/<x>_en_ada.lst come
from ~/vextest ada.py runs; RU-era maps kept as map/{su,start,end}.map
(different binaries). mzdup over the EN binaries needed mzretools decoder
hardening: 286/x87 opcodes and never-throw semantics for unimplemented
bytes (data inside routine extents would desync the instruction stream
otherwise).
EN satellite C ports (all verified vs $(F19EN)/<X>.EXE + map/<x>_en.map
via `portcheck.py --exe /home/xor/games/f19/F19/<X>.EXE --map
map/<x>_en.map --srcdir src_<x>`):

- src_end/ — enbrief.c 27 MATCH + enbrief2.c 1 MATCH (loadMapView, the
  0x671 map-view resource loader: scanline LUT + PIC palette nibble-expand
  + AnimRec/ChanRec record fill + mode setup), plus drawstr/stalloc/
  textfmt/enworld/enfile/enmain/enaward/enstr helpers (allocBuffer
  freeBuffer drawStringAt my_ltoa my_itoa readWorldData strcoll
  loadFileSection initGraphics loadPicFromFileAt mystrcpy mystrcat ...).
  Flags: enbrief/stalloc /Gs /Os,
  enworld /Od (unconditional chkstk+push di,si prologue), textfmt /Os.
  enbrief2.c /Gs /Ot — loadMapView emits a branch-target alignment nop
  that /Os suppresses; separate original module from /Os enbrief.c.
  Left as skeleton: dos_alloc/dos_free/openFile/closeFile/fileClose/
  createFile family (int21h hand-asm), pic-decode cluster (decodePic
  showPicFile doPicDecode picMakeDict picReadDataAndMakeDict
  dictionaryLookup picBlit openBlitClosePic — bp-repurposing asm),
  setTimerIrqHandler/installCBreakHandler (int21h vectors), clearRect
  (rep stosw), clipAndDrawLine/calibrateTimerSpeed/readJoyAxis (hw asm),
  strcoll/gety/move_ovlcur + all CRT.
- src_start/ — 146 MATCH:
  cleanup.c: cleanup
  drawstr.c: drawStringAt drawStringFar drawStringAtFar
  stalloc.c: allocBuffer freeBuffer
  stfile.c: resFileOpen resFileCreate resFileClose resFileReadFar
    resFileWrite resFileReadBlock resFileWriteBlock
  stgen.c: rangeApprox memAppend commFetch setMoveDstComm7A doNothing
    positionUnit calcBearing findOrPlaceItem parseWorld exportWorldToComm
    formatGridRef formatTimeStr clampValue missionGenerate runGenerator
  stgrid.c: parseGrid replaceExtension
  stload.c: loadSpriteScaled loadPicRes loadSpriteRes loadResSection
  stmenu.c(/Os): sub_18F12 sub_193EE sub_1A68C sub_1ACA0 sub_1AE22
    sub_1AFA8 sub_1B184 sub_1B304 sub_1B452
  stpanel.c(/Ot): sub_12754
  stobj.c(/Ot):  sub_15460 sub_15B68
  stobjb.c(/Os): sub_161A4
  stmap.c(/Ot): mapToScreenX mapToScreenY drawMapLine drawClippedMapLine
    drawMapPoint plotMapPoint sinMul cosMul toggleSelRect drawRiskPanel
    rectInView shiftByMode drawScorePanel drawUnitList sub_10AE8
    sub_11366
  stmain.c(/Os): sub_10010 (START main: comm-block + overlay-slot setup,
    title/credit/adv splash, 4-way memory-tier switch, byte_2C160 menu
    state machine, object init) sub_10810 sub_108B7 (+ empty retn slots
    sub_108B5/sub_108B6/sub_14CCA)
  stparse.c: parseGridTerrain parseTerrain
  stpinp.c: saveHallfame loadHallfame pilotNameInput
  stterr.c: lookupGridCell
  stutil.c(/Os): randMul mystrcpy getTimeOfDay drawStringCentered
    showMsgWaitKey itemDistance getItemCoordStr readInputKey initGraphics
    stringWidth drawLine drawClippedLineEx drawTileIcon mystrcat
    selectNextObject selectNextUnit selectPrevUnit selectPrevObject
    drawUnitMarkers drawTileMarkers drawSiteMarkers rtcSync
    evalChoiceExpr clipEntries drawMapArc dosRead advanceBufPos
    setViewOrigin sub_161F1 delayTicks bufReadBytes resetTableFlags bufReadFile
    tickEffectTable dispatchDrawMode wrapUnitText wrapUnitTextFar
    srand rand seedRng drawRoutePath drawThreatRings printMission
    printObjective drawStoreIcons stepPanelAnim
  textfmt.c: my_ltoa my_itoa
  Left as skeleton: dos_alloc doFcbSearch (int21h),
  decodePic/showPicFile/openBlitClosePic + pic cluster, clearRect/
  clearDirtyRects (rep stosw), clipLineCohenSutherland (vector-table asm),
  readJoyAxis/routine_71/routine_100 (in/out + cli), time/abs/getche/
  strcoll/strncmp/strncpy (CRT), installCBreakHandler, jump-table patcher
  sub_14107, reg-ABI trampolines (sub_141A3/sub_16DC2/sub_16E0C),
  mode-parser _openfile (sub_1E7B6), CRT time cluster
  (sub_1EFFE/sub_1EB7C), timer-port asm (sub_144F2/sub_14F8F).
- src_su/ — suutil.c 9 MATCH: the field-format/word-wrap helper cluster
  (fused `sub_10971` extent re-split into 9: sub_10971, the far/near
  word-wrap twins sub_1099A/sub_10AEB, string-width sub_10C0E, the
  long/int comma-itoa sub_10C52/sub_10D89, delay sub_10E5F, frameless
  sub_10E79, regarg-shift sub_10E84). Flags: suutil.c /AS /Gs /Os.
  word-wrap loops match via `while`-scan + `goto scanned` and a
  `do{chkbrk;backdec;chkspace}while(cch!=0x20)` backtrack with mid-entry
  gotos; `lim` is `uint16` (unsigned `jb`); sub_10AEB uses `uint8*` for
  `sub ah,ah` while sub_1099A keeps `char`/`cbw` + `les bx`/`es:[bx]`
  far reads and an extra `while(*wgt==' ')wgt++` space-skip. `cp<=st`
  on `char far*` emits MSC's offset-only `cmp`.
  Everything else stays skeleton: int21h file-IO/CRT asm
  (createFile/openFile/closeFile/readFile1/setTimerIrqHandler are the
  hand-asm DS=SS family).
portcheck satellite notes: modules compile with /DEXE_<EXENAME> so base
src/ can suppress colliding defs (egmath.c rangeApprox, egtacmap.c
drawMapLine, stubs.c cleanup are all `#ifndef EXE_START`-guarded) and
stubs.c gains EXE_START-only shims (intDispatch, misc_clearKeyFlags,
drawClippedLineEx). MODULE_FLAGS accepts `'<exe>/<file>'` keys —
start/textfmt.c and end/textfmt.c need /Os (their my_itoa uses
`for (k=5; k>0 && num[k]==0; k--)`; under /Ot a stray nop pad appears).
Current satellite focus is the English F19 drivers only:
MGRAPHIC.EXE (82 rtns, map/mgraphic_en.map — all 84 ABI slots named from
f15 slot.h, handlers ~94% byte-identical to f15's driver) and ASOUND.EXE
(48 rtns, map/asound.map — STEALTH.EXE-gen driver, 9 slots 0x64-0x6c per
f15se2-ex asound_model ABI). Drivers need `tools/drvmap.py` seeding before
mzmap (jump-table headers, no call-reachable exports).

## Layout

- `lst/EGAME.EXE.lst` — IDA listing of the main exe (primary analysis source)
- `map/egame.map` — mzmap routine map (names applied from matched_routines.tsv)
- `conf/egame.json` — lst2asm config (per-site fixes, BSS split, subs regexes)
- `build/` — generated: egame.asm/.obj/.exe, EGAME.EXE.h/.c, test exes
- `decompiled/` — Ghidra headless output, one .c per FUN_<seg>_<off> (451 funcs)
- `src/` — ported C (inttype.h, pointers.h, stubs.c + module .c files)
- `tools/` — verify_f19.py, portcheck.py, ghidra_src.py, compare_exe.py

## Build / verify

- `make verify` — regenerate skeleton asm, assemble (uasm -q -0 -Zm), link
  (msc510 LINK via dosbuild.sh), compare load image to original (0 diffs).
- `make hdr` — regenerate C decls (build/EGAME.EXE.h) via lst2ch.py.
- `make port SRC=src/x.c ROUTINES="name1 name2" [CLFLAGS="..."]` —
  compile+link+mzdiff per routine. Flags default to the per-module set
  recorded in `tools/portcheck.py` `MODULE_FLAGS`; CLFLAGS overrides.
- `make seed ROUTINES=name` — dump the Ghidra decompile for routines.

## Porting workflow

1. `python3 tools/ghidra_src.py <name>` — initial C from decompiled/FUN_*.c.
   Ghidra loads the image at para 0x1000; FUN_<para>_<off> maps to the routine
   at seg<para-0x1000>:<off> (e.g. FUN_1000_1068 = seg000:0x1068,
   FUN_1fef_04cf = seg001:0x4cf).
2. If in matched_routines.tsv with f15se2 source, adapt that source — it is
   usually nearly identical; only constants/globals differ.
3. Verify: `python3 tools/portcheck.py src/x.c name [extra /flags]`.
   MATCH = instruction stream identical modulo call targets, literals, and
   data-segment offsets.
4. Behavioral check: `python3 tools/dosunit16.py dosunit/<spec>.json`
   replays routines under ~/vextest dosunit real16 — original EXE vs the
   portcheck test-EXE — comparing regs/flags/observed memory. Specs in
   dosunit/*.json declare per-side dseg cell patches, far-ptr targets,
   observations and code ranges; 347 math/format vectors (190 egame_math +
   157 start_math, normal + edge inputs) currently AGREE — including two
   signedRatio16 zero-divisor vectors where both sides fault int 0
   (AGREE-FAULT, faithful). Edge coverage pinned MSC-16-bit quirks like
   abs(-0x8000) staying negative (isqrt/rangeApprox/signedRatio16).
   stubs.c supplies real sine/fixedMulQ14 (asm-faithful) so test-EXEs
   exercise true math.

   Generic probe sweep: `python3 dosunit/gen_probe.py` emits probe_<exe>_N
   specs (5 fixed 6-word vectors per ported routine, bounded extents,
   sentinel-seeded store obs); `DOSUNIT_ILIMIT=100000 python3
   tools/dosunit16.py dosunit/probe_<exe>_N.json` runs them. Every probe
   case unconditionally uses the oracle slotstub fixture (driver-ABI
   far-jump slots in DGROUP patched `EA`->`xor ax,ax; retf` — near-BFS
   can't see lcalls hidden behind indirect dispatch, and the fixture is
   inert for routines that never reach a slot). `find_ints` BFS follows
   rel16 calls, jmp tail-calls, short jumps and lcall seg:off immediates
   (40K-insn budget over per-address-deduped instructions — overlapping
   callee windows no longer rescan shared code); reachable `int NN` sites
   get a cached prepatched fixture build/<EXE>-ints-<tag>.EXE with
   `CD xx`->`xor ax,ax` (bounded "service succeeded, ax=0", symmetric to
   the cand build's zero-returning helpers) on BOTH sides (int_stub_c
   covers the cand image's own CRT/skeleton int sites). Two more fixture
   layers stack on the same scheme: `call_stub` nops oracle near/far calls
   whose target is a cand-side trivial stub (stubs.c/ststubs.c —
   `sub_<linaddr>` names index linear image offsets; `pop ax` encodes a
   near-site param-return stub), and `dsss_stub` nops reachable `ds:=ss`
   writes (`push ss;pop ds` / `mov r,ss;mov ds,r`) — real-DOS no-ops
   (DS==SS==DGROUP) that would otherwise slam the probe's scratch DS with
   the stack segment (sub_15460's resFileOpen int21 path).  ALL fixture
   builders drop MZ reloc entries whose word overlaps a patched span —
   the loader would otherwise rewrite nop'd `9A` seg fields and desync the
   stream (drawLine/drawClippedLineEx were corrupted by `90 90`->`90 a0`).
   Result: `python3 dosunit/coverage.py` prints the inventory
   (dedicated/probe-agree/probe-mix/artifact/incomplete/skipped).
   Latest full sweep (post dsss+iseen+reloc-drop+call_stub_c+partial
   dedicated specs): 83 dedicated + 212 probe-agree + 0 probe-mix +
   11 artifact + 5 incomplete (all in/out hardware io) + 0 skipped —
   0 diff, 0 uncovered across all 311 ported routines.  The former 5
   skipped entry points probe under call/int/dsss stubbing: gfxInit and
   openBlitClosePic 5/5 AGREE, main and waitForKeyPress 5/5 AGREE-FAULT
   (symmetric budget burn on poll loops), runGameSession incomplete
   (oracle reaches device io).  Fixture cache keys cover base content, so
   post-relink regen (`gen_probe.py`) is mandatory before trusting
   probe diffs — stale int-stub fixtures replay pre-relink data
   offsets and produce systematic false DIFFs (bombTarget,
   updateThreatTargeting were both this).  coverage.py's case index is
   global across shards — regens reshard cases without orphaning
   out.json results (result ids carry the fn name).

   Dedicated edge specs: `python3 dosunit/gen_edge.py` regenerates
   dosunit/edge.json — disassembles both sides, pairs DS operands
   positionally (deep=True inlines near-call callees one level, keyed by
   call ordinal so a stubbed callee can't shift the pairing), then builds
   ONE shared-DS demand map (explicit > abs-cell > indexed-span > store-
   zero) because replay patches land in both guests. span_obs=False
   scopes obs to top-level scalar stores when indexed/callee stores are
   provenance-suspect (own-image name tables); obs_drop skips pointer-
   constant stores. Currently 8 cases / 36 vectors all AGREE, incl.
   fireAirThreat on the ~630-insn real path and loadWorldStrings' full
   movedata chain off a synthetic comm-buf world image. NB: edge's
   MOVEDST/BUFPOS/WORLDSRC cand offsets are hardcoded per test-exe
   layout — re-derive by disasm after every relink (last verified
   layout: STGEN moveDst 0x3f18 pos 0x686e, ENBRIEF head 0x5966).

   Incomplete taxonomy (not regressions — sandbox limits):
   - device_io_or_halt (in/out port io): runGameSession (session loop
     drives real hardware), sub_16076/sub_16486/sub_1714C/sub_17280 —
     the only remaining incompletes; port io cannot be modeled.
   - artifact class: diffs proven to be probe mechanics, listed in
     coverage.py ARTIFACTS (stub callees, write-stream overlap,
     binary-relative pointer values, ds:=ss provenance).

   Partial-coverage routines have dedicated specs in
   dosunit/partial.json (dosunit/gen_partial.py, 12 cases / 36
   vectors, all AGREE): allocBuffer ok+err paths on END and START
   copies (call_stub/call_stub_c force `dos_alloc`->const segment on
   both sides; the err arm nops cleanup/print/exit so the post-exit
   fallthrough compares), both drawStringCentered twins (START's
   alloc+zerofill observes the zeroed DS window; EGAME's rec-set +
   strlen/strupr + driver draw stubs the resident lcall and observes
   rec fields + the uppercased seeded string), drawClippedLineEx/
   drawClippedLineRegion (all in-graph calls stubbed `zero` —
   symmetric with the cand thunk bodies; window-cell stores
   compared), drawGaugeBar (fillRectBoth's driver lcalls stubbed —
   they write into the runtime ljmp-slot table which the code-range
   guard rejects; window-record ptrs seeded at fixed scratch cells so
   the callee's [bx+4] stores are observed; [0x4efa] gate toggled
   per-vector), drawStoreIcons lo/hi cases (dead-overlay drawString
   near-calls nopped; es-record ptr seeded at a scratch window
   covering both 0x13 arms; first-lcall answer toggled for the
   byte_98E6 gate), loadPicFromFileAt (showPicFile nopped on both
   sides — the hand-asm PIC decoder loops on seeded bytes; openFile/
   closeFile run real code through int_stub/dsss_stub).
   dosunit16.py keys verdict sources by (group, id): duplicate fn
   names across cases (the two drawStringCentered twins) no longer
   cross-wire obs pairs.

## MSC 5.1 codegen facts (F19)

- Small model `/AS`: near code+data; cross-segment callees declared `far`.
- Optimize on: `/Gs` + `/Os` or `/Ot` — never `/Od`. The choice is per-module
  and recorded in `tools/portcheck.py` `MODULE_FLAGS`; `/Os` vs `/Ot` shows on
  routines with early returns (shared vs inlined epilogue). `/Ot` also emits
  nop pads before odd-aligned branch targets; `/Os` suppresses them. A map
  extent may swallow an unnamed trailing function from a different original
  module — put it in a separately-flagged module whose basename sorts next
  (test-exe link order is alphabetical) — stobjb.c after stobj.c.
- MSC 5.1 has no working `volatile` (accepted syntactically, ignored). Code
  touching timer/interrupt-updated globals was likely shipped under `/Zi`
  (debug) because optimized builds broke — if a routine resists matching under
  `/Os` or `/Ot`, check whether it was ever optimized at all.
- `/Oa` (assume no aliasing): needed where a global stays cached in a register
  across a pointer store — e.g. drawTargetView emits `push bx` not
  `push [gmem]` only under `/Oa`. Also dedicates `si`/`di` to expressions
  reused as call args across calls (`0x18<<n` → `mov si,imm; shl si,cl`
  reused via `push si`, vs per-call recompute without `/Oa`). Currently set
  on egtarget.c, egcombat.c, egframe.c, egtacmap.c, egrender.c, egflight.c.
- Stack probing (`__chkstk` prologue) = module compiled WITHOUT `/Gs`.
- Local stack slots are assigned by variable-NAME hash, not decl order:
  bucket = sum(UPPERCASED name bytes) % 16 (case-insensitive symbol hash),
  buckets walked ascending into consecutive slots, same-bucket vars prepend
  (last-declared gets lower slot). Verified against drawTacticalMap's
  11-local frame: gridX/gridStep collide under lowercase-sum but land apart
  under uppercase-fold. Compiler index temps (e.g. `arr[i]` scaled index)
  get slots too — past the named-locals area. Reusing f15se2's identifier
  names — or engineering names to hit buckets — reproduces the frame.
- Far stream pointers: `p++; c=*p++` emits inc/bx/inc/es read pattern.
- `x = -y + K` compiles to `neg ax; add ax,K` (use unary minus, not `K - y`).
  But a bare `x = -x` emits `mov ax,[x]; neg ax; mov [x],ax`; the original's
  `sub ax,ax; sub ax,[x]; mov [x],ax` needs `x = 0x10000 - x` (the constant
  overflows to 0 — yes, really). f15se2 computeHudAttitude.
- `v = v;` (self-assignment) evicts the compiler's cached register copy of a
  value, forcing recomputation; likewise splitting `&&` chains into nested
  `if`s makes MSC forget register-resident condition operands. Used to get
  `bx` (not `di`) for `sams[i]` scaling in f15se2 fireAirThreat.
- `register` at FUNCTION scope is unreliable ("now register, now you don't");
  declaring `register int i` inside a NESTED `{}` scope instead frees si/di
  for code outside it — f15se2 drawProjectionSphere. (The phantom-slot
  register params in spawnSamThreat/drawWeaponRadarInfo are a different case.)
- 16-bit `labs(x)` (no int32 cast) emits `cmp ax,0x7fff`; a `(int32)` cast
  emits `cwd` instead.
- Deferred "cold" arm (`mov ax,K; jmp` merging BACKWARD into a shared tail):
  MSC sinks a low-frequency arm to just before a multi-edge `goto` label /
  function chunk only when both arms end in an explicit `goto cont`. Write it
  as `if (C) { if (D) goto ARM; call(x); goto CONT; } ARM: call(y); goto CONT;
  CONT:` — the shared `goto CONT` tail forces a backward tail-merge and ARM
  emits after the continuation jump. Plain `if/else` or `?:` keep arms inline.
  Used in renderHudFrame (egtacmap.c); same pattern drives computeBearing.
- Shared `cwd;add;adc` tail-merge picks ONE canonical owner; competing `+=`
  arms emit `jmp` to it. The owner is whichever block MSC lowers first in
  block-emission order: a real `goto`-label arm emits in the early pass, a
  `$JCC` cold arm emits last and MERGES. To make a `score +=` arm own the
  tail (and force a competing arm to `jmp` BACKWARD into it), give the owner
  arm a real `goto` label (`if (cond) goto SEC; ... SEC:`). Armed/cold arms
  that merely fall through merge forward. calcMissionScore awardSec/armed.
- `goto` to a FORWARD label folds into `jcc label` (jump-threading); `goto`
  to a BACKWARD cold block stays an unconditional `jmp` guarded by the
  inverted `jnotcc +skip`. To get `jcc` reaching a distant shared block,
  `goto` a forward tramp label that then `goto`s the far target:
  `if (x<0) goto FW; stmt; break; FW: goto FAR;` → `jl FW; jmp cont; FW: jmp
  FAR`. A named label that's only `goto`'d emits as a deferred block whose
  head reloads the far pointer (`les bx`) — needed for cold dispatch targets.
  calcMissionScore awardUnit armed/unarmed dispatch.
- `v = *p++` emits `add [p],2` EAGERLY then `mov ax,[bx]` reading the
  stale cached pointer — not `mov` then `add`. Mixing `p[-k]` reads (bx
  reloads) with `*p++` writes reproduces sliding-window decoders.
  insertOutlineEdges.
- Loop-body `if/else` whose then-arm is a `?:` statement: with the `?:` arm
  FIRST in source, MSC splits the ternary test into the bottom-placed loop
  dispatch and jumps into the arm mid-block. Writing the OTHER arm first
  (`if (i<=n) {else-code} else {?:-arm}`) keeps the `?:` self-contained at
  the arm head AND lets an identical `?:` nested inside the first arm emit
  as a separate deferred block ahead of it. drawWaypointPanel.
- Early-exit `goto` chains (`if (x==0) goto TAIL; ...; if (y!=0) goto MID;`)
  reproduce MSC's function-chunk layout where a shared continuation label is
  reached from several jumps — nested `if`s give a different block topology.
- Two converging register temps (`ax`+`dx`, no stack slots) from an `if/else`:
  assign the SAME lvalue per-branch with the shared ops OUTSIDE the differing
  terms — `if (c) V = (A>>s)-f1(); else V = (B>>s)-f2();` tail-merges into one
  `shr/sub/store` while each branch only computes its own ax/dx operands.
  Storing into named locals forces `mov [bp],r` stores; `?:` on two operands
  CSEs the flag into a reg + retests it. Used in drawThreatIndicator.
- `>>` on `int16` emits `sar`; cast the shifted operand to `uint16` for `shr`
  (e.g. `((uint16)(viewZ-0x80)) >> 7`) — drawThreatIndicator.
- Scaled index persistence: for `tab[i].f` reads reused across intervening
  calls, MSC dedicates `si = i*stride` (emitting `push si` in the prologue)
  ONLY when the colour/select args in between are `?:` expressions — an
  interleaved `if/else` makes it drop to a recomputed `bx`. drawMissionObjectives.
- Switch on ≥4 compact cases emits a `jmp cs:[bx+tab]` jump table; the table +
  MSC's reordered case bodies (here 1,4,3,2,5,7,6,8) land in the map's
  `U`(unreachable) region, so `portcheck` passes `--map` to mzdiff to skip them.
  mzretools tolerates `jmp/call cs:[bx+disp]` table operands (no code-offset map).
- Inside a deferred chunk, which arm MSC emits forward vs backward isn't fixed
  by `?:` polarity — try `if (c) A else B` vs `if (!c) B else A` to flip the
  deferred arm (`jnz` vs `jz`). drawMissionObjectives `WTORI`/`OSNOWN.` select.
- A guarded call that FALLS THROUGH into a second call is sequential
  statements, not if/else: `if (!c) callA; callB;` — MSC defers callA's body
  early, and `jnz` jumps backward to callB which then runs in both paths.
  redrawTacMap loop2 (planeIndex call + always-run viewIndex call).
- Boolean-from-comparison is branchless: `(x == NULL)` → `cmp ax,1; sbb cx,cx;
  neg cx`. Write as nested assignment in the condition, e.g.
  `if ((arr[i] = ((h = fopen(p,"rb")) == NULL))) count--;` — keeps values in
  registers, avoids the store-then-load a named temp forces.
- Compiler scratch slots: `[bp-x]` stores that appear "from nowhere" are often
  MSC temporaries, NOT source variables. If a store looks impossible to order,
  remove the explicit variable/assignment and write the bare expression —
  `(a<<5) - b` — the compiler allocates its own stack temp in the right spot.
- Assignment chaining reuses partial results across widths: for mixed 8/16/32b
  zero-init `f1E = f1F = f16 = f1A = f1D = f1C = 0` — ORDER matters (brute-force
  it) and makes MSC carry 0 through al/ax/dx instead of `mov [x],0` per field.
- Whole-struct assignment `a[i+1] = a[i]` → `rep movsw` intrinsic (size is a
  compile-time const). A far-pointer argument is one DX:AX arg, not two words —
  declare `func(uint8 far *p)` so `func(ptr+off)` pushes seg+off correctly.
- Signedness flips `cmp` operand direction too: `uint16 vs int16` local changes
  `cmp [es:bx],ax` vs `cmp ax,[es:bx]` (which operand lands in ax).
- Loop-condition assignments: `do { if ((v=f()) != 4) break; } while
  ((v=f()+4) == 8)` puts the call+store at the loop bottom; `while (v=g, true)
  switch(...)` uses the comma operator to run an assignment in the test slot.
- `for(;;) switch(x){...}` WITHOUT braces around the switch emits the
  `jmp short` to the bottom-tested loop; bracing the switch removes it.
  Shared case-tail code = `goto label` placed INSIDE the last case.
- Code dedup across switch cases: identical code written in EACH case gets
  coalesced by MSC into one block that later cases jump into mid-block — write
  the repeated code redundantly, don't factor it out with goto.
- Non-orthogonality: later code affects earlier emission — a `return` vs
  `break` can drop a `les bx` reload; fixing a LATER mismatch can change an
  earlier jump to near (jnz+jmp) when a short no longer reaches.
- `/Oi` folds libc calls into intrinsics: memcpy → `rep movsw` (+`adc cx,cx;
  rep movsb` for odd tails — MSC 5.1 doesn't eliminate the tail even when the
  count is known-even; that residual may be genuinely unmatchable).
- `/Ot` pads branch-target labels to even with a `nop` (visible in EN
  binaries as `jmp short X; nop; label:`); `/Os` suppresses it. The pad is
  emitted at compile time on .obj-relative parity, so it reproduces only
  when the routine's in-module offset has the original parity — putting the
  routine first in its own module file makes that offset 0 (even), which
  matched sub_12754's 13 pads exactly. Function-END pads emit under both.
- `imul`/`mov ax` operand pairing for `A*B` on same-base memory operands
  (`*(p+i) * *(p+i+2)`, `p[0]*p[1]`): MSC puts the LARGER-displacement
  operand in `ax` regardless of source order. To get `mov ax,[f0]; imul
  [f2]` order, index two DISTINCT symbol bases — the original used
  `word_23E26[si]` / `word_23E28[si]` (IDA auto-named the +2 field as its
  own symbol). Struct-field form `arr[i].f0 * arr[i].f2` also preserves
  left-to-right. sub_12754.
- `if ((a | b) == 0)` on two byte fields emits `mov al,[a]; sub ah,ah;
  mov cl,[b]; sub ch,ch; or ax,cx; jnz` — `a==0 && b==0` emits two
  `cmp byte/jnz` pairs instead. stepPanelAnim letter-case gate.
- `[bx+si]` register roles in `s[i++]`: a POINTER-typed local `uint8 *s`
  gives `bx=i; inc i; si=s; mov al,[bx+si]`; an int offset + cast swaps
  them (`si=i, bx=s`). stepPanelAnim DSL string walk.
- `while ((peek = s[i]) >= lo && peek <= hi)` bottom-tests: the `&&` splits
  the bounds checks across the rotation (>= at bottom, <= at top); an
  `if (!...) break` inside fuses both compares at the top. For a ref loop
  whose FIRST instruction is a call, use `do { } while` not `while`.
- To keep a byte-dec/inc and its wrap test separate (`dec byte [bx+0xa];
  mov bx,[bp+4]; cmp byte [bx+0xa],0xff`) split `--e->f` and the `if` into
  two statements — `--e->f == 0xff` fused them.
- A 4-way char dispatch that the ref renders as a `cmp ax,K` chain (not
  jump table) is a `switch` — `if/else if` on a local emits `mov ax,[c]`
  differently. stepPanelAnim `<` `>` `^` `_` arm.
- `f->flag` value cached in si across a call (escort/site loops): declare
  `register int16 f = obj[i].flags` at loop-body top — but ONLY under `/Ot`;
  `/Oa` makes MSC CSE the field ADDRESS instead of the value, spills loop
  temps (`i*stride`, `&arr[sl]`, call results) to stack, and grows the
  frame. Re-read the field plainly for any test the ref recomputes —
  write `worldObjects[m2].targetFlags & 0x200` NOT `(f & 0x200)` when the
  ref does `mov bx,[m2]; shl bx,4; test word[bx+off]` afresh.
- `x != 0` (or `(uint16)x != 0`, `!!x`) materializes branchless:
  `cmp mem,1; sbb ax,ax; inc ax`. `x >= 1` on int16 emits `jl/jmp` branch
  arms, and the branch forces later terms into a deferred chunk. In a
  `switch (randMul(k) + (cond) + bias)` index expression, the branchy bool
  also flips the range guard (`ja`+inline dispatch vs ref's `jbe`+deferred
  dispatch block). runGenerator ground-unit switch.
- A switch jump table mid-routine must be marked as a `U` block in the
  map (`R7738-8323 U8324-8335 R8336-866b`) — mzdiff then skips ref's table
  bytes and guesses the target offset by equal U-block size.
- `== 0xffff` vs `== -1`: the unsigned constant emits `cmp ax,0FFFFh` and
  keeps ax live into a following `mov bx,ax`; `== -1` emits `inc ax; jz`
  + reload. Use the F15-spelling `0xffff`/`0xffffu` where ref reuses the
  compared value.
- Struct-globals sized too small silently collide: `missionMidX[4]` made
  `missionMidX[4..7]` writes land on the NEXT dseg global (mzdiff reports
  "data offset mapping collides"). Size stub arrays to the full indexed
  range seen in the ref (here `missionMidX[8]`).
- Switch cmp-chains emit case TESTS sorted by value ascending (8,0x18,0x1b)
  regardless of declaration order, but case ARMS emit in declaration order.
  To get ref's "test 8 first / arm 8 deferred last" layout, declare the
  clear cases first: `case 0x18: case 0x1b: ...; case 8: ...; default: ...`.
  pilotNameInput.
- `x = ++x % K` (pre-inc fused into the modulo expr) emits
  `inc [x]; mov ax,[x]; cwd; mov cx,K; idiv cx` — dividend loads BEFORE the
  divisor. `x++; x %= 6` or `x = x % 6` emit `mov cx,K` first.
  pilotNameInput blink counter.
- Char-buffer locals descend from the named-scalar area: `buf[0x50]`
  occupies -0x54..-0x05 with buf[0] at -0x54 (just below scalars at -2/-4),
  and later scalars (cursor,len) continue descending to -0x56/-0x58.
  Undersizing the array shifts them all up. pilotNameInput.
- A scalar `char` local still eats a full word slot; a `char[2]` array packs
  both bytes into ONE word slot. Dead byte-pair inits (`mov byte [bp-x],K`)
  emit in DECLARATION order, independent of the bucket-assigned slot order —
  pick names whose buckets reproduce the slot layout, then order the decls
  to reproduce the init stream. sub_1B452.
- `case K1: case K2:` sharing one body gets RANGE-folded by MSC
  (`cmp/jl/jle/jg` chains). Writing the identical body per case lets dedup
  coalesce them instead, emitting individual `cmp ax,K; jz` tests — and when
  the shared arm is out of `jz` reach, MSC relaxes to `jnz skip; jmp arm`,
  which is what the original shows. sub_1B452 (twice).
- `if (a[i].f1 != 0 && (a[i].f2 & M) == 0)` as ONE `&&` condition keeps
  `i*stride` CSE'd in `si` across both field reads; as two separate
  `if`s MSC recomputes the index in `bx` per test. sub_1B452 object scan.
- A 4-byte struct passed BY VALUE emits `push [bx+si+2]; push [bx+si]` —
  the ONLY idiom that splits table base (`si`) and scaled index (`bx`)
  into the two-reg EA; every scalar/pointer spelling folds to `[reg+disp]`.
  Declare `func(struct Row r)` and call `func(tab[i])`. sub_1ACA0 row loop.
- Jump-table operand bytes sit inside the routine extent: mark them `U`
  in the map (`R..x U tab R..`) or mzdiff disassembles the table as code.
- `for` vs `while` chunk layout: `for (i=0;i<n;i++)` emits
  `init; jmp TEST; INC:inc; TEST:cmp; jbe out; body; jmp INC` — the initial
  `jmp` forward lets pending DEFERRED chunks (earlier switch arms) land in
  the gap before INC/TEST. The same body as `while` emits the test INLINE
  (top-tested) instead. sub_1A68C row loop after the theater switch.
- CL heap exhaustion (`fatal error C1002`) on stubs.c under -DEXE_START:
  satellite overflow stubs go in a new `src_start/ststubs.c`-style module —
  satellite builds compile every srcdir *.c, so defs still link.
- Patched binaries (EN START.EXE sub_18F12): some sites were hand-patched
  after link — a `call sub_10AE8` NOP'd out (`push ax; nop nop nop` orphan
  remains) and `cmp ax,[r]; jz A` replaced by `mov [r],ax; jmp short A`
  forcing a branch. No C produces the patched bytes; write the unpatched
  semantics (confirmed vs the RU build) and mark each equal-size patch
  site a `U` block in the map.
- `while (n < K) { if (t <= K) continue; ...; n++; }` emits the ref's
  backward-test loop (`CMP: cmp; jge out; if-skip; body; inc; jmp CMP`) —
  the `continue` gives the early `jmp` to CMP, and the body falling out of
  the loop makes `n++` land where ref has it. A `do/while` there emits the
  test at the bottom instead. sub_18F12 wait loop.
- `f->a = f->b = 0` chained field stores emit `sub ax,ax; mov es:[bx+a],ax;
  mov es:[bx+b],ax` — far-struct field stores keep `es:bx` live and reuse
  the zeroed ax across the chain (no reload). sub_18F12 gameData wipe.
- A `mov byte [bp-<odd>],K` write into a word local's HIGH byte is a union
  alias: `union { int16 w; char b[2]; } u` gives `u.b[1]` → the odd byte.
  Plain `char` locals always take the low byte of their own word slot and
  byte arrays are word-aligned, so nothing else produces odd offsets.
  sub_193EE `sel`/`[bp-15h]`.
- Frames with never-touched slots are just more declared locals — MSC
  sizes `sub sp` by bucket-assigned slots regardless of use. A dead
  `char buf[2n]` occupies n consecutive word slots at its name's bucket
  position, which is the cheap way to cover a big dead span (sub_193EE's
  29-word frame with only 7 live locals).
- `(K - v) >> 1` on int16 operands emits `sar`; write `(uint16)(K - v) >> 1`
  for the ref's `shr` centering arithmetic. sub_193EE row loop.

## Routines that are asm in the original (do NOT port)

Every routine in the map is now named (zero `routine_*` left). The non-C
groups below stay as the asm skeleton emits them:

- File I/O cluster seg000:0xE1xx–0xE4xx (openFile/closeFile/picBlit/
  openBlitClosePic wrappers): hand-asm style (reg args, int21h, jmp error tail).
- Pic-decode cluster seg000:0xE4F8+ (picBlit/decodePicRow/picReadDataAndMakeDict/
  picMakeDict/doPicDecode/dictionaryLookup): hand-asm.
- Overlay trampolines seg000:0xe04e & 0xe0ae: MSC overlay-manager entry/exit
  stubs (`call far ptr sub_2F07A`/`sub_2F075` prologue/epilogue + far calls to
  overlay-resident sub_2F0xx, register-convention args in bx). Stay asm.
  (The resident file loaders they bracket — loadFileNear/loadFileSection/
  writeFileSection + the resFile* shims — are ported C in enfile.c/egui.c.)
- seg001 clip/outcode/3D-pipeline/rasterize helpers (the whole Code4 cluster:
  projectVertexToScreen, rotatePoint3d, transformModelVertices, transformVertexList,
  projectModelEdges, buildInverseRotationMatrix, multiplyMatrix3x3,
  transformAndCullObject, insertSortedObject, renderSortedList, processSceneObject,
  skipDisplayListByLod, renderHorizonSky, projectSceneObject, fillSpanRect,
  clipLine*, clipPointInside, rasterizeEdgeSpan, plus the *Far/*Thunk entry
  shims transformAndCullObjectFar, advanceModelPointerLod, renderSortedListFar,
  rotatePoint3dFar, transformModelVerticesFar, transformVertexListThunk,
  projectModelEdgesFar, multiplyMatrix3x3Far, drawModelDisplayList,
  clipLineFar, drawClipLineGlobal, resetScanlineSpans, clipAndRasterizeEdge,
  flushSpanDirtyRect, storeObjTransformByOpcode, drawFlatHorizon,
  testVisibilityMask, transposeOrientationMatrix, installDivZeroHandler,
  installDivZeroVector): register-convention asm — args in bx/si/di/cx, no C
  prologue. These are in f15se2's egseg1.asm.
  NOTE: some have `push bp`/`mov bp,sp` yet are still hand-asm (e.g.
  projectSceneObject repurposes bp as a scratch reg mid-body).
- seg002 drawInstrumentGauges(2)(Far), setupInstrumentLayout(Far), and the
  joystick cluster initJoystickCalibration/copyJoySample/
  readCalibratedJoystick/readJoyAxis/scaleJoyAxis/_copyJoystickData/
  _restoreJoystickData: register-convention asm (f15se2 egseg2.asm).
- seg003 setInt9Handler, restoreInt9Handler, seg000 installCBreakHandler:
  int21h/int9h handlers.
- seg000 sine/cosine interp cluster (sine, cosine, sinInterp, sineFar,
  cosineFar, cosineB, sinInterpB), getIntVector, gameLoopBody, readBiosTick,
  biosTickLo, initTimerState, writeBlock, nullsub1-4, crtPad0.
- MSC 5.1 CRT/libc tail seg000:0xe932+ — __cinit, _exit, __exit, __ctermsub,
  initterm/inittermfar, __FF_MSGBANNER, __chkstk, __nullcheck, __NMSG_TEXT,
  __NMSG_WRITE, __maperror, dosmaperr, _fclose, _fopen, _fread, __filbuf,
  __freebuf, __getbuf, __openfile, _fflush, __getstream, _close, _open,
  __cXENIXtoDOSmode, _read, _write, _stackavail, __nfree, __nmalloc,
  __amalloc, __amexpand, __amlink, __amallocbrk, _brkctl, brkctlFindSeg,
  _strcat, _strcpy, _strlen, _itoa, _kbhit, _getch, _int86, _movedata,
  _segread, _strupr, _memcpy, _abs, _srand, _rand, _remove, __bios_keybrd,
  __aNldiv, __aNlmul, __aNlshl, __aNlshr, __aNNalshl, __aNNalshr, __aNuldiv,
  __aNulshr. Identified by IDA FLIRT + SLIBCE.LIB byte matching.
- `start` (seg000:e880): DOS crt0.
- seg004 (the ~49KB reserved overlay/BSS area): all zeros in the image —
  the ~55 phantom `bss4_*` entries are map artifacts, not code.

## Verified C ports so far (all MATCH — 183)

eg3dload.c(/Os):  load3DAll, load3D3, load3DT, load3DG, printError
                  strcpyFromDot, load15Flt3d3
stparse.c(/Ot):   replaceExtension
enfile.c(/Ot):    loadFileNear, loadFileSection, writeFileSection
egui.c(/Os+/Oa):  loadColorPalette, drawModelPoint, drawViewportLine
                  drawClippedLineRegion, drawMapMarkerBox, projectMapPoint
                  blitGaugeSprite, blitSprite, drawStatusBar
                  formatMissionClock, formatTwoDigit, resFileOpen
                  resFileCreate, resFileClose, resFileRead, resFileReadFar
                  resFileWrite
eg3dmap.c(/Ot):   scaleCoordToLod, process3dg, findNearestTileObject
                  addTileEntry, lookupTileEntry, drawNearestTileObject
                  drawMapTiles, computeTileBounds, worldToTileIndex
                  drawMapTileObject, buildVertexSignMask
                  projectModelVertices, aspectScaleY, projectObjects
eg3dview.c(/Ot):  renderMapTerrain, setup3DTransform, rasterize3DWorld
                  setupViewport, setViewRotation, setViewPosition
egrender.c(/Ot+/Oa): render3DView, waitFrameSync
egmath.c(/Os):    isqrt, matVecDotAxis, drawWorldObject, clampRange
                  clampValue, rangeApprox, computeBearing, sinMul, cosMul
                  signExtendByte, signOf, seedRng, randomRange
                  readAxisInput
egflight.c(/Os+/Oa):applyRotationDelta, computeAttitudeAngles
                  rebuildOrientation, signedRatio16, valueToAngle
                  complementAngle, waitForKeyPress, drawAirspeedTape
                  insertOutlineEdges, renderFrame, stepFlightModel
egframe.c(/Os+/Oa):updateHudGauge, countermeasures, tickWeaponSlots
                  updateBulletsAndFire, updateTracerParticles
                  applyGravityFall, initFrameRandom, resetSimObjectLocks
                  initFlightParams, initWeaponLoadout, commitCommSnapshot
                  sendSoundCmd, recordFrame, buildStoreName, initStoreData
                  findWaypointFeatures, moveDataFar, moveStuff
                  moveNearFar, setCommWorldbufPtr, updateFrame
egtacmap.c(/Os+/Oa):projectWorldPoint, clearStatusPanel, renderHudFrame
                  updatePanelMode, drawPanelModeText, updateStatusPanel
                  initTacMapView, drawGaugeBar, redrawTacMap
                  zoomIn, zoomOut
                  mapXToScreen, mapYToScreen, plotMapObject
                  readMapPixelColor, drawMapArc, drawMapLine
                  drawFullscreenLine, drawScreenLineOnePage
                  drawHudViewLine, setDrawColor, fillRectBoth
                  drawMapPoint, drawStatusItem, drawPanelGridText
                  drawPanelText, fillPanelBox, drawCenteredLabelBox
                  drawStringBothPages, drawStringActivePage
                  drawStringCentered, drawNumber, readScreenPixel
                  hudMessage, getWeaponStat, drawTacticalMap
                  cacheScopePanel
                  restoreScopePanel, captureScopePanel, drawMissionObjectives,
                  drawThreatIndicator, drawTargetInfoPanel
                  drawLoadoutPanel, drawWeaponsPanel, drawAirTargetInfoPanel
                  drawWaypointPanel, updateTargetingHud
egcombat.c(/Os+/Oa):updateThreatSites, fireGroundThreat
                  computeThreatRangeBearing, updateThreatAlert
                  updateObjects, fireAirThreat, spawnEnemyAircraft
                  updateThreatTargeting, samCanAcquireTarget
                  destroySimObject, destroyGroundTarget, markTargetReached
                  bombTarget, fireMissile
egtarget.c(/Os+/Oa):drawHudWorldOverlay, drawTargetBox, drawLockReticle,
                  drawTargetLabel, buildRangeString, findStoreAtGrid,
                  bearingToStore, bearingToSimObject, computeTargetBearing,
                  hudPitchScale, getStoreMapCode, isTargetOverWater,
                  drawTargetView, shapeDataOffset, computeAimProjection
egkeys.c(/Ot):    makeSound, updateEngineSound, recalcTimeScale
                  setupLodDistances, exitTimeAccel, copyStoreToWaypoint
                  keyDispatch
egmain.c(/Os+/Oa):main, drawCockpit, runGameSession, gfxInit

## Verified semantically (NOT byte-exact)

Both routines below are Z3-proven semantically equivalent via
`tools/z3check.py` (see DECOMPILATION.md §5b): every comparable SSA part
matches after data-segment/string-offset normalization; remaining refusals
are `part_boundary_mismatch` artifacts of the extra prologue load.

- `spawnSamThreat` (egcombat.c, seg000:0x585c): instruction-for-instruction
  identical to the original except one prologue `mov si,[bp+4]` — MSC 5.1
  emits a register-param init because `off`'s first store sits past the guard
  branches. Byte-exact is unreachable under MSC 5.1 (si-dedication needs a
  register var = extra home slot, or a param = the init load). See
  DECOMPILATION.md §5. Z3: 17/17 compared parts proven.
- `drawWeaponRadarInfo` (egui.c, seg000:0xab2d): identical except prologue
  `mov si,[bp+8]`. Same mechanism: `register int16 off` (param, phantom arg3)
  is the only way to commit `si = weaponIdx*14` for the `[si+base+fieldoff]`
  record reads — the name(addr)+flag(test)+lethality+dangerTier cluster shares
  `i*14` in `si`, recomputed once after `strcpy("Maks dalxn.")`. Plain indexing
  or `off` locals give `bx`/`sub sp,2` instead. Byte-exact unreachable.
  Z3: 18 proven, 1 part mis-paired by the comparator (contents agree),
  2 boundary refusals.

RUNNABLE-EXE NOTE: `verify-exes`/portcheck compare LOAD IMAGES only. The
skeleton emits intra-image far calls/`seg` immediates as raw db bytes, so
LINK wrote ~1 MZ reloc entry where the original has 33-276 — an
unpatched exe hangs under DOS on unrelocated far refs. `make exes` runs
tools/mzhdrfix.py post-link: it grafts the original's reloc entries +
runtime header fields (minalloc/ss/sp/ip/cs/ovno) onto the rebuilt image,
so build/*_en.exe run correctly in DOSBox. The bytes DOS executes are
100% identical either way.
