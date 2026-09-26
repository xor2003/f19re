# F-19 Stealth Fighter reconstruction

Goal: recover F19 (MicroProse, MSC 5.1) as C source for eventual merge into
/home/xor/games/f15se2-re. Approach: byte-exact asm skeleton + incremental C
ports verified against the original binary.

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

## MSC 5.1 codegen facts (F19)

- Small model `/AS`: near code+data; cross-segment callees declared `far`.
- Optimize on: `/Gs` + `/Os` or `/Ot` — never `/Od`. The choice is per-module
  and recorded in `tools/portcheck.py` `MODULE_FLAGS`; `/Os` vs `/Ot` shows on
  routines with early returns (shared vs inlined epilogue).
- MSC 5.1 has no working `volatile` (accepted syntactically, ignored). Code
  touching timer/interrupt-updated globals was likely shipped under `/Zi`
  (debug) because optimized builds broke — if a routine resists matching under
  `/Os` or `/Ot`, check whether it was ever optimized at all.
- `/Oa` (assume no aliasing): needed where a global stays cached in a register
  across a pointer store — e.g. drawTargetView emits `push bx` not
  `push [gmem]` only under `/Oa`. Currently set on egtarget.c.
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

## Routines that are asm in the original (do NOT port)

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
  clipLine*, clipPointInside, rasterizeEdgeSpan, etc.): register-convention asm —
  args in bx/si/di/cx, no C prologue. These are in f15se2's egseg1.asm.
  NOTE: some have `push bp`/`mov bp,sp` yet are still hand-asm (e.g.
  projectSceneObject repurposes bp as a scratch reg mid-body).
- seg002 drawInstrumentGauges + helpers: register-convention asm.
- seg003 setInt9Handler, seg000 installCBreakHandler: int21h/int9h handlers.
- `start` (seg000:e880): DOS crt0.

## Verified C ports so far (all MATCH — 172)

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
egmath.c(/Os):    isqrt, matVecDotAxis, drawWorldObject, clampRange
                  clampValue, rangeApprox, computeBearing, sinMul, cosMul
                  signExtendByte, signOf, seedRng, randomRange
                  readAxisInput
egflight.c(/Os):  applyRotationDelta, computeAttitudeAngles
                  rebuildOrientation, signedRatio16, valueToAngle
                  complementAngle, waitForKeyPress, drawAirspeedTape
                  insertOutlineEdges
egframe.c(/Os+/Oa):updateHudGauge, countermeasures, tickWeaponSlots
                  updateBulletsAndFire, updateTracerParticles
                  applyGravityFall, initFrameRandom, resetSimObjectLocks
                  initFlightParams, initWeaponLoadout, commitCommSnapshot
                  sendSoundCmd, recordFrame, buildStoreName, initStoreData
                  findWaypointFeatures, moveDataFar, moveStuff
                  moveNearFar, setCommWorldbufPtr
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
egtarget.c(/Os+/Oa):drawTargetBox, drawLockReticle, drawTargetLabel
                  buildRangeString, findStoreAtGrid, bearingToStore
                  bearingToSimObject, computeTargetBearing, hudPitchScale
                  getStoreMapCode, isTargetOverWater, drawTargetView
                  shapeDataOffset, computeAimProjection
egkeys.c(/Ot):    makeSound, updateEngineSound, recalcTimeScale
                  setupLodDistances, exitTimeAccel, copyStoreToWaypoint

## Verified semantically (NOT byte-exact)

- `spawnSamThreat` (egcombat.c, seg000:0x585c): instruction-for-instruction
  identical to the original except one prologue `mov si,[bp+4]` — MSC 5.1
  emits a register-param init because `off`'s first store sits past the guard
  branches. Byte-exact is unreachable under MSC 5.1 (si-dedication needs a
  register var = extra home slot, or a param = the init load). See
  DECOMPILATION.md §5.
- `drawWeaponRadarInfo` (egui.c, seg000:0xab2d): identical except prologue
  `mov si,[bp+8]`. Same mechanism: `register int16 off` (param, phantom arg3)
  is the only way to commit `si = weaponIdx*14` for the `[si+base+fieldoff]`
  record reads — the name(addr)+flag(test)+lethality+dangerTier cluster shares
  `i*14` in `si`, recomputed once after `strcpy("Maks dalxn.")`. Plain indexing
  or `off` locals give `bx`/`sub sp,2` instead. Byte-exact unreachable.
