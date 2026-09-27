/* END.EXE — world data block reader (f15se2 enworld.c lineage) */
#include "inttype.h"

extern void loadWorldData(void *dest, int16 size);

int16 worldWaypointCount, worldObjectCount, worldRouteCount;
int16 worldRouteTable[1];
int16 worldObjects[1];
int16 worldSamCount, worldGridSize;
int16 worldSamTable[1];
int16 unitTypeTable[1];
int16 worldUnitFlags[1];
int16 worldStringBuf[1];
int16 gridFlags[1];
int16 worldMiscHeader[1];
int16 weaponDataBlock[8];
int16 targetBlockWd[18];
int16 flightDataBuf[1];

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
