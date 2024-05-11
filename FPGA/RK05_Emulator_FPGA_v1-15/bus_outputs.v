//==========================================================================================================
// RK05 Emulator
// Bus Outputs Logic
// File Name: bus_outputs.v
// Functions: 
//   Translate internal signals to the polarity of the RK05 outputs.
//   Output polarity is switchable so any member of the SN7545x family of drivers can be used.
//   Gate all outputs with the internal Selected and Ready signal.
//
//==========================================================================================================

module bus_outputs (
    input wire Selected,               // active high signal indicates that the drive is selected
    input wire File_Ready,             // disk contents have been copied from the microSD to the SDRAM.
    input wire Fault_Latch,            // included for future support. Software will always write zero to Fault_Latch.
    input wire BUS_RWS_RDY_H,          // read/write/seek ready signal
    input wire BUS_ADDRESS_ACCEPTED_H, // cylinder address accepted pulse
    input wire BUS_ADDRESS_INVALID_H,  // address invalid pulse
    input wire BUS_SEEK_INCOMPLETE_H,  // seek incomplete signal, not used, always inactive
    input wire Write_Protect,          // CPU register that indicates the drive write protect status.
    input wire BUS_RD_DATA_H,          // Read data pulses
    input wire BUS_RD_CLK_H,           // Read clock pulses
    input wire BUS_RD_GATE_L,          // Read gate, when active enables read circuitry
    input wire [3:0] Sector_Address,   // counter that specifies which sector is present "under the heads"
    input wire bus_sector_pulse,       // active-high 2 usec sector pulse
    input wire bus_index_pulse,        // active-high 2 usec index pulse
    input wire cpu_dc_low,		       // DC low signal driven by a CPU register
    input wire interface_test_mode,
    input wire [7:0] preamble1_length,
    input wire [7:0] preamble2_length,
    input wire [15:0] data_length,

    output wire BUS_FILE_READY_L,       // File ready (drive ready). For the emulator: c. microSD inserted, d. RUN/LOAD at RUN position, e. data copied from microSD to SDRAM.
    output wire BUS_RWS_RDY_L,          // Read, Write or Seek ready/on cylinder. Indicates that the drive is in the File Ready condition and is not performing a Seek operation.
    output wire BUS_ADDRESS_ACCEPTED_L, // Address accepted. The drive has accepted a Seek command with a valid address.
    output wire BUS_ADDRESS_INVALID_L,  // Address invalid. The drive has received a nonexecutable Seek command with a cylinder address greater than 202.
    output wire BUS_SEEK_INCOMPLETE_L,  // Seek incomplete. Connect to a 75452 and to the FPGA but not used (never active).
    output wire BUS_WT_PROT_STATUS_L,   // Write protect status.
    output wire BUS_WT_CHK_L,           // Write check. Connect to a 75452 and to the FPGA but not used (never active).
    output wire BUS_RD_DATA_L,          // Read data pulses, 160 ns pulse
    output wire BUS_RD_CLK_L,           // Read clock pulses, 160 ns pulse
    output wire [3:0] BUS_SEC_CNTR_L,   // 4-bit Sector counter 
    output wire BUS_SEC_PLS_L,          // A 2 us negative pulse each time a sector slot passes the transducer.
    output wire BUS_INDX_PLS_L,         // A 2 us negative pulse for each revolution of the disk.
    output wire BUS_DC_LO_L,            // DC low indicator, not used for the emulator but connect it to a 75452.
    output wire BUS_RK05_HIGH_DENSITY_L, // All RK05's are high density.
    output wire Selected_Ready          // disk contents have been copied from the microSD to the SDRAM & drive selected & ~fault latch
);

//============================ Start of Code =========================================


assign Selected_Ready = Selected & File_Ready & ~Fault_Latch;
//assign Selected_Ready = 1'b1; // ALWAYS Selected AND Ready FOR DEBUGGING *************************

// Can keep or remove the inversion in front of the following equations, depending on
// the specific type of SN7545x driver that is installed on the circuit board.
//
// G180 schematic, lower-right corner
assign BUS_RD_DATA_L =           interface_test_mode ? ~preamble2_length[6] : ~(BUS_RD_DATA_H & ~BUS_RD_GATE_L & Selected_Ready);
assign BUS_RD_CLK_L =            interface_test_mode ? ~preamble2_length[7] : ~(BUS_RD_CLK_H  & ~BUS_RD_GATE_L & Selected_Ready);
assign BUS_RK05_HIGH_DENSITY_L = interface_test_mode ? ~preamble1_length[7] : ~Selected_Ready;

//M7680 schematic, page 1 of 2, right side
assign BUS_SEC_PLS_L =           interface_test_mode ? ~preamble1_length[4] : ~(bus_sector_pulse & Selected_Ready);
assign BUS_INDX_PLS_L =          interface_test_mode ? ~preamble1_length[5] : ~(bus_index_pulse  & Selected_Ready);
assign BUS_SEEK_INCOMPLETE_L =   interface_test_mode ? ~preamble2_length[3] : ~(1'b0);
assign BUS_RWS_RDY_L =           interface_test_mode ? ~preamble2_length[0] : ~(BUS_RWS_RDY_H    & Selected_Ready);
assign BUS_SEC_CNTR_L[3] =       interface_test_mode ? ~preamble1_length[3] : ~(Sector_Address[3] & Selected_Ready);
assign BUS_SEC_CNTR_L[2] =       interface_test_mode ? ~preamble1_length[2] : ~(Sector_Address[2] & Selected_Ready);
assign BUS_SEC_CNTR_L[1] =       interface_test_mode ? ~preamble1_length[1] : ~(Sector_Address[1] & Selected_Ready);
assign BUS_SEC_CNTR_L[0] =       interface_test_mode ? ~preamble1_length[0] : ~(Sector_Address[0] & Selected_Ready);

//M7680 schematic, page 2 of 2, right side
assign BUS_ADDRESS_INVALID_L =   interface_test_mode ? ~preamble2_length[2] : ~(BUS_ADDRESS_INVALID_H  & Selected_Ready);
assign BUS_ADDRESS_ACCEPTED_L =  interface_test_mode ? ~preamble2_length[1] : ~(BUS_ADDRESS_ACCEPTED_H & Selected_Ready);

//M7701 schematic, page 3 of 3, right side
assign BUS_FILE_READY_L =        interface_test_mode ? ~data_length[11] : ~(Selected_Ready);
assign BUS_WT_CHK_L =            interface_test_mode ? ~preamble2_length[5] : ~(Fault_Latch & Selected);
assign BUS_WT_PROT_STATUS_L =    interface_test_mode ? ~preamble2_length[4] : ~(Write_Protect & Selected);
assign BUS_DC_LO_L =             interface_test_mode ? ~preamble1_length[6] : ~cpu_dc_low;
//assign BUS_DC_LO_L =             ~(1'b0);
//assign BUS_AC_LO_L =             ~(1'b0);

endmodule // End of Module bus_outputs
