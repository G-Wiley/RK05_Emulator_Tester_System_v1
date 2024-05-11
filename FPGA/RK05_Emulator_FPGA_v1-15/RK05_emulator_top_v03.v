//==========================================================================================================
// RK05 Emulator
// Top Level definition
// File Name: RK05_emulator_top_v03.v
//==========================================================================================================

`include "bus_disk_read.v"
`include "bus_disk_write.v"
`include "bus_outputs.v"
`include "clock_and_reset.v"
`include "drive_select.v"
`include "sdram_controller.v"
`include "sector_and_index.v"
`include "seek_to_cylinder.v"
`include "spi_interface.v"
`include "timing_gen.v"

//================================= TOP LEVEL INPUT-OUTPUT DEFINITIONS =====================================
module RK05_emulator_top (

// BUS Connector Inputs, 20 signals
    input wire BUS_RK11D_L,         // RK11 mode for Drive Select BUS_RK11D_L
    input wire [3:0] BUS_SEL_DR_L,  // Drive Select signals
    input wire [7:0] BUS_CYL_ADD_L, // Cylinder Address
    input wire BUS_STROBE_L,        // Strobe to enable movement, gates the Cylinder Address or Restore line
    input wire BUS_HEAD_SELECT_L,   // Head Select
    input wire BUS_WT_PROTECT_L,    // Write Protect
    input wire BUS_WT_DATA_CLK_L,   // Composite write data and write clock
    input wire BUS_WT_GATE_L,       // Write gate, when active enables write circuitry
    input wire BUS_RESTORE_L,       // Restore, moves heads to cylinder 0
    input wire BUS_RD_GATE_L,       // Read gate, when active enables read circuitry

// BUS Connector Outputs, 17 signals
    output wire BUS_FILE_READY_L,       // data copied from microSD to SDRAM, drive is ready
    output wire BUS_RWS_RDY_L,          // Read, Write or Seek ready and on-cylinder
    output wire BUS_ADDRESS_ACCEPTED_L, // Address accepted. 5 μs pulse indicates that the drive accepted a Seek command
    output wire BUS_ADDRESS_INVALID_L,  // Address invalid. 5 μs pulse indicates that the drive received a nonexecutable Seek command 
    output wire BUS_SEEK_INCOMPLETE_L,  // Seek Incomplete, indicates some malfunction, connected but not currently used
    output wire BUS_WT_PROT_STATUS_L,   // Write Protect status
    output wire BUS_WT_CHK_L,           // Write Check, indicates a write malfunction, connected but not currently used
    output wire BUS_RD_DATA_L,          // Read data pulses, 160 ns pulse
    output wire BUS_RD_CLK_L,           // Read clock pulses, 160 ns pulse
    output wire [3:0] BUS_SEC_CNTR_L,   // Sector Counter
    output wire BUS_SEC_PLS_L,          // 2 us negative pulse each time a sector slot passes the transducer
    output wire BUS_INDX_PLS_L,         // 2 us negative pulse for each revolution of the disk, 600 μs after the sector pulse
    output wire BUS_DC_LO_L,            // DC low indicator, not used for the emulator, connected but not currently used
    output wire BUS_RK05_HIGH_DENSITY_L, // All RK05’s are high density

// SDRAM I/O. 39 signals. 16 - DQ, 13 - Address: A0-A12, 2 - Addr Select: BS0 BS1, 8 - SDRAM Control: WE# CAS# RAS# CS# CLK CKE DQML DQMH
  //input [15:0] SDRAM_DQ_in,        // input from DQ signal receivers
  //output [15:0] SDRAM_DQ_output,   // outputs to DQ signal drivers
  //output SDRAM_DQ_enable,          // DQ output enable, active high
    output wire [12:0] SDRAM_Address,// SDRAM Address
    output wire SDRAM_BS0,	         // SDRAM Bank Select 0
    output wire SDRAM_BS1,	         // SDRAM Bank Select 1
    output wire SDRAM_WE_n,	         // SDRAM Write
    output wire SDRAM_CAS_n,         // SDRAM Column Address Select
    output wire SDRAM_RAS_n,         // SDRAM Row Address Select
    output wire SDRAM_CS_n,	         // SDRAM Chip Select
    output wire SDRAM_CLK,	         // SDRAM Clock
    output wire SDRAM_CKE,	         // SDRAM Clock Enable
    output wire SDRAM_DQML,	         // SDRAM DQ Mask for Lower Byte
    output wire SDRAM_DQMH,	         // SDRAM DQ Mask for Upper (High) Byte
    inout wire [15:0] SDRAM_DQ,      //SDRAM multiplexed Data Input & Output

// ESP32 CPU SPI Port 6 signals: MISO MOSI CLK CS 2-bit-register-select
    output wire CPU_SPI_MISO,       // SPI Controller Input Peripheral Output
    input wire CPU_SPI_MOSI,        // SPI Controller Output Peripheral Input
    input wire CPU_SPI_CLK,         // SPI Controller Clock
    input wire CPU_SPI_CS_n,        // SPI Controller Chip Select, active low
  
// Front Panel from FPGA, 4 signals. WT_PROT_indicator WT_indicator RD_indicator ON_CYL_indicator
    output wire FPANEL_WT_PROT_indicator, // Write Protect indicator
    output wire FPANEL_WT_indicator,      // Write indicator
    output wire FPANEL_RD_indicator,      // Read indicator
    output wire FPANEL_ON_CYL_indicator,  // On Cylinder indicator
  
// Clock and Reset external pins: pin_clock pin_reset
    input wire clock,
    input wire pin_reset_n,
    
// Tester Outputs, new in Emulator v1 hardware
    output wire TESTER_OUTPUT_1_L, // this is pin 87
    output wire TESTER_OUTPUT_2_L, // this is pin 44
    output wire TESTER_OUTPUT_3_L, // this is pin 45

    output wire Servo_Pulse_FPGA_pin,   // this is pin 73
    output wire SPARE_PIO1_24,      // this is pin 74
    output wire SELECTED_RDY_LED_N, // this is pin 75
    output wire CMD_INTERRUPT      // this is pin 76
);

