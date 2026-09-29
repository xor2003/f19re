#include "inttype.h"
#include <dos.h>
/* Link stubs for portcheck: globals and callees not yet ported.
 * Each ported module's externs resolve here so a test exe links. */
#include "inttype.h"

int16 g_mapX, g_mapY;
int16 g_clipMinX, g_clipMinY, g_clipMaxX, g_clipMaxY;
int16 g_drawColor, g_vtxX, g_vtxY;
int16 g_viewParamsBuf[16];
int16 *g_viewParams = g_viewParamsBuf;
int16 g_skyColorIndex;
int8  g_renderPageToggle, g_timerTick;
int32 g_viewTargetX, g_viewTargetY;
int16 g_viewTargetAlt, g_viewTargetObj;
int16 g_viewHeading, g_viewPitch, g_viewRoll;
int16 g_crashCamX, g_crashCamY, g_crashCamZ;
int16 g_viewClipBottom, g_camRotMatrix[9];
int8  g_horizonGroundColor, g_savedPosVisible;
struct ViewSnapshot { int32 worldX, worldY; int16 alt, heading, pitch, roll; };
struct ViewSnapshot g_viewSnapshotRing[16];
int16 g_rearViewShape[4], g_leftViewShape[4], g_rightViewShape[4], g_frontViewShape[4];
void far gfx_waitRetrace(void) {}
void far gfx_waitRetrace2(void) {}
char colorLut[16];
char g_colorPalettes[256];

#ifndef EXE_START /* src_start/stutil.c provides START's 5-arg drawLine */
void drawLine(int16 a, int16 b, int16 c, int16 d) {}
#endif
int16 openFile(const char *p, int16 m) { return 0; }
void picBlit(int16 h, int16 p, int16 m) {}
int16 closeFile(int16 h) { return 0; }
int16 createFile(const char *p, int16 a) { return 0; }
int16 readFile1(int16 h, int16 c, int16 o) { return 0; }
int16 readFile2(int16 h, int16 c, int16 o, int16 s) { return 0; }
int16 writeFileAtRaw(int16 h, int16 o, int16 bo, int16 bs, int16 c) { return 0; }

int8 g_lodShift;
int16 g_mapOriginX, g_mapOriginY, g_projErr, g_viewScale, g_projX, g_projY;
int16 mapMul(int16 a, int16 b) { return 0; }
int16 mapDiv(int16 a, int16 b) { return 0; }

int16 g_radarScopeRange;
int16 g_mapCenterX, g_mapCenterY;
int16 g_mapZoomLevel;
int16 g_externalCamDist;
int16 g_viewX_, g_viewY_, g_projDepth, g_ourHead, g_vprojX, g_vprojY;
int16 sine(int16 a) { return 0; }
int16 fixedMulQ14(int16 a, int16 b) { return 0; }

struct GaugeParams { int16 bufPtr, srcX, srcY, page, dstX, dstY, width, height; };
struct GaugeParams gaugeSpriteParams;
struct SpriteParams {
    int16 bufPtr, srcX, srcY, page, dstX, dstY, width, height;  /* +0x00..+0x0E */
    int16 pad16[4];         /* +0x10..+0x17 */
    uint8 flags;            /* +0x18 */
    uint8 transparent;      /* +0x19 */
};
struct SpriteParams blitSpriteParams;   /* @0x5856 */
int16 gfxBufPtr;
uint8 g_drawPage;
void gfx_blitSpriteClipped(int16 *p) {}
void gfx_blitSpriteOpaque(int16 *p) {}
int16 g_lineX1, g_lineX2, g_lineY1, g_lineY2, g_viewCenterX, g_viewCenterY;
char far *g_modelStreamPtr;
void gfx_setColor(uint8 c) {}
void drawClipLineGlobal(void) {}

int16 g_modelEdgeCount, g_vtxSignMaskLo = 0, g_vtxSignMaskHi = 0;
int8 g_modelWideVtxFlag;

struct VpParms { int16 f[11]; };
struct VpParms *g_vpParms;
int16 g_clipMaxX, g_clipMaxY;
int16 gfx_clipWindow(int16 a, int16 b) { return 0; }
void gfx_setOrigin(int16 a) {}
void gfx_restoreVp(void) {}
/* drawClippedLine: real port now lives in src_end/enbrief.c */

void gfx_nop23(void) {}

int8 g_objShade;
void far renderSortedListFar(void) {}
void far gfx_setBlitOffset2(void) {}

#pragma pack(1)
struct TileSceneObject { int16 x, y, z; uint8 shape; };
#pragma pack()
int16 g_mapOriginX, g_mapOriginY;
int16 g_mapLodIndex, g_curLod, g_modelEvenOddBit, g_tileZoomShift, g_tileWorldSize, g_tileGridDim;
const int16 g_mapTileLodTable[5] = {0};
int16 sign3dt;
uint16 sizes3dt[5];
uint8 buf_3dt[8192];
int16 size3d3_2;
int16 size3d3_3;
int16 size3d3_4;
int16 size3d3_5;
int16 size3d3_6;
int16 size3d3_7;
uint8 buf3d3_3[256];
uint16 g_modelOffsetTable[64];
uint16 g_modelVertX[256];
uint16 g_modelVertZ[256];
struct TileSceneObject *matrix3dt_2[5][32];
uint16 matrix3dt[5][32];
struct TileSceneObject *g_curTileEntry;
const uint16 buf3d3[1] = {0};
char far g_world3dData[1];
struct Proj3d { int32 x, y; int16 w; int32 z; };
struct Proj3d g_proj3d;
int16 g_objLocalX, g_objLocalY;
int16 g_objColorBase;
int16 g_lodObjectCount[5];
const int16 g_dirGridOffsets[72];
int16 far transformAndCullObjectFar(int16 a, int16 b, int16 c) { return 0; }

int16 g_viewCenterY2;

int16 g_hudVisible;
int8 g_halfScaleRender;
int16 g_overlayCenterX, g_overlayCenterY;
int16 g_clipMaxX, g_clipMaxY;
void far gfx_setOvlVal2(int16 a) {}
int16 far gfx_calcRowAddr(int16 a, int16 b) { return a; }
void far gfx_setBlitOffset(int16 a) {}
int16 g_viewRotMatrix[9];
void far buildRotationMatrixFar(int16 *m, int16 a, int16 b, int16 c) {}
void far drawProjectionSphere(int16 a) {}
int16 g_posVisibleFlag, g_detailLevel;
int8 g_offscreenRender, g_frameSyncPending;
int16 g_sortedObjCount, g_spinAngle, g_frameRateScaling;

