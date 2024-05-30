/*--------------------------------------------------------------------------
**
**  Name: RK05Bin2Simh.cpp
**
**  Author: Tom Hunter
**
**  Description:
**      Convert RK05 Emulator disk image to SIMH RK05 disk image.
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
static void read_and_convert_disk_image_data(void);

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
static u8 outBuf[SimhSectorSize];
static bool stopOnError = true;
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

        if (strcmp(*argv, "-f") == 0) {
            argv += 1;
            argc -= 1;
            stopOnError = false;
        } else {
            printf("Unknown option %s\n", *argv);
            printUsage();
            }
        }

    if (argc != 2) {
        printUsage();
    }

    // Check if output file exists and prompt for overwrite.
    if (file_exists(argv[1])) {
       char buf[16];
       printf("Output file %s already exists - do you want to overwrite? (y/n): ", argv[1]);
       fgets(buf, sizeof(buf), stdin);
       if (_stricmp(buf, "y\n") != 0){
           return 0;
       }
    }

    // Open the input file.
    ifp = fopen(argv[0],"rb");
    if (ifp == NULL) {
        printf("Can't open %s", argv[0]);
        perror(" ");
        exit(1);
    }

    // Open output file.
    ofp = fopen(argv[1],"wb");
    if (ofp == NULL) {
        printf("Can't create %s", argv[1]);
        perror(" ");
        exit(1);
    }

    // Read header and display info.
    if (verifyImageFileHeader()) {
        displayImageFileHeader(false);

        // Do the actual conversion
        read_and_convert_disk_image_data();
        if (sectorErrorCount != 0) {
            printf("%d sectors with errors\n", sectorErrorCount);
            if (sectorErrorCount >= MaxSectorErrors) {
                printf("Only the first %d sectors with CRC error have been listed\n", MaxSectorErrors);
            }
        }
    }

    // Cleanup and exit
    fclose(ifp);
    fclose(ofp);
    printf("Conversion completed");
    if (sectorErrorCount != 0) {
        printf(", but the original was corrupted");
    }
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
    printf("Usage:\n");
    printf("    RK05Bin2Simh [options] <emulator_image_file> <simh_image_file> \n");
    printf("Options:\n");
    printf("    -f       Force conversion even if there are sector header or CRC errors.\n");
    exit(1);
    }

/*--------------------------------------------------------------------------
**  Purpose:        Perform the image conversion.
**
**  Parameters:     Name        Description.
**
**  Returns:        Nothing
**
**------------------------------------------------------------------------*/
static void read_and_convert_disk_image_data(void)
{
    int rc;
    bool zeroPad = false;
    int i;
    u8 *ip;
    u8 *op;
    u8 tmp;
    int headerword;

    printf("Converting RK05 Emulator image data to SIMH image format\n");
    for (cylindercount = 0; cylindercount <  numberOfCylinders; cylindercount++){
        for (headcount = 0; headcount <  numberOfHeads; headcount++){
            for (sectorcount = 0; sectorcount <  numberOfSectorsPerTrack; sectorcount++){
                // Read the next RK05 emulator image sector
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

                if (sectorErrorCount != 0 && stopOnError) {
                    printf("Conversion aborted - input file is corrupted\n");
                    exit(1);
                }

                // now generate the output buffer skipping the header in the input buffer
                ip = inBuf + 2;
                op = outBuf;

                // Convert the RK05 emulator binary format into the SIMH little endian 12 bit zero
                // padded format. The conversion is illustrated in the following commented code:
                //
                //    op[0] = 0x21;       ip[0] = 0x21;
                //    op[1] = 0x03;  <=   ip[1] = 0x43;
                //    op[2] = 0x54;       ip[2] = 0x65;
                //    op[3] = 0x06;

                for (i = 0; i < SimhSectorSize; i += 4) {
                    *op++ = *ip++;
                    *op++ = (*ip   >> 0) & 0x0F;
                    tmp   = (*ip++ >> 4) & 0x0F;
                    tmp  |= (*ip   << 4) & 0xF0;
                    *op++ = tmp;
                    *op++ = (*ip++   >> 4) & 0x0F;
                }

                // Output to file
                rc = fwrite(outBuf, 1, op - outBuf, ofp);
                if (rc != SimhSectorSize) {
                    printf("Write data error C:%d, H:%d, S:%d\n", cylindercount, headcount, sectorcount);
                    exit(1);
                }
            }
        }
    }
}

/*---------------------------  End Of File  ------------------------------*/
