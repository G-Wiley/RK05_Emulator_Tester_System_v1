/*--------------------------------------------------------------------------
**
**  Name: RK05BinInfo.cpp
**
**  Author: Tom Hunter
**
**  Description:
**      Display RK05 Emulator image information.
**
**--------------------------------------------------------------------------
*/

/*
**  -------------
**  Include Files
**  -------------
*/
#include "RK05Util.h"
/*
**  -----------------
**  Private Constants
**  -----------------
*/

/*
**  -----------------------
**  Private Macro Functions
**  -----------------------
*/

/*
**  -----------------------------------------
**  Private Typedef and Structure Definitions
**  -----------------------------------------
*/

/*
**  ---------------------------
**  Private Function Prototypes
**  ---------------------------
*/
static void printUsage(void);
static void verifyDiskImageData(void);

/*
**  ----------------
**  Public Variables
**  ----------------
*/
FILE *ifp;
FILE *ofp;

/*
**  -----------------
**  Private Variables
**  -----------------
*/
static u8 inBuf[Rk05SectorSize];
static bool verifySectors = false;
static bool longInfo = false;
static unsigned int calcCrc;
static int sectorcount;
static int headcount;
static int cylindercount;
static int sectorErrorCount = 0;

/*
**--------------------------------------------------------------------------
**
**  Public Functions
**
**--------------------------------------------------------------------------
*/

/*--------------------------------------------------------------------------
**  Purpose:        Program entry point.
**
**  Parameters:     Name        Description.
**                  argc        argument count
**                  argv        array of argument strings
**
**  Returns:        0 if normal termination, non-zero otherwise.
**
**------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
    // Process command line arguments.
    argv += 1;
    argc -= 1;

    while (argc > 0) {
        if (**argv != '-') {
            break;
        }

        if (strcmp(*argv, "-v") == 0) {
            argv += 1;
            argc -= 1;
            verifySectors = true;
        } else if (strcmp(*argv, "-l") == 0) {
            argv += 1;
            argc -= 1;
            longInfo = true;
        } else {
            printf("Unknown option %s\n", *argv);
            printUsage();
            }
        }

    if (argc != 1) {
        printUsage();
    }

    // Open the input file.
    ifp = fopen(argv[0], "rb");
    if (ifp == NULL) {
        printf("can't open %s\n", argv[0]);
        perror(" ");
        exit(1);
    }

    // Read, verify and display requested info.
    if (verifyImageFileHeader()) {
        displayImageFileHeader(longInfo);
        if (verifySectors) {
            verifyDiskImageData();
            if (sectorErrorCount == 0) {
                printf("Disk image is clean - no errors found\n");
            } else {
                printf("%d sectors with errors\n", sectorErrorCount);
                if (sectorErrorCount >= MaxSectorErrors) {
                    printf("Only the first %d sectors with CRC error have been listed\n", MaxSectorErrors);
                }
            }
        }
    }

    // Cleanup and exit.
    fclose(ifp);
    printf("\n");
    return 0;
}


/*
**--------------------------------------------------------------------------
**
**  Private Functions
**
**--------------------------------------------------------------------------
*/

/*--------------------------------------------------------------------------
**  Purpose:        Print short description of command and its parameters.
**
**  Parameters:     Name        Description.
**
**  Returns:        Nothing
**
**------------------------------------------------------------------------*/
static void printUsage(void)
    {
    printf("\nUsage:\n");
    printf("    RK05BinInfo [options] <emulator_image_file>\n");
    printf("Options:\n");
    printf("    -l       Detailed output.\n");
    printf("    -v       Verify CRC of all sectors.\n");
    printf("\n");
    exit(1);
    }

/*--------------------------------------------------------------------------
**  Purpose:        Verify the header word and CRC of all sectors.
**
**  Parameters:     Name        Description.
**
**  Returns:        Nothing
**
**------------------------------------------------------------------------*/
static void verifyDiskImageData(void)
{
    int rc;
    int bytecount = dataLength / 8;
    int sectorcount;
    int headcount;
    int cylindercount;
    int headerword;

    printf("\nVerifying sector headers and CRCs:\n");
    for (cylindercount = 0; cylindercount <  numberOfCylinders; cylindercount++){
        for (headcount = 0; headcount <  numberOfHeads; headcount++){
            for (sectorcount = 0; sectorcount <  numberOfSectorsPerTrack; sectorcount++){
                // Read the next RK05 emulator image sector.
                rc = fread(inBuf, 1, Rk05SectorSize, ifp);
                if (rc < Rk05SectorSize) {
                    if (ferror(ifp) != 0) {
                        printf("Read error C:%d, H:%d, S:%d\n", cylindercount, headcount, sectorcount);
                        return;
                    }
                }

                // Check the header word first.
                headerword = cylindercount << 5;
                if (   inBuf[0] != ((headerword >> 0) & 0xFF)
                    || inBuf[1] != ((headerword >> 8) & 0xFF)) {

                    if (sectorErrorCount++ < MaxSectorErrors) {
                        printf("Invalid header word at C:%d, H:%d, S:%d\n", cylindercount, headcount, sectorcount);
                    }
                }

                // Calculate CRC starting with the sector data including the stored CRC.
                // The result must be zero otherwise the sector has been corrupted.
                calcCrc = crc16buf(0, inBuf + 2, Rk05SectorSize - 2);
                if (calcCrc != 0) {
                    if (sectorErrorCount++ < MaxSectorErrors) {
                        printf("Invalid CRC at C:%d, H:%d, S:%d\n", cylindercount, headcount, sectorcount);
                    }
                }
            }
        }
    }
}

/*---------------------------  End Of File  ------------------------------*/