//============================ Internal Connections ==================================

wire [7:0] MAJOR_VERSION;
assign MAJOR_VERSION = 1;
wire [7:0] MINOR_VERSION;
assign MINOR_VERSION = 15;

wire reset;

wire [7:0] preamble1_length;
wire [7:0] preamble2_length;
wire [15:0] data_length;
wire [7:0] postamble_length;
wire [4:0] number_of_sectors;
wire [7:0] bitclockdivider_clockphase;
wire [7:0] bitclockdivider_dataphase;
wire [7:0] bitpulse_width;
wire [15:0] microseconds_per_sector;
wire cpu_dc_low;

wire clkenbl_sector;
wire clkenbl_index;
wire bus_sector_pulse;
wire bus_index_pulse;

wire File_Ready;
wire Write_Protect;
wire Fault_Latch;
wire [2:0] Drive_Address;

wire Selected_Ready;
wire [7:0] Cylinder_Address;
wire Head_Select;
wire [3:0] Sector_Address;
wire oncylinder_indicator;
wire write_indicator;
wire read_indicator;

wire BUS_RD_DATA_H;
wire BUS_RD_CLK_H;

wire load_address_spi;
wire load_address_busread;
wire load_address_buswrite;
wire dram_read_enbl_spi;
wire dram_read_enbl_busread;
wire dram_write_enbl_spi;
wire dram_write_enbl_buswrite;
wire dram_readack;
wire dram_writeack;

wire [7:0] spi_serpar_reg;
wire [15:0] dram_readdata;
wire [15:0] dram_writedata_spi;
wire [15:0] dram_writedata_buswrite;

wire [15:0] SDRAM_DQ_in;
wire [15:0] SDRAM_DQ_output;
wire SDRAM_DQ_enable;

wire clkenbl_read_bit;
wire clkenbl_read_data;
wire clock_pulse;
wire data_pulse;
wire clkenbl_1usec;

wire [7:0] bdw_test;
wire interface_test_mode;

wire strobe_selected_ready;
wire read_selected_ready;
wire write_selected_ready;

wire Servo_Pulse_FPGA;

//reg [7:0] test_counter;

//wire [2:0] bdw_state_output; // for debugging for visibility of the bus write state

//============================ MISC TOP LEVEL LOGIC TO DRIVE THE INDICATORS ==================================

assign FPANEL_WT_PROT_indicator = interface_test_mode ? ~data_length[3] : ~Write_Protect;
assign FPANEL_WT_indicator =      interface_test_mode ? ~data_length[1] : ~write_indicator;
assign FPANEL_RD_indicator =      interface_test_mode ? ~data_length[0] : ~read_indicator;
assign FPANEL_ON_CYL_indicator =  interface_test_mode ? ~data_length[5] : ~(oncylinder_indicator & File_Ready);
//assign FPANEL_WT_indicator = ~(~BUS_WT_GATE_L & Selected_Ready);
//assign FPANEL_RD_indicator = ~(~BUS_RD_GATE_L & Selected_Ready);

