/*--------------------------------------------------------------------------
**
**  Name: RK05Util.cpp
**
**  Author: Tom Hunter
**
**  Description:
**      RK05 utility functions.
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

#include "RK05Util.h"

/*
**  -----------------
**  Private Constants
**  -----------------
*/
#define MAX_SBUF 256

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

/*
**  ----------------
**  Public Variables
**  ----------------
*/

extern FILE *ifp;
extern FILE *ofp;

char magicNumber[10] = "\x89RK05\r\n\x1A"; 
char versionNumber[4] = "1.0";

char imageName[11] = "";
char imageDescription[200] = "";
char imageDate[20] = "";

char controller[100] = "RK8-E";
int bitRate = 1440000;
int preamble1Length = 120;
int preamble2Length = 82;
int dataLength = 3104;
int postambleLength = 36;
int numberOfCylinders = 203;
int numberOfSectorsPerTrack = 16;
int numberOfHeads = 2;
int microsecondsPerSector = 2500;

/*
**  -----------------
**  Private Variables
**  -----------------
*/

/*
**--------------------------------------------------------------------------
**
**  Public Functions
**
**--------------------------------------------------------------------------
*/

/*--------------------------------------------------------------------------
**  Purpose:        Check if file already exists.
**
**  Parameters:     Name        Description.
**                  filename    file name to check
**
**  Returns:        true if the file exists and false otherwise
**
**------------------------------------------------------------------------*/
bool file_exists(const char *filename)
{
    struct stat buffer;
    return stat(filename, &buffer) == 0;
}


/*--------------------------------------------------------------------------
**  Purpose:        Safely copy string.
**
**  Parameters:     Name        Description.
**                  dst         pointer to destination
**                  src         pointer to source
**                  n           size of destination in bytes
**                  
**                  
**
**  Returns:        Nothing
**
**------------------------------------------------------------------------*/
void safecpy(char *dst, char *src, int n)
{
    // pad destination string with zeroes and enforce zero terminator.
    strncpy(dst, src, n - 1);
    dst[n - 1] = '\0';
}

/*--------------------------------------------------------------------------
**  Purpose:        Serialise an integer to output file.
**
**  Parameters:     Name        Description.
**                  value       integer value to be serialised
**
**  Returns:        true if successful and false otherwise
**
**------------------------------------------------------------------------*/
bool serialize_int(int value)
{
    int rc;
    u8 buf[4];

    buf[0] = (value >> 24) & 0xFF;
    buf[1] = (value >> 16) & 0xFF;
    buf[2] = (value >>  8) & 0xFF;
    buf[3] = (value >>  0) & 0xFF;

    rc = fwrite(buf, 1, 4, ofp);
    if (rc != 4) {
        fprintf(stderr, "Write header error\n");
        return(false);
    }

    return(true);
}

/*--------------------------------------------------------------------------
**  Purpose:        Serialise a string to output file.
**
**  Parameters:     Name        Description.
**                  cp          pointer to string to be serialised
**                  size        size of the string variable in bytes
**
**  Returns:        true if successful and false otherwise
**
**------------------------------------------------------------------------*/
bool serialize_string(char *cp, int size)
{
    int rc;
    static char buf[MAX_SBUF];

    if (size > MAX_SBUF) {
        fprintf(stderr, "Header string too long: %d (MAX=%d)\n", size, MAX_SBUF);
        return(false);
    }

    safecpy(buf, cp, size);

    rc = fwrite(buf, 1, size, ofp);
    if (rc != size) {
        fprintf(stderr, "Write header error\n");
        return(false);
    }

    return(true);
}

/*--------------------------------------------------------------------------
**  Purpose:        Deserialise integer from input file.
**
**  Parameters:     Name        Description.
**                  vp          pointer to integer which will be set to the
**                              deserialised value.
**
**  Returns:        true if successful and false otherwise
**
**------------------------------------------------------------------------*/
bool deserialize_int(int *vp)
{
    int rc;
    u8 buf[4];
    int value = 0;

    rc = fread(buf, 1, 4, ifp);
    if (rc != 4) {
        printf("Read header error\n");
        return(false);
    }

    value  = buf[0] << 24;
    value |= buf[1] << 16;
    value |= buf[2] <<  8;
    value |= buf[3] <<  0;

    *vp = value;

    return(true);
}

/*--------------------------------------------------------------------------
**  Purpose:        Deserialise string from input file.
**
**  Parameters:     Name        Description.
**                  cp          pointer to string varaible which will be set
**                              to the deserialised value.
**                  size        size of the string variable in bytes
**
**  Returns:        true if successful and false otherwise
**
**------------------------------------------------------------------------*/
bool deserialize_string(char *cp, int size)
{
    int rc;
    int value = 0;

    rc = fread(cp, 1, size, ifp);
    if (rc != size) {
        printf("Read header error\n");
        return(false);
    }

    return(true);
}

