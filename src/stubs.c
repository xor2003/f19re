#include "inttype.h"
#include <dos.h>
/* Link stubs for portcheck: globals and callees not yet ported.
 * Each ported module's externs resolve here so a test exe links. */
#include "inttype.h"

int16 g_mapX, g_mapY;
int16 g_clipMinX, g_clipMinY, g_clipMaxX, g_clipMaxY;
int16 g_drawColor, g_vtxX, g_vtxY;
char colorLut[16];
char g_colorPalettes[256];

void drawLine(int16 a, int16 b, int16 c, int16 d) {}
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
void drawClippedLine(void) {}

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

int16 g_ourPitch, g_ourHead, g_ourRoll;
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


uint8 g_aircraftModels[4];
int16 flt15_buf1[16];
/* egmath.c drawWorldObject stubs */
int32 g_ViewX, g_ViewY, g_camEyeX, g_camEyeY;
int16 g_camEyeZ;
int16 g_viewMode;
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
int16 getTimeOfDay(void) { return 0; }
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
void redrawTacMap(int16 x, int16 y) { }   /* sub_187EC */
void sub_19979(void) { } void sub_19E4F(void) { } void sub_1A0BD(void) { }
void sub_1A300(void) { } void nullsub_3(void) { }
void projectVertex(int32 x, int32 y, int32 z) { }       /* sub_11372 */
void FAR gfx_drawStatusBox(int16 *p,int16 a,int16 b,int16 c,int16 d,int16 e,int16 f) { } /* sub_2F0F7 */
int16 g_weaponMask, g_curPanelMode;   /* word_33D64 / word_385CE */
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
int16 g_threatSpec;                     /* word_351CA — spec of threat being prosecuted */
int16 g_northSouthSign;                 /* word_37484: theater N/S direction sign */
int16 dispatchKeyCmd(int16 key) { return 0; }   /* sub_1D4C6: key-command dispatch */
int16 frameTick, g_nightMode, g_unusedFrameVal, g_missionTick; /* 343B6/33D8A/35450/354C0 */
int16 g_setupSlots[0x20];              /* @0x37622 */
int16 g_replayCount;                   /* word_351C4 */
struct Projectile { int16 mapX, mapY, alt, speed, worldX, worldY, worldZ, ttl, specIdx, weaponIdx, targetLock, targetRef; };
struct Projectile g_projectiles[12];   /* @0x5422: stride 0x18 guided-weapon slots */
int16 g_acqRange;                      /* word_351CE */
int16 g_samRange;                      /* word_33D00: SA-14 spec range factor (=16) */
int16 g_samSpeed;                      /* word_33D02: SA-14 spec speed (>>6 = 14) */
int16 g_acqAimY;                       /* word_351D0 */