assign SPARE_PIO1_24 = Selected;
//assign SPARE_PIO1_24 = 1'b0;
assign SELECTED_RDY_LED_N = ~Selected_Ready;
assign TESTER_OUTPUT_1_L = interface_test_mode ? ~data_length[8]  : ~1'b0; // this is pin 87
assign TESTER_OUTPUT_2_L = interface_test_mode ? ~data_length[9]  : ~1'b0; // this is pin 44
assign TESTER_OUTPUT_3_L = interface_test_mode ? ~data_length[10] : ~1'b0; // this is pin 45

assign Servo_Pulse_FPGA_pin = Servo_Pulse_FPGA;
//assign Servo_Pulse_FPGA_pin = ~Selected_Ready;

//assign TESTER_OUTPUT_1 = test_counter[0]; // this is pin 87
//assign TESTER_OUTPUT_2 = test_counter[1]; // this is pin 44
//assign TESTER_OUTPUT_3 = test_counter[2]; // this is pin 45
//assign Servo_Pulse_FPGA_pin = test_counter[3];   // this is pin 73
//assign SPARE_PIO1_24 = test_counter[4];      // this is pin 74
//assign SELECTED_RDY_LED_N = test_counter[5]; // this is pin 75
//assign CMD_INTERRUPTX = test_counter[6];  // this is pin 76

//always @ (posedge clock)
//begin : PINTESTLOOP // block name
    //if(reset==1'b1) begin
        //test_counter <= 8'd0;
    //end
    //else begin
        //test_counter <= test_counter + 1;
    //end
//end

//============================ SDRAM Bidirectional I/O pins ==================================

SB_IO DQ00
(
.PACKAGE_PIN (SDRAM_DQ[0]), // User's Pin signal name
.OUTPUT_ENABLE (SDRAM_DQ_enable), // Output Pin Tristate/Enable control
.D_OUT_0 (SDRAM_DQ_output[0]), // Data 0 - out to Pin
.D_IN_0 (SDRAM_DQ_in[0]), // Data 0 - Pin input
// n.c. pins we don't use
.LATCH_INPUT_VALUE (), .CLOCK_ENABLE (), .INPUT_CLK (), .OUTPUT_CLK (), .D_OUT_1 (), .D_IN_1 () 
);
defparam DQ00.PIN_TYPE = 6'b101001; // non-latched bi-directional I/O buffer
defparam DQ00.PULLUP = 1'b0;
// By default, the IO will have NO pull up.
// This parameter is used only on bank 0, 1, and 2. Ignored when it is placed at bank 3
defparam DQ00.NEG_TRIGGER = 1'b0;
// Specify the polarity of all FFs in the IO to be falling edge when NEG_TRIGGER = 1.
// Default is rising edge.
//defparam DQ00.IO_STANDARD = "LVCMOS";
// Other IO standards are supported in bank 3 only: SB_SSTL2_CLASS_2, SB_SSTL2_CLASS_1,
// SB_SSTL18_FULL, SB_SSTL18_HALF, SB_MDDR10,SB_MDDR8, SB_MDDR4, SB_MDDR2 etc.

SB_IO DQ01
(
.PACKAGE_PIN (SDRAM_DQ[1]),
.OUTPUT_ENABLE (SDRAM_DQ_enable),
.D_OUT_0 (SDRAM_DQ_output[1]),
.D_IN_0 (SDRAM_DQ_in[1]),
.LATCH_INPUT_VALUE (), .CLOCK_ENABLE (), .INPUT_CLK (), .OUTPUT_CLK (), .D_OUT_1 (), .D_IN_1 () 
);
defparam DQ01.PIN_TYPE = 6'b101001; // non-latched bi-directional I/O buffer
defparam DQ01.PULLUP = 1'b0;
defparam DQ01.NEG_TRIGGER = 1'b0;
//defparam DQ01.IO_STANDARD = "LVCMOS";

SB_IO DQ02
(
.PACKAGE_PIN (SDRAM_DQ[2]),
.OUTPUT_ENABLE (SDRAM_DQ_enable),
.D_OUT_0 (SDRAM_DQ_output[2]),
.D_IN_0 (SDRAM_DQ_in[2]),
// n.c. pins we don't use
.LATCH_INPUT_VALUE (), .CLOCK_ENABLE (), .INPUT_CLK (), .OUTPUT_CLK (), .D_OUT_1 (), .D_IN_1 () 
);
defparam DQ02.PIN_TYPE = 6'b101001; // non-latched bi-directional I/O buffer
defparam DQ02.PULLUP = 1'b0;
defparam DQ02.NEG_TRIGGER = 1'b0;
//defparam DQ02.IO_STANDARD = "LVCMOS";