/*--------------------------------------------------------------------------
**  Purpose:        Write the RK05 image file header.
**
**  Parameters:     Name        Description.
**
**  Returns:        true if successful and false otherwise
**
**------------------------------------------------------------------------*/
bool write_image_file_header(void)
{
    bool rc;

    printf("Writing image header\n");
    rc =       serialize_string(magicNumber, sizeof(magicNumber));
    rc = rc && serialize_string(versionNumber, sizeof(versionNumber));
    rc = rc && serialize_string(imageName, sizeof(imageName));
    rc = rc && serialize_string(imageDescription, sizeof(imageDescription));
    rc = rc && serialize_string(imageDate, sizeof(imageDate));
    rc = rc && serialize_string(controller, sizeof(controller));
    rc = rc && serialize_int(bitRate);                 
    rc = rc && serialize_int(preamble1Length);         
    rc = rc && serialize_int(preamble2Length);         
    rc = rc && serialize_int(dataLength);              
    rc = rc && serialize_int(postambleLength);         
    rc = rc && serialize_int(numberOfCylinders);       
    rc = rc && serialize_int(numberOfSectorsPerTrack); 
    rc = rc && serialize_int(numberOfHeads);           
    rc = rc && serialize_int(microsecondsPerSector);   

    return rc;
}

/*--------------------------------------------------------------------------
**  Purpose:        Read and verify RK05 image file header.
**
**  Parameters:     Name        Description.
**
**  Returns:        true if successful and false otherwise
**
**------------------------------------------------------------------------*/
bool verifyImageFileHeader(void)
{
    bool rc;
    static char tmp[10];

    printf("\n");

    if (!deserialize_string(tmp, sizeof(magicNumber)) || strncmp(tmp, magicNumber, sizeof(magicNumber)) != 0) {
        printf("invalid magic number in header\n");
        return false;
    }

    if (!deserialize_string(tmp, sizeof(versionNumber)) || strncmp(tmp, versionNumber, sizeof(versionNumber)) != 0) {
        printf("unexpected version number in header\n");
        return false;
    }

    rc =       deserialize_string(imageName, sizeof(imageName));
    rc = rc && deserialize_string(imageDescription, sizeof(imageDescription));
    rc = rc && deserialize_string(imageDate, sizeof(imageDate));
    rc = rc && deserialize_string(controller, sizeof(controller));
    rc = rc && deserialize_int(&bitRate);                 
    rc = rc && deserialize_int(&preamble1Length);         
    rc = rc && deserialize_int(&preamble2Length);         
    rc = rc && deserialize_int(&dataLength);              
    rc = rc && deserialize_int(&postambleLength);         
    rc = rc && deserialize_int(&numberOfCylinders);       
    rc = rc && deserialize_int(&numberOfSectorsPerTrack); 
    rc = rc && deserialize_int(&numberOfHeads);           
    rc = rc && deserialize_int(&microsecondsPerSector);

    return rc;
}

/*--------------------------------------------------------------------------
**  Purpose:        Display image file header optionally with full details.
**
**  Parameters:     Name        Description.
**                  detailed    true if full details reqested, false otherwise
**
**  Returns:        Nothing
**
**------------------------------------------------------------------------*/
void displayImageFileHeader(bool detailed)
{
    printf("Image Name = \"%s\"\n", imageName);
    printf("Image Creation Date & Time = %s\n", imageDate);
    printf("Image Description = \"%s\"\n", imageDescription);

    if (detailed) {
        printf("\nEmulation Parameters:\n");
        printf("Controller = %s\n", controller);
        printf("Bit Rate = %d\n", bitRate);
        printf("Preamble1 Length = %d\n", preamble1Length);
        printf("Preamble2 Length = %d\n", preamble2Length);
        printf("Data Length = %d\n", dataLength);
        printf("Postamble Length = %d\n", postambleLength);
        printf("Cylinders = %d\n", numberOfCylinders);
        printf("Sectors per track = %d\n", numberOfSectorsPerTrack);
        printf("Heads = %d\n", numberOfHeads);
        printf("Microseconds per sector = %d\n", microsecondsPerSector);
    }
}

/*--------------------------------------------------------------------------
**  Purpose:        Perform table driven CRC16 calculation of buffer.
**
**  Parameters:     Name        Description.
**                  crc         starting CRC value
**                  bp          pointer to buffer for calculation
**                  size        size of the buffer for calculation
**                  
**
**  Returns:        calculated CRC
**
**------------------------------------------------------------------------*/
u16 crc16buf(u16 crc, const u8 *bp, int size)
{
    static unsigned short crc16Table[256] = {
     0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
     0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
     0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
     0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
     0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
     0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
     0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
     0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
     0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
     0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
     0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
     0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
     0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
     0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
     0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
     0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
     0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
     0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
     0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
     0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
     0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
     0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
     0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
     0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
     0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
     0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
     0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
     0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
     0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
     0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
     0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
     0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040
    };

    while (size--) {
        crc = crc16Table[(crc ^ (*bp++)) & 0xFF] ^ (crc >> 8);
    }

    return crc;
}

/*---------------------------  End Of File  ------------------------------*/
