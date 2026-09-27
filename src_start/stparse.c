/* START.EXE — grid/terrain entry point + .3DT tile reader
 * (linker stparse.c; EN seg000:0x71f8 parseGridTerrain, 0x720c parseTerrain) */
#include "inttype.h"
#include <stdio.h>

extern FILE *fileHandle;                    /* dseg:0x98f4 */
extern char *regnPlhPtr;                    /* dseg:0x4dac */
extern void parseGrid(void);                /* seg000:0x73df */
extern void replaceExtension();             /* seg000:0x7534 */
extern int16 showMsgWaitKey(const char *m); /* seg000:0x7518 */

extern int16 terrainDirtyFlag;              /* dseg:0x3d0e */
extern int16 terrainSignature;              /* dseg:0x3bbe */
extern uint16 terrainBuf1[5];               /* dseg:0x3bc0 */
struct TerrainPtrTable { uint8 *entries[32]; };
extern struct TerrainPtrTable terrainTileCounts[5]; /* dseg:0x3bca */
extern struct TerrainPtrTable terrainTilePtrs[5];   /* dseg:0xce28 */
extern uint8 terrainTileBlock[];            /* dseg:0xa5c8 */
void parseTerrain(char *filename);

void parseGridTerrain(void) {               /* seg000:0x71f8 */
    parseGrid();
    parseTerrain(regnPlhPtr);
    terrainDirtyFlag = 0;
}

void parseTerrain(char *filename) {         /* seg000:0x720c */
    int16 tmp, level, tileOffset, entry;
    uint16 i;
    replaceExtension(filename, ".3dT");
    if ((fileHandle = fopen(filename, "rb")) == 0) {
        showMsgWaitKey("Open Error on *.3DT, assuming new file !");
    }
    else {
        fread(&terrainSignature,2,1,fileHandle);
        if (terrainSignature != 0x3131) {
            showMsgWaitKey("Bad Tile file format.");
        }
        else {
            fread(terrainBuf1,2,5,fileHandle);
                for (level = 0; level < 5; level++) {
                    if (terrainBuf1[level] > 0x20) {
                    showMsgWaitKey("Too many tiles.");
                    return;
                }
                fread(&terrainTileCounts[level],2,terrainBuf1[level], fileHandle);
            }
            tileOffset = 0;
            for (level = 0; level < 5; level = level + 1) {
                for (entry = 0; terrainBuf1[level] > entry; entry++) {
                    terrainTilePtrs[level].entries[entry] = (uint8*)terrainTileBlock + tileOffset;
                    for (i = 0; i < terrainTileCounts[level].entries[entry]; i++) {
                        if (tileOffset > 0xdac) {
                            showMsgWaitKey("Too much tile data");
                            return;
                        }
                        fread((uint8*)terrainTileBlock + tileOffset,2,1,fileHandle);
                        fread((uint8*)terrainTileBlock + 2 + tileOffset,2,1,fileHandle);
                        fread((uint8*)terrainTileBlock + 4 + tileOffset,2,1,fileHandle);
                        fread(&tmp,2,1,fileHandle);
                        *((uint8*)terrainTileBlock + 6 + tileOffset) = tmp;
                        tileOffset += 7;
                    }
                }
            }
        }
        fclose(fileHandle);
    }
}
