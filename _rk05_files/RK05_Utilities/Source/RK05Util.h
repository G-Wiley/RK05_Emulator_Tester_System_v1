/*--------------------------------------------------------------------------
**
**  Name: RK05Util.h
**
**  Author: Tom Hunter
**
**  Description:
**      RK05 utility declarations.
**
**--------------------------------------------------------------------------
*/

/*
**  -------------
**  Include Files
**  -------------
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>

/*
**  -----------------
**  Private Constants
**  -----------------
*/
#define MaxSectorErrors     10
#define SimhSectorSize   ((256 * 16) / 8)
#define Rk05SectorSize   (((256 * 12) / 8) + 2 + 2)

/*
**  -----------------------
**  Private Macro Functions
**  -----------------------
*/
#if defined (__GNUC__)
#define _stricmp strcasecmp
#endif

/*
**  -----------------------------------------
**  Private Typedef and Structure Definitions
**  -----------------------------------------
*/
typedef signed char  i8;
typedef signed short i16;
typedef signed long  i32;
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;

/*
**  --------------------------
**  Public Function Prototypes
**  --------------------------
*/
bool file_exists(const char *filename);
void safecpy(char *dst, char *src, int n);
bool serialize_int(int value);
bool serialize_string(char *cp, int size);
bool deserialize_int(int *vp);
bool deserialize_string(char *cp, int size);
bool write_image_file_header(void);
bool verifyImageFileHeader(void);
void displayImageFileHeader(bool detailed);
u16 crc16buf(u16 crc, const u8 *bp, int size);

/*
**  ----------------
**  Public Variables
**  ----------------
*/

extern FILE *ifp;
extern FILE *ofp;

extern char magicNumber[10];
extern char versionNumber[4];

extern char imageName[11];
extern char imageDescription[200];
extern char imageDate[20];

extern char controller[100];
extern int bitRate;
extern int preamble1Length;
extern int preamble2Length;
extern int dataLength;
extern int postambleLength;
extern int numberOfCylinders;
extern int numberOfSectorsPerTrack;
extern int numberOfHeads;
extern int microsecondsPerSector;

/*---------------------------  End Of File  ------------------------------*/