int16 g_viewPosX, g_viewPosY, g_viewPosZ;
struct VtxScratchPad { char pad[0x1878]; } vtxScratch;
char far *g_modelStreamPtr;
int16 g_modelVtxCount;
int16 g_tileZoomShift;
uint8 buf3d3_1[128], buf3d3_2[128];
int16 g_modelVtxXTab[64], g_modelVertY[64];
void far transformModelVerticesFar(void) {}

int16 g_objDistance, g_curLod, g_modelEvenOddBit;
void far advanceModelPointerLod(void) {}
void far projectModelEdgesFar(void) {}
void far drawModelDisplayList(void) {}

/* findNearestTileObject externs */
struct { char pad[0x16]; } nearestTile;
struct { int16 gridX[9]; int16 gridY[11]; int16 lut[3]; } g_neighborSampling;
uint8 g_shapeTargetCategory[128];
int16 g_geeShakeToggle;		/* EN-only: key-toggled gee-warning override */
struct { uint8 lod, subIndex, tileX, tileY; int16 value; uint8 shape, pad7; } g_dynTileEntries[16];
int16 g_tileEntryIdx, g_render3DTiles;

int16 g_objRelX, g_objRelY;
int16 g_objTransform[4];
int16 g_objRenderMode;
int8 g_objHasRotation;
void far rotatePoint3dFar(void) {}

int16 g_lodGridDim[8];
uint8 g_topLodGrid[64], buf1_3dg[256], buf2_3dg[64], buf3_3dg[64], buf4_3dg[64];

int16 g_tileEntryCount;

int16 g_rotationCounter;
int8 g_orientationDirty;
int16 g_orientMatrix[9], g_matrixScratch[9];
void far multiplyMatrix3x3Far(const int16 *a, const int16 *b, int16 *c) {}

int16 g_ourPitch, g_ourHead, g_ourRoll, g_camSavedHead, g_camSavedRoll;
int8 g_rollWasNonzero;
int16 cosine(int16 a) { return a; }

const int16 g_angleLut[260] = {0};

void FAR audio_engineDroneOff(void) {}

int16 g_frameTimingAccum = 0;

int16 flagFarToNear = 0;
struct { int16 events[0x300]; } g_replayLog;
int16 g_landTargetId[2] = {0};
int16 g_waterTargetId[2] = {0};
int16 g_planeCount = 0;
int16 g_targetEntityCount = 0;
int16 g_planeScanCount = 0;
int16 g_groundUnitCount = 0;
int16 g_simObjects[4] = {0};
int8 g_tileKillTally[0x64] = {0};
int8 g_stringPool[0x2EE] = {0};
int8 g_mapCellFlags[0x100] = {0};
int16 g_unusedSavedWord = 0;
int16 g_padlockAircraft = 0;
int16 waypoints[8] = {0};

uint8 FAR *farPointer;
struct CommData FAR *commData;

int16 g_scopeClipLeft = 0, g_scopeClipRight = 0, g_scopeClipTop = 0, g_scopeClipBottom = 0;
int16 g_mapMode = 0;
int16 g_panelLabelOn;
int16 *g_pageFront, *g_pageBack;
union REGS regs;

void FAR fillSpanRect(int16 a,int16 b,int16 c,int16 d,int16 e) {}
uint8 far gfx_getDrawPage(void) { return 0; }
void  far gfx_setDrawPage(int16 a) {}
void FAR gfx_drawString(int16 *a,const char *b,int16 c) {}

int16 *g_pageOffscreen;
void FAR gfx_copyRect(int16 a,int16 b,int16 c,int16 d,int16 e,int16 f,int16 g,int16 h) {}
int16 pageFrontBuf[1] = {0}, pageBackBuf[1] = {0}, pageOffBuf[1] = {0};

int16 g_threatActiveTimer, g_threatTimerInit, g_threatRefX, g_threatRefY, g_threatRefZ, g_threatRefHead;
int16 g_unusedEventHist0, g_planeScanCount, g_missionStatus, g_difficultyTier;
int16 g_playerPlaneFlags, g_bombDamageMask, g_gunHits, g_damageTakenFlag;
int16 g_viewX_, g_viewY_, g_viewZ, g_ourHead, g_autopilotEngaged, waypointIndex;
char strBuf[64];
struct { int16 lead[3]; struct { int16 active; int16 f02; int16 alertLevel; int16 pad[5]; } planes[74]; } g_planeTable;
struct { int16 state; int16 pad[8]; } g_targetSlots[4];
int16 g_waypointNameBase;
struct TileObject;                  /* full def in eg3dmap.c / egtarget.c */
struct TileObject *g_nearestTileObj;
int16 g_rotMatrix[9];                 /* @0x80B6 — 3x3 camera/view rotation matrix */
int16 g_targetLock;                   /* word_351D8 — target-lock acquired flag */
int16 g_storeDefCount;                /* word_3838E — number of g_storeDefs entries */
int16 g_selGridX, g_selGridY, g_selTileId;  /* word_36F3A/3C/46 — store-query cache */
int16 g_selStoreState;                /* word_343BE — store-selection state */
struct { int16 mapX,mapY,u4,type,ttl,uA; } mapEvents[4];
void appendMapEvent(int16 a, int16 b) {}

void refreshActivePanel(int16 a) {}

int16 g_scopeArcColor, g_targetBearing, g_targetRange, g_viewX_2, g_vprojXlo, g_vprojYlo;
char g_itoaScratch[24];
struct { int8 name[8]; int16 lethality, dangerTier, flags; } g_samSpecs[16]; /* @0x4894 threat/weapon spec table (aNone) — 'Net ','SA-2',... 14-byte records */

