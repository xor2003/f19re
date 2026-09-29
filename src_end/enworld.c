/* END.EXE — world data block reader (f15se2 enworld.c lineage) */
#include "inttype.h"
#include "pointers.h"

#define FP_OFF(p) (*(uint16 *)&(p))
#define FP_SEG(p) (*((uint16 *)&(p) + 1))

extern void far *commData;                    /* far ptr word_23C66 */
extern void movedata(uint16 sseg, uint16 soff, uint16 dseg, uint16 doff, uint16 len);
void readWorldData(void);
void loadWorldData(void *dest, int16 size);
int16 setupWorldBufPtr(void);
void readFromWorldBuf(void *dest, int16 size, int16 count, int16 handle);
void writeToWorldBuf(void *dest, int16 size, int16 count, int16 handle);

uint8 far *worldBufPtr;                        /* word_209B6/word_209B8 */
int16 worldBufHandle;                          /* word_2243A */
int16 worldDataReady;                          /* word_23782 */
char *worldStrings[100];                       /* word_237B2 */

int16 worldWaypointCount, worldObjectCount, worldRouteCount;
int16 worldRouteTable[1];
int16 worldObjects[1];
int16 worldSamCount, worldGridSize;
int16 worldSamTable[1];
int16 unitTypeTable[1];
int16 worldUnitFlags[1];
char  worldStringBuf[1];
int16 gridFlags[1];
int16 worldMiscHeader[1];
int16 weaponDataBlock[8];
int16 targetBlockWd[18];
int16 flightDataBuf[1];

/* seg000:0x03f7 — build worldStrings[] pointer table over the 750-byte pool */
void loadWorldStrings(void) {
    int16 i, j;        /* i = strIdx @[bp-2], j = byte pos @[bp-4] */
    setupWorldBufPtr();
    worldDataReady = 1;
    readWorldData();
    worldStrings[0] = worldStringBuf;
    i = 1;
    for (j = 0; j < 0x2ee; j++) {
        if (worldStringBuf[j] == 0 && i < 100) {
            worldStrings[i++] = &worldStringBuf[j + 1];
        }
    }
}

void readWorldData(void) {
    loadWorldData(&worldWaypointCount, 2);
    loadWorldData(&worldObjectCount, 2);
    loadWorldData(worldRouteTable, 2);
    loadWorldData(&worldRouteCount, 2);
    loadWorldData(worldObjects, worldObjectCount << 4);
    loadWorldData(&worldSamCount, 2);
    loadWorldData(worldSamTable, 36 * worldSamCount);
    loadWorldData(unitTypeTable, 100);
    loadWorldData(worldUnitFlags, 100);
    loadWorldData(worldStringBuf, 750);
    loadWorldData(gridFlags, 0x100);
    loadWorldData(&worldGridSize, 2);
    loadWorldData(worldMiscHeader, 2);
    loadWorldData(&weaponDataBlock, 16);
    loadWorldData(&targetBlockWd, 36);
    loadWorldData(flightDataBuf, 0x600);
}

/* seg000:0x0558 */
void loadWorldData(void *dest, int16 size) {
    if (worldDataReady != 0) {
        readFromWorldBuf(dest, size, 1, worldBufHandle);
    } else {
        writeToWorldBuf(dest, size, 1, worldBufHandle);
    }
}

/* seg000:0x059e — worldBufPtr = commData + 0x7a (pair load into ax:dx) */
int16 setupWorldBufPtr(void) {
    worldBufPtr = (uint8 far *)commData + 0x7a;
    return 1;
}

/* seg000:0x05c6 — copy from the in-memory world buffer */
void readFromWorldBuf(void *dest, int16 size, int16 count, int16 handle) {
    char far *farDest;
    farDest = (char far *)dest;
    movedata(FP_SEG(worldBufPtr), FP_OFF(worldBufPtr), FP_SEG(farDest), FP_OFF(farDest), size * count);
    FP_OFF(worldBufPtr) += size * count;
}

/* seg000:0x0605 — write back into the world buffer (f15 writeToWorldBuf) */
void writeToWorldBuf(void *dest, int16 size, int16 count, int16 handle) {
    char far *farDest;
    farDest = (char far *)dest;
    movedata(FP_SEG(farDest), FP_OFF(farDest), FP_SEG(worldBufPtr), FP_OFF(worldBufPtr), size * count);
    FP_OFF(worldBufPtr) += size * count;
}