SB_IO DQ03
(
.PACKAGE_PIN (SDRAM_DQ[3]),
.OUTPUT_ENABLE (SDRAM_DQ_enable),
.D_OUT_0 (SDRAM_DQ_output[3]),
.D_IN_0 (SDRAM_DQ_in[3]),
// n.c. pins we don't use
.LATCH_INPUT_VALUE (), .CLOCK_ENABLE (), .INPUT_CLK (), .OUTPUT_CLK (), .D_OUT_1 (), .D_IN_1 () 
);
defparam DQ03.PIN_TYPE = 6'b101001; // non-latched bi-directional I/O buffer
defparam DQ03.PULLUP = 1'b0;
defparam DQ03.NEG_TRIGGER = 1'b0;
//defparam DQ03.IO_STANDARD = "LVCMOS";

SB_IO DQ04
(
.PACKAGE_PIN (SDRAM_DQ[4]),
.OUTPUT_ENABLE (SDRAM_DQ_enable),
.D_OUT_0 (SDRAM_DQ_output[4]),
.D_IN_0 (SDRAM_DQ_in[4]),
// n.c. pins we don't use
.LATCH_INPUT_VALUE (), .CLOCK_ENABLE (), .INPUT_CLK (), .OUTPUT_CLK (), .D_OUT_1 (), .D_IN_1 () 
);
defparam DQ04.PIN_TYPE = 6'b101001; // non-latched bi-directional I/O buffer
defparam DQ04.PULLUP = 1'b0;
defparam DQ04.NEG_TRIGGER = 1'b0;
//defparam DQ04.IO_STANDARD = "LVCMOS";

SB_IO DQ05
(
.PACKAGE_PIN (SDRAM_DQ[5]),
.OUTPUT_ENABLE (SDRAM_DQ_enable),
.D_OUT_0 (SDRAM_DQ_output[5]),
.D_IN_0 (SDRAM_DQ_in[5]),
// n.c. pins we don't use
.LATCH_INPUT_VALUE (), .CLOCK_ENABLE (), .INPUT_CLK (), .OUTPUT_CLK (), .D_OUT_1 (), .D_IN_1 () 
);
defparam DQ05.PIN_TYPE = 6'b101001; // non-latched bi-directional I/O buffer
defparam DQ05.PULLUP = 1'b0;
defparam DQ05.NEG_TRIGGER = 1'b0;
//defparam DQ05.IO_STANDARD = "LVCMOS";

SB_IO DQ06
(
.PACKAGE_PIN (SDRAM_DQ[6]),
.OUTPUT_ENABLE (SDRAM_DQ_enable),
.D_OUT_0 (SDRAM_DQ_output[6]),
.D_IN_0 (SDRAM_DQ_in[6]),
// n.c. pins we don't use
.LATCH_INPUT_VALUE (), .CLOCK_ENABLE (), .INPUT_CLK (), .OUTPUT_CLK (), .D_OUT_1 (), .D_IN_1 () 
);
defparam DQ06.PIN_TYPE = 6'b101001; // non-latched bi-directional I/O buffer
defparam DQ06.PULLUP = 1'b0;
defparam DQ06.NEG_TRIGGER = 1'b0;
//defparam DQ06.IO_STANDARD = "LVCMOS";

SB_IO DQ07
(
.PACKAGE_PIN (SDRAM_DQ[7]),
.OUTPUT_ENABLE (SDRAM_DQ_enable),
.D_OUT_0 (SDRAM_DQ_output[7]),
.D_IN_0 (SDRAM_DQ_in[7]),
// n.c. pins we don't use
.LATCH_INPUT_VALUE (), .CLOCK_ENABLE (), .INPUT_CLK (), .OUTPUT_CLK (), .D_OUT_1 (), .D_IN_1 () 
);
defparam DQ07.PIN_TYPE = 6'b101001; // non-latched bi-directional I/O buffer
defparam DQ07.PULLUP = 1'b0;
defparam DQ07.NEG_TRIGGER = 1'b0;
//defparam DQ07.IO_STANDARD = "LVCMOS";

SB_IO DQ08
(
.PACKAGE_PIN (SDRAM_DQ[8]),
.OUTPUT_ENABLE (SDRAM_DQ_enable),
.D_OUT_0 (SDRAM_DQ_output[8]),
.D_IN_0 (SDRAM_DQ_in[8]),
// n.c. pins we don't use
.LATCH_INPUT_VALUE (), .CLOCK_ENABLE (), .INPUT_CLK (), .OUTPUT_CLK (), .D_OUT_1 (), .D_IN_1 () 
);
defparam DQ08.PIN_TYPE = 6'b101001; // non-latched bi-directional I/O buffer
defparam DQ08.PULLUP = 1'b0;
defparam DQ08.NEG_TRIGGER = 1'b0;
//defparam DQ08.IO_STANDARD = "LVCMOS";