int16 g_inputDisabled, g_axisInputAccum[4], g_soundPriorityFloor, g_ejectState;
int16 g_smokeTimer;                    /* word_34B0A — flag-0x20 duration counter */
int8 g_commEventFlag;                  /* byte_38D18 */
int16 g_frameRateScaling, g_frameSyncWait, g_timeAccelMode, g_bulletTrackCount;
int16 g_threatDisplayTtl;
int16 FAR misc_readJoystick(int16 a) { return 0; }
void FAR audio_playSound(int16 a) {}
void FAR audio_engineDroneOn(void) {}
int16 g_engineThrust;                   /* word_33588 */
int16 g_lodDistBase, g_lodDistScale, g_lodDistNear, g_lodDistFar;  /* word_2F870..876 */
int16 g_particles[32];                  /* @0x5260 struct Particle[8] ring */
int16 g_smokeSourceIdx, g_smokeParticleSlot;  /* word_343BE / word_34110 */
int16 g_maneuverTable[3*8*8];             /* @0x52A2 — [skill][relBearing][aspect] */
int16 g_activeThreatCount;                /* word_351CC */
int16 g_closestThreatIndex;               /* word_385D2 */
int16 g_hitMapX, g_hitMapY, g_hitAlt;     /* word_38378/38384/3838A */
int16 g_hitEffectTimer;                   /* word_35AE2 */
int16 g_prevKillMarker;                   /* word_351E0 */
int16 g_aamLeadDist;                      /* word_351E2 */
int16 g_axisInput1;                       /* word_351E4 */
int16 g_lgbCount;                         /* word_349AC */
int16 g_lgbTimer;                         /* word_349AE */


uint8 g_aircraftModels[4];
int16 flt15_buf1[16];
/* egmath.c drawWorldObject stubs */
int32 g_ViewX, g_ViewY, g_camEyeX, g_camEyeY;
int16 g_camEyeZ;
int16 g_viewMode;
int8  g_camExtFlag;
int16 g_aimClipSave;
int16 g_lockCooldown;
int16 g_lockMark;
int16 g_threatProxX;
int16 g_threatProxY;
int8  g_halfScaleRender;
int16 g_curLod;
void pascal shiftLongLeftInPlace(int16 c, int32 *p) { *p <<= c; }
void pascal shiftLongRightInPlace(int16 c, int32 *p) { *p >>= c; }
int16 FAR projectSceneObject(uint8 FAR *m, int16 a, int16 b, int16 c, int16 d, int16 e, int16 f) { return 0; }
/* drawTargetView stubs */
int16 g_targetInHudFlag, g_detailLevel, g_gfxModeUnset, frameTick;
int16 *g_targetViewParams;
int16 g_trkRoll, g_trkBearing, g_trkPitch, g_trkRange, g_trkSize, g_trkScale;
int16 g_viewX_, g_viewY_, g_ourHead, g_ourRoll, g_extViewPitch;
int8  g_extraScaleShift, g_offscreenRender;
uint16 FAR *g_viewParamsFar;
int16 sign3dg;
uint8 g_theaterGrids[2048];

/* eg3dload.c load15Flt3d3 stubs */
#include <stdio.h>
char regnStr[32] = "STFLT.xxx";
char *regnFile = "regn.xxx";
int16 sign3d3;
size_t size3d3;
uint8 flt15_buf2[0x800];
FILE *fileHandle;

int16 g_rngSeed;
int16 g_unusedLoadDoneFlag;
#ifndef EXE_START /* src_start/stutil.c provides the real definition */
int16 getTimeOfDay(void) { return 0; }
#endif
int16 g_trackedEnemyIdx;
int16 g_gunAmmo;
int16 g_fuelRemaining;
int16 g_stores[4][2];
int16 g_wpnSlots[0x18];               /* @0x5236: 4 weapon-slot records, stride 0xC */
int16 g_fireRecs[0x18];               /* @0x5230: stride-0xC fire records (g_wpnSlots aliases +6) */
int16 g_eventTimers[4];               /* @0x4EF8: countermeasure cooldown counters */
int16 bulletTracks[0x30];             /* @0x9BA6: stride-0xC tracer records {posX,posY,alt,velX,velY,velZ} */
int16 g_gunFiredFlag;                 /* word_354C6 */
int16 g_hudMsgTimer;
char  g_hudMessageBuf[64];

/* renderHudFrame (seg000:0x7e74) */
int8  g_hudDrawnFlag;                /* byte_330F5 */
uint8 joyAxes[2];                    /* @0x3345A */
uint8 hercFlag;                      /* byte_379C0 — setupMono copy for render loops */
int16 g_savedGfxOvl;                 /* word_38D28 — gfxOvlAddr saved by _main */

/* egmain.c callees — still asm/library */
int16 getOverlayLoadSeg(int16 slot) { return 0; }     /* sub_102B4 */
void  setupOverlaySlots(uint16 addr) { }              /* sub_103B2 */
void  installCBreakHandler(void) { }                  /* sub_11C83 */
void  restoreCbreakHandler(void) { }                  /* sub_11CA6 */
void  far gfx_initOverlay(void) { }                   /* sub_2F066 */
void  far gfx_setMonoFlag(int16 mono) { }             /* sub_2F1C4 */
void  far setupInstrumentLayoutFar(void) { }          /* sub_2208A */
void  far copyJoystickData(uint8 far *data) { }       /* sub_22D57 */
int16 far restoreJoystickData(uint8 far *data) { return 0; } /* sub_22D45 */
int16 far gfx_getModecode(void) { return 0; }         /* sub_2F165 */
#ifndef EXE_END /* real def in src_end/enaward.c */
void  openBlitClosePic(int16 picId, int16 p) { }      /* sub_1E40A */
#endif
void  initMissionStrings(void) { }                    /* sub_10446 */
void  far audio_shutdown(void) { }                    /* sub_2F223 */
void  far audio_setup(void) { }                       /* sub_2F21E */
void  setTimerIrqHandler(void) { }                    /* sub_11D12 */
void  restoreTimerIrqHandler(void) { }                /* sub_11D59 */
void  runGameLoop(void) { }                           /* sub_11CD2 */
void  far setInt9Handler(void) { }                    /* seg003:0x000e */
void  far restoreInt9Handler(void) { }                /* seg003:0x005e */
uint8 far *g_floppyMotorPtr;                          /* dword_354C2 */
char  *regnName = (char *)"regn.xxx";                 /* word_2EEE8 */
char  *scenarioPlh[8];                                /* @0x7A */
int16 g_cornerSpeed;                 /* word_38A10 — maneuvering-speed ref */
int16 g_knots;                       /* word_373E8 — airspeed, knots */
int16 g_climbRate;                   /* word_38D1E — vertical speed */
int16 g_groundAltitude;              /* word_3837A — terrain height under plane */
int16 g_rollPitchTrim;               /* word_354A6 — flight-path marker input */
int16 g_flightPathMarkerY;           /* word_384C4 — HUD marker row */
int16 g_aamSeekerX;                  /* word_38B0E — AAM seeker screen X */
int16 g_aamSeekerY;                  /* word_38B16 — AAM seeker screen Y */
int16 g_waypointBearing;             /* word_3836C — bearing to active waypoint */
int16 g_homeBaseIdx;                 /* word_37638 — home/carrier object slot */
void  far gfx_setDacAnimCount(int16 n) {}        /* sub_2F1B5 */


