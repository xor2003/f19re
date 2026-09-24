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
void closeFile(int16 h) {}

int8 g_lodShift;
int16 g_mapOriginX, g_mapOriginY, g_projErr, g_viewScale, g_projX, g_projY;
int16 mapMul(int16 a, int16 b) { return 0; }
int16 mapDiv(int16 a, int16 b) { return 0; }

uint8 g_radarScopeRange;
int16 g_mapCenterX, g_mapCenterY;
uint8 g_mapZoomLevel;
int16 g_viewX_, g_viewY_, g_projDepth, g_ourHead, g_vprojX, g_vprojY;
int16 sine(int16 a) { return 0; }
int16 fixedMulQ14(int16 a, int16 b) { return 0; }

struct GaugeParams { int16 bufPtr, srcX, srcY, page, dstX, dstY, width, height; };
struct GaugeParams gaugeSpriteParams;
int16 gfxBufPtr;
uint8 g_drawPage;
void gfx_blitSpriteClipped(int16 *p) {}
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
void updateEngineSound(void) {}
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
int16 *g_pageFront, *g_pageBack;
union REGS regs;

void drawClippedLineRegion(int16 a,int16 b,int16 c,int16 d,int16 e,int16 f,int16 g,int16 h,int16 i) {}

void FAR fillSpanRect(int16 a,int16 b,int16 c,int16 d,int16 e) {}
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
struct { int16 mapX,mapY,u4,type,ttl,uA; } mapEvents[4];
void appendMapEvent(int16 a, int16 b) {}

void refreshActivePanel(int16 a) {}

int16 g_scopeArcColor, g_targetBearing, g_targetRange, g_viewX_2, g_vprojXlo, g_vprojYlo;
char g_itoaScratch[24];

int16 g_inputDisabled, g_axisInputAccum[4], g_soundPriorityFloor, g_ejectState;
int16 g_frameRateScaling, g_frameSyncWait, g_timeAccelMode, g_bulletTrackCount;
int16 g_threatDisplayTtl;
int16 FAR misc_readJoystick(int16 a) { return 0; }
void FAR audio_playSound(int16 a) {}


uint8 g_aircraftModels[4];
int16 flt15_buf1[16];
/* egmath.c drawWorldObject stubs */
int32 g_ViewX, g_ViewY, g_camEyeX, g_camEyeY;
int16 g_camEyeZ;
int8  g_viewMode, g_halfScaleRender;
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

/* eg3dload.c load15Flt3d3 stubs */
#include <stdio.h>
char regnStr[32] = "STFLT.xxx";
int16 sign3d3;
size_t size3d3;
uint8 flt15_buf2[0x800];
FILE *fileHandle;
void strcpyFromDot(char *d, const char *s) {}