SB_IO DQ09
(
.PACKAGE_PIN (SDRAM_DQ[9]),
.OUTPUT_ENABLE (SDRAM_DQ_enable),
.D_OUT_0 (SDRAM_DQ_output[9]),
.D_IN_0 (SDRAM_DQ_in[9]),
// n.c. pins we don't use
.LATCH_INPUT_VALUE (), .CLOCK_ENABLE (), .INPUT_CLK (), .OUTPUT_CLK (), .D_OUT_1 (), .D_IN_1 () 
);
defparam DQ09.PIN_TYPE = 6'b101001; // non-latched bi-directional I/O buffer
defparam DQ09.PULLUP = 1'b0;
defparam DQ09.NEG_TRIGGER = 1'b0;
//defparam DQ09.IO_STANDARD = "LVCMOS";

SB_IO DQ10
(
.PACKAGE_PIN (SDRAM_DQ[10]),
.OUTPUT_ENABLE (SDRAM_DQ_enable),
.D_OUT_0 (SDRAM_DQ_output[10]),
.D_IN_0 (SDRAM_DQ_in[10]),
// n.c. pins we don't use
.LATCH_INPUT_VALUE (), .CLOCK_ENABLE (), .INPUT_CLK (), .OUTPUT_CLK (), .D_OUT_1 (), .D_IN_1 () 
);
defparam DQ10.PIN_TYPE = 6'b101001; // non-latched bi-directional I/O buffer
defparam DQ10.PULLUP = 1'b0;
defparam DQ10.NEG_TRIGGER = 1'b0;
//defparam DQ10.IO_STANDARD = "LVCMOS";

SB_IO DQ11
(
.PACKAGE_PIN (SDRAM_DQ[11]),
.OUTPUT_ENABLE (SDRAM_DQ_enable),
.D_OUT_0 (SDRAM_DQ_output[11]),
.D_IN_0 (SDRAM_DQ_in[11]),
// n.c. pins we don't use
.LATCH_INPUT_VALUE (), .CLOCK_ENABLE (), .INPUT_CLK (), .OUTPUT_CLK (), .D_OUT_1 (), .D_IN_1 () 
);
defparam DQ11.PIN_TYPE = 6'b101001; // non-latched bi-directional I/O buffer
defparam DQ11.PULLUP = 1'b0;
defparam DQ11.NEG_TRIGGER = 1'b0;
//defparam DQ11.IO_STANDARD = "LVCMOS";

SB_IO DQ12
(
.PACKAGE_PIN (SDRAM_DQ[12]),
.OUTPUT_ENABLE (SDRAM_DQ_enable),
.D_OUT_0 (SDRAM_DQ_output[12]),
.D_IN_0 (SDRAM_DQ_in[12]),
// n.c. pins we don't use
.LATCH_INPUT_VALUE (), .CLOCK_ENABLE (), .INPUT_CLK (), .OUTPUT_CLK (), .D_OUT_1 (), .D_IN_1 () 
);
defparam DQ12.PIN_TYPE = 6'b101001; // non-latched bi-directional I/O buffer
defparam DQ12.PULLUP = 1'b0;
defparam DQ12.NEG_TRIGGER = 1'b0;
//defparam DQ12.IO_STANDARD = "LVCMOS";

SB_IO DQ13
(
.PACKAGE_PIN (SDRAM_DQ[13]),
.OUTPUT_ENABLE (SDRAM_DQ_enable),
.D_OUT_0 (SDRAM_DQ_output[13]),
.D_IN_0 (SDRAM_DQ_in[13]),
// n.c. pins we don't use
.LATCH_INPUT_VALUE (), .CLOCK_ENABLE (), .INPUT_CLK (), .OUTPUT_CLK (), .D_OUT_1 (), .D_IN_1 () 
);
defparam DQ13.PIN_TYPE = 6'b101001; // non-latched bi-directional I/O buffer
defparam DQ13.PULLUP = 1'b0;
defparam DQ13.NEG_TRIGGER = 1'b0;
//defparam DQ13.IO_STANDARD = "LVCMOS";

SB_IO DQ14
(
.PACKAGE_PIN (SDRAM_DQ[14]),
.OUTPUT_ENABLE (SDRAM_DQ_enable),
.D_OUT_0 (SDRAM_DQ_output[14]),
.D_IN_0 (SDRAM_DQ_in[14]),
// n.c. pins we don't use
.LATCH_INPUT_VALUE (), .CLOCK_ENABLE (), .INPUT_CLK (), .OUTPUT_CLK (), .D_OUT_1 (), .D_IN_1 () 
);
defparam DQ14.PIN_TYPE = 6'b101001; // non-latched bi-directional I/O buffer
defparam DQ14.PULLUP = 1'b0;
defparam DQ14.NEG_TRIGGER = 1'b0;
//defparam DQ14.IO_STANDARD = "LVCMOS";