/* applyGravityFall + sendSoundCmd */
int16 g_wreckAlt;                      /* word_3845E */
int16 g_wreckFallVel;                  /* word_379B8 */
int16 g_wreckX, g_wreckY;              /* word_3837E / word_38392 — wreck world pos */
int16 g_liveObjCount;                  /* word_384FC — remaining live objects */
int16 g_selSimObj;                     /* word_343C4 — selected/viewed sim object idx */
int16 g_missionStage;                  /* word_37622 — campaign progress counter */
int16 g_currentWeaponType;             /* word_388C4 — current weapon/target type */
int16 g_airTargetLock;                 /* word_343BA — locked air target index */
int16 g_groundTargetLock;              /* word_343BC — locked ground target index */
int16 g_lockedTargetKilled;            /* word_35AE4 — locked target was destroyed */
int16 g_objTypes[0x40];                /* @0x49D6: 32B type records name[30]+kills */
void notifyViewObj(int16 idx) { }      /* sub_14C98: hwPortWrite view-target cmd */
int16 placeString(int16 idx) { return 0; }   /* sub_14D03: build target name */
int16 g_enemyGroundRemaining;        /* word_38500 — live ground-target count */
void hwPortWrite(int16 cmd) { }        /* sub_14CAC noop */
void far gfx_setFadeSteps(int16 n) { }    /* sub_2F15B */
int16 *g_mapTerrainMode;                  /* word_3468E */
void testWorldPosVisible(int16 x, int16 y, int16 z) { } /* sub_17E29: sets g_projClipFlag */
void sub_19979(void) { } void sub_19E4F(void) { } void sub_1A0BD(void) { }
void sub_1A300(void) { } void nullsub_3(void) { }
void projectVertex(int32 x, int32 y, int32 z) { }       /* sub_11372 */
void FAR gfx_drawStatusBox(int16 *p,int16 a,int16 b,int16 c,int16 d,int16 e,int16 f) { } /* sub_2F0F7 */
int16 g_weaponMask, g_curPanelMode, g_chaffCount, g_rocketCount;
struct CellRect { int16 x1, y1, x2, y2; } g_weaponCells[7];
int16 g_scanDir;                      /* word_38D1A */
int8 g_projClipFlag;                  /* byte_32242 */
int16 g_statCells[0x50];              /* struct StatCell[] @0x56AA */
struct StoreDef { int16 subIdx; uint16 coordX; uint16 coordY; int8 pad[8]; int16 nameIdx; };
struct StoreDef g_storeDefs[4];         /* @0x80C8 */
int8  g_airTargetMark, g_gndTargetMark; /* byte_38380 / byte_384E0 */
int8  g_classTab[0x80];                /* @0x95F0: nameIdx -> class nibble */
int8  g_statTab[8][0xD];               /* @0x512C: [statIdx][class] */
char *g_nameTab[0x80];                  /* @0x9696 */
char *g_nameTabBase;                    /* word_38506 = g_nameTab[0] slot */
char g_nameBuf[0x20];                   /* @0x65E6 */
char g_strpool[0x300];                  /* @0x9764 */
int16 g_selStoreIdx;                    /* word_37626 */
int32 g_worldX, g_worldY;               /* word_37CB8 / word_382D4 */
int16 g_scopeCenterX, g_scopeCenterY;   /* word_354A8/354AC */
int16 g_altitude, g_startRange;        /* word_33578/36E22 */
int16 missileSpecIndex;                /* word_33D80 — selected missile slot */
int16 g_lastMissileSlot;               /* word_36E1E — last fired projectile slot */
int16 computeLoftAngle(void) { return 0; }   /* sub_1CAF2 */
int16 missleSpec[0x10];                /* @0x4F00 4B {weaponIdx,ammo} records */
int16 missiles[0x140];                 /* @0x4F24 26B records */
int16 g_wpnSpriteX[4], g_wpnSpriteY[4];    /* @0x5968/@0x5970 sprite src */
int16 sams[0x80];                      /* @0x4C36 18B records */
int16 g_keyCode;                        /* word_384CE: pending keycode */
int16 g_threatScopeRange;               /* word_343B2: threat gauge level / scope range */
int16 g_scopeSweepTimer;                /* word_343C0: threat-scope sweep countdown */
int16 g_prevScopeRange;                 /* word_384F6: last drawn scope range */
int16 g_scopeArcRange;                  /* word_3831A */
int16 g_threatLabelTarget;              /* word_38372: >=0 plane idx, <0 ~idx simObjects */
int16 g_threatRadarFlag;                /* word_38B14 */
int16 g_scopeArcStart, g_scopeArcEnd;   /* word_383F8 / word_383FA */
int16 g_unusedEventHist1;               /* word_3845C: event-history shift stage 2 */
int16 g_threatToneLevel;                /* word_343C2 */
int16 g_enemyThreatCount;               /* word_36E24 */
int16 g_nearestThreatRange;             /* word_354CE */
int16 g_enemyAlertFlag;                 /* word_38502 */
int16 g_missionTimeLimit;               /* word_384C6 — mission deadline, ticks */
int16 g_threatSpec;                     /* word_351CA — spec of threat being prosecuted */
int16 g_northSouthSign;                 /* word_37484: theater N/S direction sign */
int16 dispatchKeyCmd(int16 key) { return 0; }   /* sub_1D4C6: key-command dispatch */
int16 frameTick, g_nightMode, g_unusedFrameVal, g_missionTick; /* 343B6/33D8A/35450/354C0 */
int16 g_wpPanelMode, g_wpSelectIdx;      /* word_37AB6 / word_33702 */
void drawFuelCell(int16 amount, int16 color) { } /* sub_19D5E */
void far gfx_setObjAttr(int16 a) { } /* sub_2F0CF */
void far beginEdgeGroup(void) { }   /* sub_21D2E */
void far insertEdge(void) { }       /* sub_21EB0 */
void far endEdgeGroup(void) { }     /* sub_21D18 */
int16 g_edgeQuad[4];                    /* word_32A09 */
int16 g_tapeClipX;                     /* word_346D8 */
int16 g_setupSlots[0x20];              /* @0x37622 */
int16 g_replayCount;                   /* word_351C4 */
struct Projectile { int16 mapX, mapY, alt, speed, worldX, worldY, worldZ, ttl, specIdx, weaponIdx, targetLock, targetRef; };
struct Projectile g_projectiles[12];   /* @0x5422: stride 0x18 guided-weapon slots */
int16 g_acqRange;                      /* word_351CE */
int16 g_samRange;                      /* word_33D00: SA-14 spec range factor (=16) */
int16 g_samSpeed;                      /* word_33D02: SA-14 spec speed (>>6 = 14) */
int16 g_acqAimY;                       /* word_351D0 */
int16 g_initPhase;                    /* word_38388 — mission init phase 0/1/2 */
int16 g_mapExtentX, g_mapExtentY;     /* word_36FEA / word_36FEC — theater map bounds */
int16 g_autopilotAltitude;            /* word_33D84 */
int16 g_fireCooldown;                 /* word_34B02 */
int16 g_unusedEventHist2;             /* word_384CC — event-history shift stage 3 */
int16 g_prevThreatIndex;              /* word_343C8 — previous g_closestThreatIndex */
int16 g_isCampaignMission;            /* word_33D88 — gameData->isCampaignMission copy */
int16 g_autoCrashDive;                /* word_354BE — low-altitude dive warning */
int16 g_inLandingCorridor;            /* word_343CA — inside landing-proximity box */
int16 g_landingTimer;                 /* word_343D2 — landing-progress counter */
int16 g_targetLeadAngle;              /* word_385D0 — drift/lead angle accum */
int16 g_frameRateAccum;               /* word_343CE — frame counter vs scaling*4 */
int16 g_markerPosX, g_markerPosY;     /* word_373EA / word_37488 — saved tac marker pos */
void far gfx_flipPage(int16 arg) { }  /* sub_2F17E */

