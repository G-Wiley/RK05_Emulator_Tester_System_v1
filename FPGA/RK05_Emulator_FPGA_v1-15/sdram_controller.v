//==========================================================================================================
// RK05 Emulator
// SDRAM Controller
// File Name: sdram_controller.v
// Functions: 
//   initialization, 
//   refresh, 
//   read from bus, 
//   read from SPI, 
//   write from bus, 
//   write from SPI.
//
//==========================================================================================================

module sdram_controller(
    input wire clock,                    // master clock 40 MHz
    input wire reset,                    // active high synchronous reset input
    input wire load_address_spi,         // enable 8-bit load from SPI to command the sdram controller to load the address from sector, head select and cylinder
    input wire load_address_busread,     // enable from bus read to command the sdram controller to load the address from sector, head select and cylinder
    input wire load_address_buswrite,    // enable from bus write to command the sdram controller to load the address from sector, head select and cylinder
    input wire dram_read_enbl_spi,       // read enable request to DRAM controller from SPI
    input wire dram_read_enbl_busread,   // read enable request to DRAM controller from the BUS interface
    input wire dram_write_enbl_spi,      // write enable request to DRAM controller from SPI
    input wire dram_write_enbl_buswrite, // write enable request to DRAM controller from the BUS interface
    input wire [15:0] dram_writedata_spi,       // 16-bit write data to DRAM controller from SPI
    input wire [15:0] dram_writedata_buswrite,   // 16-bit write data to DRAM controller from SPI
    input wire Write_Protect,            // CPU register that indicates the drive write protect status.
    input wire [7:0] spi_serpar_reg,           // 8-bit SPI serpar register used for writing to the sdram address register
    input wire [3:0] Sector_Address,           // specifies which sector is present "under the heads"
    input wire [7:0] Cylinder_Address,         // valid cylinder address
    input wire Head_Select,              // head selection (upper or lower)

    input wire [15:0] SDRAM_DQ_in,     // input from DQ signal receivers

    output reg [15:0] dram_readdata,   // 16-bit read data from DRAM controller
    output reg dram_readack,    // dram read acknowledge ==== maybe no longer needed?? ============
    output reg dram_writeack,   // dram read acknowledge

    output reg [15:0] SDRAM_DQ_output, // outputs to DQ signal drivers
    output reg SDRAM_DQ_enable, // DQ output enable, active high
    output reg [12:0] SDRAM_Address,   // SDRAM Address
    output reg SDRAM_BS0,	     // SDRAM Bank Select 0
    output reg SDRAM_BS1,	     // SDRAM Bank Select 1
    output reg SDRAM_WE_n,	     // SDRAM Write
    output reg SDRAM_CAS_n,	 // SDRAM Column Address Select
    output reg SDRAM_RAS_n,	 // SDRAM Row Address Select
    output reg SDRAM_CS_n,	     // SDRAM Chip Select
    output wire SDRAM_CLK,	     // SDRAM Clock
    output reg SDRAM_CKE,	     // SDRAM Clock Enable
    output reg SDRAM_DQML,	     // SDRAM DQ Mask for Lower Byte
    output reg SDRAM_DQMH	     // SDRAM DQ Mask for Upper (High) Byte
);

//============================ Internal Connections ==================================

// === COMMANDS USED BY THE SDRAM CONTROLLER ===
// CS RAS CAS WE Bank A10 An
// == === === == ==== === ==  ========================================================
// H   x   x  x   x    x  x   No Operation
// L   L   L  L   00  .mode.  Load Mode Register
// L   L   H  H  bank .row..  Activate, open a row
// L   H   L  H  bank  H col  Read with auto precharge (read and close row)
// L   H   L  L  bank  H col  Write with auto precharge (Write and close row)
// L   L   L  H   x    x  x   Auto Refresh
// L   L   H  L   x    H  x   Precharge All, precharge the current row of all banks
//
// During Reset, 200 us pause, DQML, DQMH and CKE held high during initial pause period
// After the 200 us pause, set the mode register, then issue eight Auto Refresh cycles
// Many Auto Refresh cycles will happen automatically from controller ST0 (command dispatch NOP)

