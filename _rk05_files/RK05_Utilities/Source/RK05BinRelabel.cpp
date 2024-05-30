/*--------------------------------------------------------------------------
**
**  Name: RK05BinRelabel.cpp
**
**  Author: Tom Hunter
**
**  Description:
**      Utility to update RK05 Emulator disk image "name" and "description"
**      in imageheader.
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
static bool setName = false;
static bool setDescription = false;

static char newName[11] = "";
static char newDescription[200] = "";
static char newDate[20] = "";

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

            safecpy(newName, *argv, sizeof(newName));
            setName = true;

            argv += 1;
            argc -= 1;
        } else if (strcmp(*argv, "-d") == 0) {
            argv += 1;
            argc -= 1;

            if (argc == 0) {
                printf("Missing 'image description' parameter\n");
                printUsage();
            }

            safecpy(newDescription, *argv, sizeof(newDescription));
            setDescription = true;

            argv += 1;
            argc -= 1;
        } else {
            printf("Unknown option %s\n", *argv);
            printUsage();
            }
        }

    if (argc != 1) {
        printUsage();
    }

    if (!(setName || setDescription)) {
        printf("Nothing to do - exiting\n");
        exit(1);
    }

    // Open the input file in read/write mode.
    ofp = ifp = fopen(argv[0],"rb+");
    if (ifp == NULL) {
        printf("Can't open %s", argv[0]);
        perror(" ");
        exit(1);
    }

    // Setup date & time string
    time_t timer;
    struct tm* tm_info;

    timer = time(NULL);
    tm_info = localtime(&timer);

    strftime(newDate, sizeof(newDate), "%Y-%m-%d %H:%M:%S", tm_info);

    // Read, verify and display requested info.
    if (verifyImageFileHeader()) {
        printf("Old settings:\n");
        displayImageFileHeader(false);

        // Update header variables.
        if (setName) {
            safecpy(imageName, newName, sizeof(imageName));
        }

        if (setDescription) {
            safecpy(imageDescription, newDescription, sizeof(imageDescription));
        }

        safecpy(imageDate, newDate, sizeof(imageDate));

        printf("\nNew settings:\n");
        displayImageFileHeader(false);

        // Rewind image file positon and write the image file header.
        if (fseek(ofp, 0, SEEK_SET) != 0) {
            printf("File positioning error in %s", argv[0]);
            perror(" ");
            exit(1);
        }

        if (!write_image_file_header()) {
            printf("Failed to write image header\n");
            exit(1);
        }
    }

    // Cleanup and exit
    fclose(ifp);
    printf("Relabelling completed\n");
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
    printf("    RK05BinRelabel [options] <emulator_image_file>\n");
    printf("Options:\n");
    printf("    -n <image_name>        - Image name (max 10 characters).\n");
    printf("    -d <image_description> - Image Description (max 199 characters).\n");
    exit(1);
    }

/*---------------------------  End Of File  ------------------------------*/