/* stepFlightModel (seg000:0x215c) */
int16 g_thrust;                       /* word_37486 — smoothed throttle */
int16 g_rollInput, g_pitchInput;      /* word_384C8 / word_38A0E — stick inputs */
int16 g_gees;                         /* word_354BA — computed gees*16 */
int16 g_stallSpeed;                   /* word_36DDC — stall speed ref */
int16 g_liftForce;                    /* word_379BA — lift at current speed */
int8  g_geeTable[0x100];              /* @0x4624 — roll->gee LUT */
char  g_geeStrBuf[0x10];              /* @0x6640 — HUD gee string */
int16 g_joyCalibTimer;                /* word_3358A — joy-calib debounce */
int16 g_joySensitivity;               /* word_34B00 — setup sensitivity */
uint8 g_joyRawX, g_joyRawY;           /* @0x3345E/0x3345F — driver raw axes */
int8  g_highGeeFlag;                  /* byte_38B0A — g-meter needle flag */
int8  g_exitStatus;                   /* byte_2EEE5 — app exit code */
int16 g_yawMatrix[9], g_pitchMatrix[9], g_rollMatrix[9]; /* 0x46B8/0x46CA/0x46DC */
void far applyViewScaleMode(void) {}            /* sub_2208E (seg002) */
void far initJoystickCalibration(void) {}       /* sub_22C5E (seg002) */
void far readCalibratedJoystick(void) {}        /* sub_22C7F (seg002) */
void far audio_setEnginePitch(int16 a, int16 b) {} /* sub_2F23C (dseg stub) */
uint8 far gfx_getModeFlag(void) { return 0; }       /* sub_2F1A6 */
int16 far gfx_allocPage(int16 page) { return 0; }   /* sub_2F02A */
void  far gfx_storeBufPtr(int16 ptr, int16 n) { }   /* sub_2F1A1 */
void  setupDac(void) { }                            /* sub_11BB4 */
uint8 g_dacSupported;                               /* byte_2EEE4 */