// SDRAM Controller state definitions and values
`define ST0  5'd0  // 0  - command dispatch NOP
`define ST1  5'd1  // 1  - Read Activate
`define ST2  5'd2  // 2  - pre-Read NOP
`define ST3  5'd3  // 3  - Read Auto Precharge
`define ST4  5'd4  // 4  - Read Precharge Wait
`define ST5  5'd5  // 5  - Read Capture Data
`define ST6  5'd6  // 6  - Write Activate
`define ST7  5'd7  // 7  - pre-Write NOP
`define ST8  5'd8  // 8  - Write Auto Precharge
`define ST9  5'd9  // 9  - Write Precharge Wait
`define ST10 5'd10 // 10 - Write After Precharge Wait
`define ST11 5'd11 // 11 - Auto Refresh
`define ST12 5'd12 // 12 - After Auto Refresh Wait
`define ST13 5'd13 // 13 - Init Precharge All
`define ST14 5'd14 // 14 - Init Precharge Wait
`define ST15 5'd15 // 15 - Init Load Mode Register
`define ST16 5'd16 // 16 - Init NOP before Precharge All


reg [23:0] memory_address; // memory address register
reg [15:0] spi_mem_addr;   // register to save prior bytes of SPI memory address
reg [4:0] memstate; // memory controller state
reg readrequest;
//reg read_on_spi_addr;
reg writerequest_spi;
reg writerequest_buswrite;
reg capture_readdata;

//============================ Start of Code =========================================

// dram_readdata[15:0] always has the data ready that was read at the memory_address.
// The read function is triggered after the odd byte is read.
// The following code is triggered when spi_cs_n is low and counts clocks
//   which is used by a mux to serialize the low or high byte of dram_readdata[15:0].
// The next word is requested after reading the high byte when spi_reg_select == 3.

assign SDRAM_CLK = ~clock;

always @ (posedge clock)
begin : HSCLOCKFUNCTIONS // block name
  if(reset) begin
    dram_readdata <= 16'd0;
    dram_readack <= 1'd0;
    dram_writeack <= 1'd0;
    memory_address <= 24'd0;
    spi_mem_addr <= 16'd0;
    memstate <= `ST16;
    readrequest <= 1'd0;
    //read_on_spi_addr <= 1'b0;
    writerequest_spi <= 1'd0;
    writerequest_buswrite <= 1'd0;
    capture_readdata <= 1'd0;

    SDRAM_CS_n <= 1'b1;
    SDRAM_RAS_n <= 1'b1;
    SDRAM_CAS_n <= 1'b1;
    SDRAM_WE_n <= 1'b1;
    SDRAM_BS1 <= 1'b0;
    SDRAM_BS0 <= 1'b0;
    SDRAM_Address <= 13'd0;
    SDRAM_DQ_output <= 16'd0;
    SDRAM_DQ_enable <= 1'b0;
    SDRAM_CKE <= 1'b1;
    SDRAM_DQML <= 1'b1;
    SDRAM_DQMH <= 1'b1;
  end
  else begin
    SDRAM_CKE <= 1'b1;

    // memory_address affected by:
    //   load_address_spi;  load_address_busread;  load_address_buswrite;
    //   dram_readack;  dram_writeack;  <if none of these - then no change to memory_address;>
    spi_mem_addr <= load_address_spi ? {spi_mem_addr[7:0], spi_serpar_reg[7:0]}: spi_mem_addr;
    memory_address <=  load_address_spi ?                             {spi_mem_addr[15:8], spi_mem_addr[7:0], spi_serpar_reg[7:0]} :
                    (load_address_busread | load_address_buswrite ? {2'b00, Cylinder_Address[7:0], Head_Select, Sector_Address[3:0], 9'h0} :
                    ((dram_read_enbl_spi | dram_read_enbl_busread | dram_writeack) ?                 memory_address + 1 : memory_address));

    capture_readdata <= (memstate == `ST5); // capture sdram read data the clock cycle after state ST5
    dram_readdata <= capture_readdata ? SDRAM_DQ_in : dram_readdata; // capture sdram read data in state ST5

    // readrequest: SET on (dram_read_enbl_spi | dram_read_enbl_busread | load_address_spi), CLEAR on (memstate == 'ST5)
    readrequest <= (dram_read_enbl_spi | dram_read_enbl_busread | load_address_spi | load_address_busread) | (readrequest & ~(memstate == `ST5));
    
    // read_on_spi_addr: SET on load_address_spi, CLEAR on (memstate == 'ST5)
    // we do this so in the memory_address calculation, we can avoid the auto-increment after the memory read due to load addr from SPI
    // No longer necessary, read_on_spi_addr is commented out everywhere now.
    //read_on_spi_addr <= load_address_spi | (read_on_spi_addr & ~(memstate == `ST5));

    // writerequest_spi: SET on (dram_write_enbl_spi & ~Write_Protect), CLEAR on (memstate == 'ST10)
    writerequest_spi <=  (dram_write_enbl_spi & ~Write_Protect) | (writerequest_spi & ~(memstate == `ST10));  

    // writerequest_buswrite: SET on (dram_write_enbl_buswrite & ~Write_Protect), CLEAR on (memstate == 'ST10);
    writerequest_buswrite <= (dram_write_enbl_buswrite & ~Write_Protect) | (writerequest_buswrite & ~(memstate == `ST10));

    dram_readack <= (memstate == `ST4);
    dram_writeack <= (memstate == `ST9);

    case(memstate)  // SDRAM Controller state machine
    `ST0: begin     // 0  - command dispatch NOP
      memstate <= readrequest ? `ST1 : ((writerequest_spi | writerequest_buswrite) ? `ST6 : `ST11);
      SDRAM_CS_n <= 1'b1;
      SDRAM_RAS_n <= 1'b1;
      SDRAM_CAS_n <= 1'b1;
      SDRAM_WE_n <= 1'b1;
      SDRAM_BS1 <= 1'b0;
      SDRAM_BS0 <= 1'b0;
      SDRAM_Address <= 13'd0;
      SDRAM_DQ_output <= 16'd0;
      SDRAM_DQ_enable <= 1'b0;
      SDRAM_DQML <= 1'b0;
      SDRAM_DQMH <= 1'b0;
     end
    `ST1: begin     // 1  - Read Activate
      memstate <= `ST2;
      SDRAM_CS_n <= 1'b0;
      SDRAM_RAS_n <= 1'b0;
      SDRAM_CAS_n <= 1'b1;
      SDRAM_WE_n <= 1'b1;
      SDRAM_BS1 <= memory_address[23];
      SDRAM_BS0 <= memory_address[22];
      SDRAM_Address <= memory_address[21:9];
      SDRAM_DQ_output <= 16'd0;
      SDRAM_DQ_enable <= 1'b0;
      SDRAM_DQML <= 1'b0;
      SDRAM_DQMH <= 1'b0;
     end
    `ST2: begin     // 2  - pre-Read NOP
      memstate <= `ST3;
      SDRAM_CS_n <= 1'b1;
      SDRAM_RAS_n <= 1'b1;
      SDRAM_CAS_n <= 1'b1;
      SDRAM_WE_n <= 1'b1;
      SDRAM_BS1 <= 1'b0;
      SDRAM_BS0 <= 1'b0;
      SDRAM_Address <= 13'd0;
      SDRAM_DQ_output <= 16'd0;
      SDRAM_DQ_enable <= 1'b0;
      SDRAM_DQML <= 1'b0;
      SDRAM_DQMH <= 1'b0;
     end
    `ST3: begin     // 3  - Read Auto Precharge
      memstate <= `ST4;
      SDRAM_CS_n <= 1'b0;
      SDRAM_RAS_n <= 1'b1;
      SDRAM_CAS_n <= 1'b0;
      SDRAM_WE_n <= 1'b1;
      SDRAM_BS1 <= memory_address[23];
      SDRAM_BS0 <= memory_address[22];
      SDRAM_Address <= {4'b0010, memory_address[8:0]}; // 9 lower bits of memory address with A10 <= 1
      SDRAM_DQ_output <= 16'd0;
      SDRAM_DQ_enable <= 1'b0;
      SDRAM_DQML <= 1'b0;
      SDRAM_DQMH <= 1'b0;
     end
    `ST4: begin     // 4  - Read Precharge Wait
      memstate <= `ST5;
      SDRAM_CS_n <= 1'b1;
      SDRAM_RAS_n <= 1'b1;
      SDRAM_CAS_n <= 1'b1;
      SDRAM_WE_n <= 1'b1;
      SDRAM_BS1 <= 1'b0;
      SDRAM_BS0 <= 1'b0;
      SDRAM_Address <= 13'd0;
      SDRAM_DQ_output <= 16'd0;
      SDRAM_DQ_enable <= 1'b0;
      SDRAM_DQML <= 1'b0;
      SDRAM_DQMH <= 1'b0;
     end
    `ST5: begin     // 5  - Read Capture Data
      memstate <= `ST0;
      SDRAM_CS_n <= 1'b1;
      SDRAM_RAS_n <= 1'b1;
      SDRAM_CAS_n <= 1'b1;
      SDRAM_WE_n <= 1'b1;
      SDRAM_BS1 <= 1'b0;
      SDRAM_BS0 <= 1'b0;
      SDRAM_Address <= 13'd0;
      SDRAM_DQ_output <= 16'd0;
      SDRAM_DQ_enable <= 1'b0;
      SDRAM_DQML <= 1'b0;
      SDRAM_DQMH <= 1'b0;
     end
    `ST6: begin     // 6  - Write Activate
      memstate <= `ST7;
      SDRAM_CS_n <= 1'b0;
      SDRAM_RAS_n <= 1'b0;
      SDRAM_CAS_n <= 1'b1;
      SDRAM_WE_n <= 1'b1;
      SDRAM_BS1 <= memory_address[23];
      SDRAM_BS0 <= memory_address[22];
      SDRAM_Address <= memory_address[21:9];
      SDRAM_DQ_output <= 16'd0;
      SDRAM_DQ_enable <= 1'b0;
      SDRAM_DQML <= 1'b0;
      SDRAM_DQMH <= 1'b0;
     end
    `ST7: begin     // 7  - pre-Write NOP
      memstate <= `ST8;
      SDRAM_CS_n <= 1'b1;
      SDRAM_RAS_n <= 1'b1;
      SDRAM_CAS_n <= 1'b1;
      SDRAM_WE_n <= 1'b1;
      SDRAM_BS1 <= 1'b0;
      SDRAM_BS0 <= 1'b0;
      SDRAM_Address <= 13'd0;
      SDRAM_DQ_output <= 16'd0;
      SDRAM_DQ_enable <= 1'b0;
      SDRAM_DQML <= 1'b0;
      SDRAM_DQMH <= 1'b0;
     end
    `ST8: begin     // 8  - Write Auto Precharge
      memstate <= `ST9;
      SDRAM_CS_n <= 1'b0;
      SDRAM_RAS_n <= 1'b1;
      SDRAM_CAS_n <= 1'b0;
      SDRAM_WE_n <= 1'b0;
      SDRAM_BS1 <= memory_address[23];
      SDRAM_BS0 <= memory_address[22];
      SDRAM_Address <= {4'b0010, memory_address[8:0]}; // 9 lower bits of memory address with A10 <= 1
      SDRAM_DQ_output <= writerequest_spi ? dram_writedata_spi : (writerequest_buswrite ? dram_writedata_buswrite : 16'd0);
      SDRAM_DQ_enable <= 1'b1;
      SDRAM_DQML <= 1'b0;
      SDRAM_DQMH <= 1'b0;
     end
    `ST9: begin     // 9  - Write Precharge Wait
      memstate <= `ST10;
      SDRAM_CS_n <= 1'b1;
      SDRAM_RAS_n <= 1'b1;
      SDRAM_CAS_n <= 1'b1;
      SDRAM_WE_n <= 1'b1;
      SDRAM_BS1 <= 1'b0;
      SDRAM_BS0 <= 1'b0;
      SDRAM_Address <= 13'd0;
      SDRAM_DQ_output <= 16'd0;
      SDRAM_DQ_enable <= 1'b0;
      SDRAM_DQML <= 1'b0;
      SDRAM_DQMH <= 1'b0;
     end
    `ST10: begin     // 10 - Write After Precharge Wait
      memstate <= `ST0;
      SDRAM_CS_n <= 1'b1;
      SDRAM_RAS_n <= 1'b1;
      SDRAM_CAS_n <= 1'b1;
      SDRAM_WE_n <= 1'b1;
      SDRAM_BS1 <= 1'b0;
      SDRAM_BS0 <= 1'b0;
      SDRAM_Address <= 13'd0;
      SDRAM_DQ_output <= 16'd0;
      SDRAM_DQ_enable <= 1'b0;
      SDRAM_DQML <= 1'b0;
      SDRAM_DQMH <= 1'b0;
     end
    `ST11: begin     // 11 - Auto Refresh
      memstate <= `ST12;
      SDRAM_CS_n <= 1'b0;
      SDRAM_RAS_n <= 1'b0;
      SDRAM_CAS_n <= 1'b0;
      SDRAM_WE_n <= 1'b1;
      SDRAM_BS1 <= 1'b0;
      SDRAM_BS0 <= 1'b0;
      SDRAM_Address <= 13'd0;
      SDRAM_DQ_output <= 16'd0;
      SDRAM_DQ_enable <= 1'b0;
      SDRAM_DQML <= 1'b0;
      SDRAM_DQMH <= 1'b0;
     end
    `ST12: begin     // 12 - After Auto Refresh Wait
      memstate <= `ST0;
      SDRAM_CS_n <= 1'b1;
      SDRAM_RAS_n <= 1'b1;
      SDRAM_CAS_n <= 1'b1;
      SDRAM_WE_n <= 1'b1;
      SDRAM_BS1 <= 1'b0;
      SDRAM_BS0 <= 1'b0;
      SDRAM_Address <= 13'd0;
      SDRAM_DQ_output <= 16'd0;
      SDRAM_DQ_enable <= 1'b0;
      SDRAM_DQML <= 1'b0;
      SDRAM_DQMH <= 1'b0;
     end
    `ST13: begin     // 13 - Init Precharge All
      memstate <= `ST14;
      SDRAM_CS_n <= 1'b0;
      SDRAM_RAS_n <= 1'b0;
      SDRAM_CAS_n <= 1'b1;
      SDRAM_WE_n <= 1'b0;
      SDRAM_BS1 <= 1'b0;
      SDRAM_BS0 <= 1'b0;
      SDRAM_Address <= 13'b0010000000000; // Precharge All requires A10 to be high
      SDRAM_DQ_output <= 16'd0;
      SDRAM_DQ_enable <= 1'b0;
      SDRAM_DQML <= 1'b0;
      SDRAM_DQMH <= 1'b0;
     end
    `ST14: begin     // 14 - Init Precharge Wait
      memstate <= `ST15;
      SDRAM_CS_n <= 1'b1;
      SDRAM_RAS_n <= 1'b1;
      SDRAM_CAS_n <= 1'b1;
      SDRAM_WE_n <= 1'b1;
      SDRAM_BS1 <= 1'b0;
      SDRAM_BS0 <= 1'b0;
      SDRAM_Address <= 13'd0;
      SDRAM_DQ_output <= 16'd0;
      SDRAM_DQ_enable <= 1'b0;
      SDRAM_DQML <= 1'b0;
      SDRAM_DQMH <= 1'b0;
     end
    `ST15: begin     // 15 - Init Load Mode Register
      // SDRAM Mode Register setting
      // BS1,BS0,A12,A11,A10 = 5'b00000
      // A9 = 1 Burst read and single write
      // A8,A7 = 2'b00 Reserved and Test Mode
      // A6,A5,A4 = 3'b010 CAS Latency = 2
      // A3 = 0 Sequential
      // A2,A1,A0 = 3'b000 Burst Length = 1
      //
      memstate <= `ST0;
      SDRAM_CS_n <= 1'b0;
      SDRAM_RAS_n <= 1'b0;
      SDRAM_CAS_n <= 1'b0;
      SDRAM_WE_n <= 1'b0;
      SDRAM_BS1 <= 1'b0;
      SDRAM_BS0 <= 1'b0;
      SDRAM_Address <= 13'b0001000100000;
      SDRAM_DQ_output <= 16'd0;
      SDRAM_DQ_enable <= 1'b0;
      SDRAM_DQML <= 1'b0;
      SDRAM_DQMH <= 1'b0;
     end
    `ST16: begin     // 16 - Init NOP before Precharge All
      memstate <= `ST13;
      SDRAM_CS_n <= 1'b1;
      SDRAM_RAS_n <= 1'b1;
      SDRAM_CAS_n <= 1'b1;
      SDRAM_WE_n <= 1'b1;
      SDRAM_BS1 <= 1'b0;
      SDRAM_BS0 <= 1'b0;
      SDRAM_Address <= 13'd0;
      SDRAM_DQ_output <= 16'd0;
      SDRAM_DQ_enable <= 1'b0;
      SDRAM_DQML <= 1'b1;
      SDRAM_DQMH <= 1'b1;
     end
    endcase
  end
end // End of Block HSCLOCKFUNCTIONS

endmodule // End of Module sdram_controller
