# RK05 Files
RK05 Files<p>
The rk05 files in this folder can be copied to a microSD card that the emulator will load. When the RUN/LOAD switch is toggled to the RUN position, the emulator loads the first rk05 file that it finds on the microSD card.<p>

Tom Hunter wrote a very nice set of utilities, in the RK05_Ruilities folder, to convert between rk05 files and SIMH files. This allows images to be created using SIMH and then saved and copied to an RK05 emulator.<p>

os8.rk05 is a bootable image of OS8<p>

scratch1.rk05 is a scratch disk, meaning that it has a valid empty file allocation table so OS8 will recognize it as a disk with no files on it.<p>

The emulator reads RK05 files from memory cards inserted into the microSD socket. These files have an rk05 file extension and are a unique format specifically for the RK05 emulator. An RK05 file has a file header followed by a binary image of the disk data. The header describes parameters specific to a particular file format used by the disk controller on each type of computer. Header parameters are values such as: number of sectors per track, number of cylinders, number of heads, bit rate, number of bits per sector, number of bits in the header of a sector, number of bits in the postamble of a sector, number of bit times in each sector, and number of bits in the postamble of each sector.<p>