/* ---- satellite (SU/START/END) shared extern stubs ---- */
#if !defined(EXE_START) && !defined(EXE_END) /* src_start/cleanup.c + src_end/enmain.c provide the real ports */
void  cleanup(void) { }
#endif
#ifdef EXE_START /* START-only callees (real defs live in the skeleton asm) */
void  intDispatch(int16 n, uint8 *a, uint8 *b) { }
void  far misc_clearKeyFlags(void) { }
/* stutil.c deps: driver-slot far calls + skeleton routines + globals */
int16 far misc_jump_5a_keybuf(void) { return 0; }
int16 far misc_jump_5b_getkey(void) { return 0; }
int16 far misc_jump_5d_readJoy(int16 a) { return 0; }
void  far misc_jump_5e_clearKeyFlags(void) { }
void  far gfx_resetBlitOffset2(void) { }
int16 far gfx_setFont(uint16 a, uint16 b) { return 0; }
void  far audio_jump_6b(void) { }
void  sub_151E8(int16 a, int16 b, int16 c, int16 d) { }
void  sub_141A3(void) { }
void  sub_18B7E(int16 a, int16 b, int16 c, int16 d) { }
void  sub_146E3(void) { }
void  sub_1DCAC(int16 a) { }
#ifndef EXE_START
char *formatGridRef(int16 a, int16 b, int16 c) { return 0; }
#endif
uint8 g_cntA, g_cntB, g_cntC, g_cntD;
uint8 cbreakHit;
struct GameData far *gameData;
/* stutil.c initGraphics/drawLine + stgrid.c deps */
void  far gfx_setPageN(uint16 a) { }
void  far gfx_setMode13(int16 a) { }
void  sub_140A3(void) { }
void  sub_151D4(void *d, int8 v, int16 n) { }
uint16 far *gfxModeSetPtr;
int16 g_gfxModeNum;
int16 g_lineX0, g_lineX1, g_lineY0, g_lineY1;
uint8 g_flagTable2CFE[16];
char *regnPlhPtr;
uint16 gridSignature;
int16 gridValidFlag;
uint8 gridBuf1[0x10];
uint8 gridBuf2[0x100];
uint8 gridBuf3[0x200];
uint8 gridBuf4[0x200];
uint8 gridBuf5[0x200];
int16 gridLevelSize[8];
int16 *nearestTerrainResult;
int16 readItemSize;
int16 *findNearestTerrain(int32 wx, int32 wy) { return 0; }
void  drawModelPoint(int16 a, int16 b, int16 c, int16 d, int16 e) { }
void  setViewPosition(int32 a, int32 b, int32 c) { }
/* stfile.c res-file shims deps */
int16 sub_14929(int16 a, int16 b, int16 c, int16 d) { return 0; }
int16 sub_149B9(int16 a, int16 b, int16 c, int16 d, int16 e) { return 0; }
int16 sub_1E172(int16 h, int16 b, int16 c, int16 mode) { return 0; }
void  sub_14C62(int16 fd, int16 sel) { }
void  sub_14B14(int16 fd, int16 a, int16 b) { }
void  sub_14B86(int16 fd, int16 sel) { }
/* stutil.c clipEntries deps */
struct ClipEntry { int16 x0,y0,w,h; int8 pad[0x52]; int8 flag; };
int16 viewOriginX, viewOriginY, clipEntryCount;
struct ClipEntry clipTable[1];
/* stutil.c stepPanelAnim (sub_16261) deps */
int16 *word_2170A, *word_2170C;                 /* dseg:0x170a/0x170c */
uint8 *word_2BE50;                              /* dseg:0xbe50 */
/* stgen.c parseWorld/exportWorldToComm globals */
int16 groundUnitCount;
int16 worldObjectCount;
int16 flightUnitCount;
uint8 wldReadBuf1[8];
uint8 wldReadBuf7[0x64];
uint8 wldReadBuf8[0x64];
uint8 objectTypeTable[0x64];
uint8 terrainGrid[0x100];
int8  wldReadBuf11[0x2ee];
int16 wldOffsets[0x64];
int16 missionDistAccum;
int16 escortMissionFlag;
int16 missionMidX[8];
uint8 targets[0x24];
int8  bufCoordStr[8];
/* stparse.c globals */
int16 terrainDirtyFlag;
int16 terrainSignature;
uint16 terrainBuf1[5];
struct TerrainPtrTable { uint8 *entries[32]; };
struct TerrainPtrTable terrainTileCounts[5];
struct TerrainPtrTable terrainTilePtrs[5];
uint8 terrainTileBlock[0x2860];
/* stgen.c missionGenerate globals */
int16 difficultySaved, theaterSaved, flag4Saved;
char *plhFiles[4] = {"lb.xxx","pg.xxx","nc.xxx","ce.xxx"};
/* stpinp.c saveHallfame globals + callees */
int16 *uiPage;
char  scrStrBuf[0x80];
int16 savedPage;
int16 doFcbSearch(void) { return 0; }
void  sub_14089(int16 n) { }
int16 far gfx_blitToCurrent(int16 p) { return 0; }
/* stutil.c drawTileIcon deps */
int16 flag_29948;
void  far gfx_blitSprite(int16 spr) { }
void  sub_14622(void *o, int16 a, int16 b, int16 c, int16 d) { }
int16 sub_13E38(int16 *p, char *s) { return 0; }   /* seg000:0x3e38 stringWidth dup */
/* stutil.c drawRiskPanel deps */
void *unitRec;
uint16 esTabBase;
int16 far *esTable[4];
int16 statT1[1], statT2[1], statT3[1], statT4[1];
char  scrStr[0x40];
void  sub_15120(char *d, char *s) { }
void  sub_13FB3(int16 n, char *b) { }
void  sub_13B76(void *o, char *s, int16 x, int16 y) { }
/* stutil.c printMission deps */
int16 briefParms, titleParms, briefPage;
char far **briefTab;
uint8 briefActive;
int8  gamePhase;
char *wldNameTab[8];
uint8 siteNameData[0x10], siteObjData[0x10];
char far *briefTextP;
void  sub_108B7(void) { }
char *sub_17558(int16 n, char *b, int16 t) { return b; }
/* printObjective (stutil.c) globals */
struct BriefTarget { int16 f[5]; char coord[6]; int16 dist; };
struct BriefTarget briefTargs[2];
struct MissionKind { int16 kind, pad2; uint8 flags; int8 pad5; int16 status, pad8[2]; };
struct MissionKind missionKinds[1];
int16 briefDepartSite, briefPatrolType;
char  unitNameTab[1][0x20];
char  briefTimeA[8], briefTimeB[8], briefCoord2[8];
int16 mystrlen(char *s) { int16 n = 0; while (s[n]) n++; return n; } /* END's is hand-asm; skeleton provides */
void  far gfx_commitPage(void) { }
/* runGenerator (stgen.c) globals */
int16 escortObj;                             /* dseg:0xbb72 */
int32 tgtPreciseX, tgtPreciseY;              /* dseg:0xc146,0xc1c6 */
char  briefTimeC[8];                         /* dseg:0x4dc2 */
uint8 loadoutTab[8 * 13];                    /* dseg:0x46f6 */
int16 missionSpeedTab[0x40];                 /* dseg:0x4506 */
struct SiteParm { int16 theaterMask, campMask, kind, reqType, flags, extra; };
struct SiteParm siteParms[0x38];             /* dseg:0x4b0c */
struct LinkPair { int16 nextA, nextB; };
struct LinkPair linkTab[0x20];               /* dseg:0x447e */
/* pilotNameInput (stpinp.c) globals */
int16 *page1Num;                             /* dseg:0x56fa */
int16 blinkColors[6];                        /* dseg:0x5930 */
uint8 blinkTimer;                            /* dseg:0x0a1c */
/* stmenu.c menu-screen globals */
struct MenuRow { int16 name, yoff; };
struct MenuRow *word_2B386;                  /* menu row-table near ptr */
uint8 far **word_2CA46;                      /* ptr→far menu string table */
int16 word_2CA48;                            /* second table selector */
uint8 far *word_207EA;                       /* far ptr→item count byte */
int16 *word_25F7C, *word_25D30, *word_25D48; /* page record handles */
int16 word_2D26E;                            /* sel-init broadcast value */
int16 selInitTab[10 * 15];                   /* dseg:0x25d4a, stride 30 */
struct MenuSelBlk { int16 sel; int16 pad[24]; };
struct MenuSelBlk menuSelTab[6];             /* dseg:0x25ea4, stride 0x32 */
struct MenuSelBlk menuSelTab2[6];            /* dseg:0x25fdc, stride 0x32 */
uint8 byte_2C160;                            /* next-screen selector */
struct MenuRow *word_2D276;                  /* second row-table near ptr */
int16 word_2C7D2;                            /* row-draw y cursor */
uint8 far *word_20822;                       /* far ptr→item count byte */
int16 *word_26050, *word_25F94, *word_25FAC; /* page record handles */
struct MenuSelBlk menuSelTab3[6];            /* dseg:0x26202, stride 0x32 */
int16 word_2D272;                            /* initTab2 broadcast value */
int16 initTab2[10 * 16];                     /* dseg:0x26094, stride 0x20 */
uint8 byte_2D060;                            /* briefing-block active flag */
int16 word_2D06C, word_2D2CA, word_2D2CC;    /* briefing string/rect args */
uint8 far *word_20862;                       /* far ptr→item count byte */
int16 *word_262A8, *word_2607A, *word_26092; /* page record handles */
struct MenuSelBlk menuSelTab4[6];            /* dseg:0x26308, stride 0x32 */
uint8 far *word_20896;                       /* far ptr→item count byte */
int16 *word_263AE, *word_262C0, *word_262D8; /* page record handles */
struct MenuSelBlk menuSelTab5[6];            /* dseg:0x2640c, stride 0x32 */
uint8 far *word_208BA;                       /* far ptr→item count byte */
int16 *word_26480, *word_263C6;              /* page record handles */
int16 word_2A0C4;                            /* rtc-bail arg */
void  sub_13B50(int16 *p, struct MenuRow r, int16 c, int16 d) { }
void  sub_10924(char *s, int16 t, int16 n, int16 a, int16 b, int16 *p, int16 f) { }
int16 sub_10AE8(char *s, int16 t, int16 n, int16 *sp, int16 *p, int16 f) { return 0; }
void  sub_125EA(void) { }
void  sub_14584(void *o, int16 a, int16 b, int16 c, int16 d) { }
void  sub_14A5F(char *s, int16 v) { }
void  sub_14746(char *s, int16 a, int16 b) { }
void  sub_1685C(int16 v) { }
int16 far gfx_unknown2b(int16 v) { return v; }   /* overlay slot 0x2b */
/* stmenu.c sub_1B452 (mission-setup screen) globals + overlay slots */
int16 word_2BE4A;                            /* scratch index */
char  far *word_209B6[4];                    /* theater name far-ptr table */
int8  byte_2B388;                            /* last theater */
uint8 byte_298F0, byte_2CA62, byte_2CA6A;    /* mode flags */
int16 word_2C7D4;                            /* mission kind selector */
int16 word_2D2C8;                            /* second sel-init value */
int16 *word_26572, *word_2655A;              /* page record handles */
int16 word_26574[10 * 16];                   /* dseg:0x6574, stride 0x20 */
int16 word_26592, word_265B2;                /* 0b4f-handler args */
int16 word_265B6, word_265D6;                /* row x coords */
int16 word_26676, word_26696;
int16 *word_2681E;                           /* selp arg */
char  *word_26820[8];                        /* list-label string table */
struct MenuSelBlk menuSelTab6[7];            /* dseg:0x66e2, stride 0x32 */
int16 word_2CA6C;                            /* mode-out code */
int16 word_2B948, word_2B95A;                /* excluded object indices */
uint8 byte_2C976, byte_2C977;                /* marks-present flag, toggle */
struct PgParms { int8 pad[0x22]; int16 f22; int8 pad1[0x0C]; int16 f30;
                 int8 pad2[0x40]; int16 f72; };