SB_IO DQ15
(
.PACKAGE_PIN (SDRAM_DQ[15]),
.OUTPUT_ENABLE (SDRAM_DQ_enable),
.D_OUT_0 (SDRAM_DQ_output[15]),
.D_IN_0 (SDRAM_DQ_in[15]),
// n.c. pins we don't use
.LATCH_INPUT_VALUE (), .CLOCK_ENABLE (), .INPUT_CLK (), .OUTPUT_CLK (), .D_OUT_1 (), .D_IN_1 () 
);
defparam DQ15.PIN_TYPE = 6'b101001; // non-latched bi-directional I/O buffer
defparam DQ15.PULLUP = 1'b0;
defparam DQ15.NEG_TRIGGER = 1'b0;
//defparam DQ15.IO_STANDARD = "LVCMOS";


//================================= MODULES =====================================

//verified clean 4/2/2023
// ======== Module ======== clock_and_reset =====
clock_and_reset i_clock_and_reset (
    // Inputs
    .clock (clock),
    .pin_reset_n (pin_reset_n),

    // Outputs
    .reset (reset)
);

// improved 4/2/2023
// ======== Module ======== bus_disk_read =====
bus_disk_read i_bus_disk_read (
    // Inputs
    .clock (clock),
    .reset (reset),
    .BUS_RD_GATE_L (BUS_RD_GATE_L),
    .clkenbl_read_bit (clkenbl_read_bit),
    .clkenbl_read_data (clkenbl_read_data),
    .clock_pulse (clock_pulse),
    .data_pulse (data_pulse),
    .dram_readdata (dram_readdata),
    .Selected_Ready (Selected_Ready),
    .Cylinder_Address (Cylinder_Address),
    .preamble2_length (preamble2_length),
    .data_length (data_length),
    .clkenbl_sector (clkenbl_sector),

    // Outputs
    .dram_read_enbl_busread (dram_read_enbl_busread),
    .BUS_RD_DATA_H (BUS_RD_DATA_H),
    .BUS_RD_CLK_H (BUS_RD_CLK_H),
    .load_address_busread (load_address_busread),
    .read_indicator (read_indicator),
    .read_selected_ready (read_selected_ready)
);

// new 4/4/2023
// ======== Module ======== bus_disk_write =====
bus_disk_write i_bus_disk_write (
    // Inputs
    .clock (clock),
    .reset (reset),
    .BUS_WT_GATE_L (BUS_WT_GATE_L),
    .BUS_WT_DATA_CLK_L (BUS_WT_DATA_CLK_L),
    .Selected_Ready (Selected_Ready),
    .data_length (data_length),
    .clkenbl_sector (clkenbl_sector),

    // Outputs
    .dram_write_enbl_buswrite (dram_write_enbl_buswrite),
    .dram_writedata_buswrite (dram_writedata_buswrite),
    .load_address_buswrite (load_address_buswrite),
    .write_indicator (write_indicator),
    .bdw_debug (bdw_test),
    .write_selected_ready (write_selected_ready)
    //.bdw_state_output (bdw_state_output)
);

