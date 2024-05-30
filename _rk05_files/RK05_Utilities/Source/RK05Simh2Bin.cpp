/*--------------------------------------------------------------------------
**
**  Name: RK05Simh2Bin.cpp
**
**  Author: Tom Hunter
**
**  Description:
**      Convert SIMH RK05 disk image to RK05 Emulator disk image.
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
static u8 inBuf[SimhSectorSize];
static u8 outBuf[Rk05SectorSize];
static unsigned int calcCrc;
static int sectorcount;
static int headcount;
static int cylindercount;

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

        if (strcmp(*argv, "-n") == 0) {
            argv += 1;
            argc -= 1;

            if (argc == 0) {
                printf("Missing 'image name' parameter\n");
                printUsage();
             }

            safecpy(imageName, *argv, sizeof(imageName));

            argv += 1;
            argc -= 1;
        } else if (strcmp(*argv, "-d") == 0) {
            argv += 1;
            argc -= 1;

            if (argc == 0) {
                printf("Missing 'image description' parameter\n");
                printUsage();
            }

            safecpy(imageDescription, *argv, sizeof(imageDescription));

            argv += 1;
            argc -= 1;
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

    // Setup date & time string
    time_t timer;
    struct tm* tm_info;

    timer = time(NULL);
    tm_info = localtime(&timer);

    strftime(imageDate, sizeof(imageDate), "%Y-%m-%d %H:%M:%S", tm_info);

    // Write the image file header.
    if (!write_image_file_header()) {
        printf("Failed to write image header\n");
        exit(1);
    }

    // Do the actual conversion.
    read_and_convert_disk_image_data();

    // Cleanup and exit
    fclose(ifp);
    fclose(ofp);
    printf("Conversion completed\n");
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
    printf("    RK05Simh2Bin [options] <simh_image_file> <emulator_image_file>\n");
    printf("Options:\n");
    printf("    -n <image_name>        - Image name (max 10 characters).\n");
    printf("    -d <image_description> - Image Description (max 199 characters).\n");
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
    u8 *dp;
    u8 tmp;
    int headerword;

    printf("Converting SIMH image data to RK05 Emulator image format\n");
    for (cylindercount = 0; cylindercount <  numberOfCylinders; cylindercount++){
        for (headcount = 0; headcount <  numberOfHeads; headcount++){
            for (sectorcount = 0; sectorcount <  numberOfSectorsPerTrack; sectorcount++){
                if (!zeroPad) {
                    // Read the next SIMH sector
                    rc = fread(inBuf, 1, SimhSectorSize, ifp);
                    if (rc < SimhSectorSize) {
                        if (ferror(ifp) != 0) {
                            printf("Read error C:%d, H:%d, S:%d\n", cylindercount, headcount, sectorcount);
                            exit(1);
                        }
                        if (feof(ifp) != 0) {
                            // Pad trailing part of partial sector.
                            memset(inBuf + rc, 0, SimhSectorSize - rc);
                            zeroPad = true;
                        }
                    }
                } else {
                    // Pad all missing sectors.
                    memset(inBuf, 0, SimhSectorSize);
                }

                // Now generate the output buffer.
                ip = inBuf;
                op = outBuf;

                // Write the header word in LSB order first.
                headerword = cylindercount << 5;
                *op++ = (headerword >> 0) & 0xFF;
                *op++ = (headerword >> 8) & 0xFF;


                // Convert two little-endian format 12 bit words stored in 4 bytes into 3 x 8 bit words.
                // the conversion is illustrated in the following commented code:
                //
                //    ip[0] = 0x21;       op[0] = 0x21;
                //    ip[1] = 0x03;  =>   op[1] = 0x43;
                //    ip[2] = 0x54;       op[2] = 0x65;
                //    ip[3] = 0x06;

                dp = op;
                for (i = 0; i < SimhSectorSize; i += 4) {
                    *op++ = *ip++;
                    tmp  = (*ip++ >> 0) & 0x0F;
                    tmp |= (*ip   << 4) & 0xF0;
                    *op++ = tmp;
                    tmp  = (*ip++ >> 4) & 0x0F;
                    tmp |= (*ip++ << 4) & 0xF0;
                    *op++ = tmp;
                }

                calcCrc = crc16buf(0, dp, op - dp);

                // Write the CRC word in LSB order first
                *op++ = (calcCrc >> 0) & 0xFF;
                *op++ = (calcCrc >> 8) & 0xFF;

                // Output to file
                rc = fwrite(outBuf, 1, op - outBuf, ofp);
                if (rc != Rk05SectorSize) {
                    printf("Write data error C:%d, H:%d, S:%d\n", cylindercount, headcount, sectorcount);
                    exit(1);
                }
            }
        }
    }
}

/*---------------------------  End Of File  ------------------------------*/