struct PgParms far *word_2D066;              /* far page parm record */
void  far ovlCall_b4f(int16 h) { }           /* overlay 1000:0b4f */
int16 far ovlCall_c53(void) { return 0; }    /* overlay 1000:0c53 */
void  far ovlCall_c58(void) { }              /* overlay 1000:0c58 */
void  far ovlCall_c8a(void) { }              /* overlay 1000:0c8a */
int16 far ovlCall_ccb(int16 v) { return v; } /* overlay 1000:0ccb poll */
#endif
void  dos_printstring(const char *s) { }
uint16 dos_alloc(uint16 size) { return 0; }
int16 dos_free(uint16 segment) { return 0; }
void  far gfx_setOvlVal1(int16 v) { }
void  far gfx_switchColor(int16 *p, int16 a, int16 b, int16 c, int16 d, int16 e, int16 f) { }
int16 selCursor;                             /* dseg:0x2c144 unit sel cursor */
int16 objCursor;                             /* dseg:0x2c968 object sel cursor */
int16 objectCount;                           /* dseg:0x2c978 */
int8  objectActive[1];                       /* dseg:0x2d278 */
int16 rtcEnabled;                            /* dseg:0xbb74 */
int8  rtcFlagByte;                           /* dseg:0x3e1f */
int16 rtcTickBuf;                            /* dseg:0x3e24 */
int16 rtcTickSaved;                          /* dseg:0x9920 */
void  sub_167FD(void) { }
void  sub_16208(void) { }
int16 sub_148FE(int16 h, int16 n, int16 b) { return h; }   /* int21h raw read */
void  sub_15152(char *d, const char far *s) { }            /* far-src strcpy */
int16 sub_15B22(int16 a, int16 b) { return a; }
int16 word_22322;                                         /* scratch buf cursor */
int16 word_2CA60, word_2CA64;                             /* map window bounds */
void  sub_14E9C(void) { }
void  sub_14EDA(void) { }
void  sub_16C0E(void) { }
uint8 byte_20A1A;
uint8 byte_20A1B;                           /* tick counter (sub_161F1) */
int16 word_21714;
int8  byte_212C2[4];
void  sub_149A1(int16 h) { }
int16 sub_16261(int16 e) { return e; }
int16 word_2367C;
uint8 unitMarksOn, tileMarksOn;
int8  tileMarkMap[4], sprParmsTab[4];
int16 unitSprOff, gridSprOff;
int16 siteTypeParms[9];
uint8 siteMarksOn;
int16 siteMarkCount, siteSprOff1, siteSprOff2;
void *objParms, *scoreRec;
char  str7964[4];
uint8 drawModeSel;
void far ovl_47B(int16 a,int16 b,int16 c,int16 d,int16 e,int16 f,int16 g,int16 h) {}
void far ovl_766(int16 a,int16 b,int16 c,int16 d,int16 e,int16 f,int16 g,int16 h) {}
void far ovl_169(int16 a,int16 b,int16 c,int16 d,int16 e,int16 f,int16 g,int16 h) {}
void far ovl_A65(int16 a,int16 b,int16 c,int16 d,int16 e,int16 f,int16 g,int16 h) {}
int16 missionTimeFlag;                        /* word_244E4: runGenerator sets 0/1 */
void  drawLineWrapper(void) { }
#if !defined(EXE_START) && !defined(EXE_END) /* src_start/stmap.c + src_end/enbrief.c provide the real ports */
int16 mapToScreenX(int16 v) { return v; }
#endif
#if !defined(EXE_START) && !defined(EXE_END) /* src_end/enbrief.c provides the real port */
int16 mapToScreenY(int16 v) { return v; }
#endif
#ifndef EXE_END /* src_end/enbrief.c provides the real port */
void  drawMapPixel(int16 x, int16 y, int16 c) { }
#endif
#if !defined(EXE_START) && !defined(EXE_END) /* stutil.c/enstr.c provide the real ports */
void  mystrcpy(char *d, const char *s) { }
#endif
#ifndef EXE_END /* src_end/enworld.c provides the real port */
void  loadWorldData(void *d, int16 s) { }
#endif
/* stutil.c drawUnitList deps */
int16 selRowIdx, flag2CA4C, flag2C7CE, word_2CA60, word_2CA64;
uint8 byte_2C9E0;
int16 selAvailTab[4], typeIdxTab[4];
char *namePtrTab[2], *typeNameTab[2];
char  strB96A[8], str282[4];
#ifndef EXE_START /* src_start/stpanel.c provides the real port */
void  sub_12754(void *t, int16 i, int16 *pd) { }
#endif
void  sub_13218(void *t, int16 i, int16 *pd) { }
#ifndef EXE_START /* src_start/stutil.c provides the real ports */
void  wrapUnitText(int16 a, char *s, int16 w, int16 x, int16 y, int16 b) { }
void  wrapUnitTextFar(int16 a, char far *s, int16 w, int16 x, int16 y, int16 b) { }
#endif
void  sub_151FE(char *d, uint8 *s, int16 n) { }        /* near copy */
void  sub_1521C(char *d, char far *s, int16 n) { }     /* far copy */
uint32 rngState;                                       /* dseg:0x7a50 LCG state */
int16 sub_150BA(void) { return 0; }                    /* int 1Ah tick read */
/* stutil.c drawRoutePath deps */
int16 pathWpA, pathWpB, pathWpC, pathWpD;              /* dseg:0xb94a/48/5a/5c */
char  str682E[4], str6832[4], str6834[4], str6836[4], str6838[4];
/* stutil.c drawThreatRings deps */
uint8 ringMode;                                        /* dseg:0x9922 */
uint8 ringTypes[0x40];                                 /* dseg:0x3e26 */
#ifdef EXE_END /* END-only callees (real defs live in the skeleton asm) */
void  seekFileAt(int16 fd, int16 off, int16 whence) { }                    /* sub_11694 */
void  fileClose(int16 fd) { }                                              /* 0x145c — int21/3Eh asm */
void  picStreamRead(int16 fd) { }                                          /* 0x1521 — asm block reader */
void  decodePic(int16 fd, int16 page) { }                                  /* 0x17e2 — asm decoder */
int16 picBufPos;                                                           /* dseg:0x1d4c */
uint8 picStreamBuf[4];                                                     /* dseg:0x194a */
void  memsetFar(uint8 far *d, int16 v, uint16 n) { }                       /* sub_13982 — rep-stosb asm in skeleton */
int16 readBiosTickLo(void) { return 0; }               /* sub_13844 */
void  seedRandom16(int16 v) { }                        /* sub_18C84 */
void  far gfx_getCurPage(int16 a) { }
int16 far gfx_charWidth(int16 ch, int16 font) { return 0; }
int16 far gfx_initDone(void) { return 0; }
void  far gfx_setPageN(uint16 a) { }
void  far gfx_commitPage(void) { }
void  drawMenuItem(void *items, int16 i, int16 *p) { }
int16 curRecordIdx;                             /* word_22C36 */
uint16 colorStyleTable[8];                      /* 0x41DE */
int8  flightRecords[64];                        /* byte_22F14 */
uint8 slotInfoTable[16];                        /* word_2245E */
int16 spriteAirBlink, spriteSamBlink, spriteGroundBlink, spriteWaypointBlink;
int16 spriteAir, spriteGround, spriteSam, spriteWaypoint;  /* normal-sprite ptrs word_1F6C4/1F704/1F744/1F7C4 */
int16 mapWinX1, mapWinY1, mapWinX2, mapWinY2;              /* word_1DF8E/90/92/94 debrief map window */
void  far pollJoystick(void) { }
int16 far misc_jump_5a_keybuf(void) { return 0; }
int16 far misc_jump_5b_getkey(void) { return 0; }
int16 far misc_jump_5d_readJoy(int16 a) { return 0; }
void  far misc_jump_5e_clearKeyFlags(void) { }
void  far gfx_blitSprite(int16 spr) { }
void  intDispatch(int16 n, uint8 *a, uint8 *b) { }     /* sub_139FD */
void  farStrcpy(char *d, char far *s) { }            /* sub_138EC — asm in skeleton */
void  sub_1288B(int16 recOff) { }                    /* rec-anim worker — real def in skeleton */
int16 runMapView(int16 sel) { return 0; }            /* sub_12192 — big map dispatcher, skeleton */
void  picStageRefill(void) { }                       /* sub_13238 — rep-movsw stage copy asm */
void  far textOp_477(void) { }   /* overlay text ops at seg 0x91d */
void  far textOp_762(void) { }
void  far textOp_165(void) { }
void  far textOp_A61(void) { }
uint8 drawTextMode;                                  /* dseg:0x8b76 — initResultFlag lo */
uint8 tickByte;                                      /* byte_1DF6B */
int16 tickArm;                                       /* word_1C6EC */
uint8 recActive[4], recField9[4], recField8[4];      /* byte_19DEA/word_19D99/word_19D98 field views */
uint8 recTable[4];                                   /* 0x295e-stride-0x5c record table */
int16 recCount;                                      /* word_3CB4 */
void  copyBytes(char *d, char *s, int16 n) { }       /* sub_13998 — asm in skeleton */
void  memcpyFromFar(char *d, char far *s, int16 n) { } /* sub_139B6 — asm in skeleton */
#endif
/* stutil.c drawStoreIcons deps */
int16 word_298E6;                                     /* dseg:0x98e6 */
int16 word_27990[4], word_27998[4];                   /* dseg:0x7990/0x7998 */
int16 far gfx_getVal(void) { return 0; }              /* slot 0x4e */
void  far gfx_setDac(int16 n) { }                     /* slot 0x44 */