// verified clean 4/2/2023
// ======== Module ======== bus_outputs =====
bus_outputs i_bus_outputs (
    // Inputs
    .Selected (Selected),
    .File_Ready (File_Ready),
    .Fault_Latch (Fault_Latch),
    .BUS_RWS_RDY_H (BUS_RWS_RDY_H),
    .BUS_ADDRESS_ACCEPTED_H (BUS_ADDRESS_ACCEPTED_H),
    .BUS_ADDRESS_INVALID_H (BUS_ADDRESS_INVALID_H),
    //.BUS_SEEK_INCOMPLETE_H (BUS_SEEK_INCOMPLETE_H),
    .Write_Protect (Write_Protect),
    .BUS_RD_DATA_H (BUS_RD_DATA_H),
    .BUS_RD_CLK_H (BUS_RD_CLK_H),
    .BUS_RD_GATE_L (BUS_RD_GATE_L),
    .Sector_Address (Sector_Address),
    .bus_sector_pulse (bus_sector_pulse),
    .bus_index_pulse (bus_index_pulse),
    .cpu_dc_low (cpu_dc_low),
    //.cpu_dc_low (File_Ready), // for debugging to have visibility of File_Ready
    //.cpu_dc_low (~SDRAM_DQ_output[0]),  // for debugging SDRAM_DQ[0]
    .interface_test_mode (interface_test_mode),
    .preamble1_length (preamble1_length),
    .preamble2_length (preamble2_length),
    .data_length (data_length),

    // Outputs
    .BUS_FILE_READY_L (BUS_FILE_READY_L),
    .BUS_RWS_RDY_L (BUS_RWS_RDY_L),
    .BUS_ADDRESS_ACCEPTED_L (BUS_ADDRESS_ACCEPTED_L),
    .BUS_ADDRESS_INVALID_L (BUS_ADDRESS_INVALID_L),
    .BUS_SEEK_INCOMPLETE_L (BUS_SEEK_INCOMPLETE_L),
    .BUS_WT_PROT_STATUS_L (BUS_WT_PROT_STATUS_L),
    .BUS_WT_CHK_L (BUS_WT_CHK_L),
    .BUS_RD_DATA_L (BUS_RD_DATA_L),
    .BUS_RD_CLK_L (BUS_RD_CLK_L),
    .BUS_SEC_CNTR_L (BUS_SEC_CNTR_L),
    .BUS_SEC_PLS_L (BUS_SEC_PLS_L),
    .BUS_INDX_PLS_L (BUS_INDX_PLS_L),
    //.BUS_AC_LO_L (BUS_AC_LO_L),
    .BUS_DC_LO_L (BUS_DC_LO_L),
    .BUS_RK05_HIGH_DENSITY_L (BUS_RK05_HIGH_DENSITY_L),
    .Selected_Ready (Selected_Ready)
);

