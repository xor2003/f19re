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
- `/Oa` (assume no aliasing): needed where a global stays cached in a register
  across a pointer store — e.g. drawTargetView emits `push bx` not
  `push [gmem]` only under `/Oa`. Currently set on egtarget.c.
- Stack probing (`__chkstk` prologue) = module compiled WITHOUT `/Gs`.
- Local stack slots are assigned by variable-NAME hash, not decl order:
  bucket = sum(name bytes) % 16, buckets allocated ascending, same-bucket
  vars prepend (last-declared gets lower slot). Reusing f15se2's identifier
  names — or brute-forcing names to hit buckets — reproduces the frame.
- Far stream pointers: `p++; c=*p++` emits inc/bx/inc/es read pattern.
- `x = -y + K` compiles to `neg ax; add ax,K` (use unary minus, not `K - y`).
- 16-bit `labs(x)` (no int32 cast) emits `cmp ax,0x7fff`; a `(int32)` cast
  emits `cwd` instead.

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

## Verified C ports so far (all MATCH — 91)

eg3dload.c(/Os):  load15Flt3d3
stparse.c (/Ot):  replaceExtension
enfile.c  (/Ot):  loadFileNear, loadFileSection, writeFileSection
egui.c    (/Ot):  loadColorPalette, drawMapMarkerBox, projectMapPoint,
                  blitGaugeSprite, drawModelPoint, drawViewportLine,
                  resFileOpen, resFileCreate, resFileClose, resFileRead,
                  resFileReadFar, resFileWrite
eg3dmap.c (/Ot):  buildVertexSignMask, computeTileBounds, process3dg,
                  drawMapTileObject, drawMapTiles, worldToTileIndex,
                  aspectScaleY, projectModelVertices, drawNearestTileObject,
                  lookupTileEntry, addTileEntry, findNearestTileObject,
                  scaleCoordToLod
eg3dview.c(/Ot):  setup3DTransform, renderMapTerrain, setupViewport,
                  setViewRotation, setViewPosition
egmath.c  (/Os):  sinMul, cosMul, computeBearing, rangeApprox, clampRange,
                  clampValue, signExtendByte, signOf, seedRng, randomRange,
                  isqrt, readAxisInput, matVecDotAxis, drawWorldObject
egflight.c(/Os):  applyRotationDelta, computeAttitudeAngles, signedRatio16,
                  valueToAngle, complementAngle, rebuildOrientation,
                  waitForKeyPress
egframe.c (/Os):  moveStuff, moveDataFar, moveNearFar, setCommWorldbufPtr,
                  makeSound, recalcTimeScale, findWaypointFeatures
egtacmap.c(/Os):  readScreenPixel, readMapPixelColor, drawMapArc, drawMapLine,
                  drawFullscreenLine, drawScreenLineOnePage, fillRectBoth,
                  drawStringBothPages, drawNumber, cacheScopePanel,
                  restoreScopePanel, captureScopePanel, plotMapObject
egcombat.c(/Os):  updateThreatAlert, markTargetReached, bombTarget,
                  samCanAcquireTarget
egtarget.c(/Os+/Oa): drawTargetBox, drawLockReticle, drawTargetLabel,
                  buildRangeString, findStoreAtGrid, bearingToStore,
                  bearingToSimObject, computeTargetBearing,
                  isTargetOverWater, shapeDataOffset, drawTargetView

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