// verified clean 4/2/2023
// ======== Module ======== drive_select =====
drive_select i_drive_select (
    // Inputs
    .BUS_RK11D_L (BUS_RK11D_L),
    .BUS_SEL_DR_L (BUS_SEL_DR_L),
    .Drive_Address (Drive_Address),
    .RK05F_Mode (1'b0),

    // Outputs
    .Selected (Selected)
);

// new 4/7/2023
// ======== Module ======== sdram_controller =====
sdram_controller i_sdram_controller (
    // Inputs
    .clock (clock),
    .reset (reset),
    .load_address_spi (load_address_spi),
    .load_address_busread (load_address_busread),
    .load_address_buswrite (load_address_buswrite),
    .dram_read_enbl_spi (dram_read_enbl_spi),
    .dram_read_enbl_busread (dram_read_enbl_busread),
    .dram_write_enbl_spi (dram_write_enbl_spi),
    .dram_write_enbl_buswrite (dram_write_enbl_buswrite),
    .dram_writedata_spi (dram_writedata_spi),
    .dram_writedata_buswrite (dram_writedata_buswrite),
    .Write_Protect (Write_Protect),
    .spi_serpar_reg (spi_serpar_reg),
    .Sector_Address (Sector_Address),
    .Cylinder_Address (Cylinder_Address),
    .Head_Select (Head_Select),

    .SDRAM_DQ_in (SDRAM_DQ_in),

    // Outputs
    .dram_readdata (dram_readdata),
    .dram_readack (dram_readack),
    .dram_writeack (dram_writeack),

    .SDRAM_DQ_output (SDRAM_DQ_output),
    .SDRAM_DQ_enable (SDRAM_DQ_enable),
    .SDRAM_Address (SDRAM_Address),
    .SDRAM_BS0 (SDRAM_BS0),
    .SDRAM_BS1 (SDRAM_BS1),
    .SDRAM_WE_n (SDRAM_WE_n),
    .SDRAM_CAS_n (SDRAM_CAS_n),
    .SDRAM_RAS_n (SDRAM_RAS_n),
    .SDRAM_CS_n (SDRAM_CS_n),
    .SDRAM_CLK (SDRAM_CLK),
    .SDRAM_CKE (SDRAM_CKE),
    .SDRAM_DQML (SDRAM_DQML),
    .SDRAM_DQMH (SDRAM_DQMH)
);


// improved 7/29/2023
// ======== Module ======== sector_and_index =====
sector_and_index i_sector_and_index (
    // Inputs
    .clock (clock),
    .reset (reset),
    .clkenbl_1usec (clkenbl_1usec),
    .number_of_sectors (number_of_sectors),
    .microseconds_per_sector (microseconds_per_sector),

    // Outputs
    .clkenbl_sector (clkenbl_sector),
    .clkenbl_index (clkenbl_index),
    .bus_sector_pulse (bus_sector_pulse),
    .bus_index_pulse (bus_index_pulse),
    .Sector_Address (Sector_Address)
);

// ======== Module ======== seek_to_cylinder =====
seek_to_cylinder i_seek_to_cylinder (
    // Inputs
    .clock (clock),
    .reset (reset),
    .Selected_Ready (Selected_Ready),
    .BUS_STROBE_L (BUS_STROBE_L),
    .BUS_CYL_ADD_L (BUS_CYL_ADD_L),
    .BUS_RESTORE_L (BUS_RESTORE_L),
    .BUS_HEAD_SELECT_L (BUS_HEAD_SELECT_L),
    .clkenbl_index (clkenbl_index),

    // Outputs
    .Cylinder_Address (Cylinder_Address),
    .Head_Select (Head_Select),
    .BUS_ADDRESS_ACCEPTED_H (BUS_ADDRESS_ACCEPTED_H),
    .BUS_ADDRESS_INVALID_H (BUS_ADDRESS_INVALID_H),
    //.BUS_SEEK_INCOMPLETE_H (BUS_SEEK_INCOMPLETE_H),
    .BUS_RWS_RDY_H (BUS_RWS_RDY_H),
    .oncylinder_indicator (oncylinder_indicator),
    .strobe_selected_ready (strobe_selected_ready)
);

// improved 4/2/2023
// ======== Module ======== spi_interface =====
spi_interface i_spi_interface (
    // Inputs
    .clock (clock),
    .reset (reset),
    .spi_clk (CPU_SPI_CLK),
    .spi_cs_n (CPU_SPI_CS_n),
    .spi_mosi (CPU_SPI_MOSI),
    .dram_readdata (dram_readdata),
    .dram_readack (dram_readack),
    .dram_writeack (dram_writeack),
    .BUS_WT_PROTECT_L (BUS_WT_PROTECT_L),
    .Cylinder_Address (Cylinder_Address),
    .Head_Select (Head_Select),
    .Selected_Ready (Selected_Ready),
    .BUS_RK11D_L (BUS_RK11D_L),
    .BUS_SEL_DR_L (BUS_SEL_DR_L),
    .BUS_CYL_ADD_L (BUS_CYL_ADD_L),
    .BUS_STROBE_L (BUS_STROBE_L),
    .BUS_HEAD_SELECT_L (BUS_HEAD_SELECT_L),
    //.BUS_WT_PROTECT_L (BUS_WT_PROTECT_L),
    .BUS_WT_DATA_CLK_L (BUS_WT_DATA_CLK_L),
    .BUS_WT_GATE_L (BUS_WT_GATE_L),
    .BUS_RESTORE_L (BUS_RESTORE_L),
    .BUS_RD_GATE_L (BUS_RD_GATE_L),
    .major_version (MAJOR_VERSION),
    .minor_version (MINOR_VERSION),
    .Sector_Address (Sector_Address),
    .strobe_selected_ready (strobe_selected_ready),
    .read_selected_ready (read_selected_ready),
    .write_selected_ready (write_selected_ready),
    .clkenbl_1usec (clkenbl_1usec),

    // Outputs
    .spi_miso (CPU_SPI_MISO),
    .load_address_spi (load_address_spi),
    .spi_serpar_reg (spi_serpar_reg),
    .dram_read_enbl_spi (dram_read_enbl_spi),
    .dram_write_enbl_spi (dram_write_enbl_spi),
    .dram_writedata_spi (dram_writedata_spi),
    .Drive_Address (Drive_Address),
    .File_Ready (File_Ready),
    .Write_Protect (Write_Protect),
    .Fault_Latch (Fault_Latch),
    .cpu_dc_low (cpu_dc_low),
    .preamble1_length (preamble1_length),
    .preamble2_length (preamble2_length),
    .data_length (data_length),
    .postamble_length (postamble_length),
    .number_of_sectors (number_of_sectors),
    .bitclockdivider_clockphase (bitclockdivider_clockphase),
    .bitclockdivider_dataphase (bitclockdivider_dataphase),
    .bitpulse_width (bitpulse_width),
    .microseconds_per_sector (microseconds_per_sector),
    .interface_test_mode (interface_test_mode),
    .command_interrupt (CMD_INTERRUPT),
    .Servo_Pulse_FPGA (Servo_Pulse_FPGA)
);

// improved 4/1/2023
// ======== Module ======== timing_gen =====
timing_gen i_timing_gen (
    // Inputs
    .clock (clock),
    .reset (reset),
    .bitclockdivider_clockphase (bitclockdivider_clockphase),
    .bitclockdivider_dataphase (bitclockdivider_dataphase),
    .bitpulse_width (bitpulse_width),

    // Outputs
    .clkenbl_read_bit (clkenbl_read_bit),
    .clkenbl_read_data (clkenbl_read_data),
    .clock_pulse (clock_pulse),
    .data_pulse (data_pulse),
    .clkenbl_1usec (clkenbl_1usec)
);

endmodule // RK05_emulator_top
