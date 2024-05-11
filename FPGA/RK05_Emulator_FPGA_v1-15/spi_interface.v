//==========================================================================================================
// RK05 Emulator
// Processor SPI Interface
// File Name: spi_interface.v
// Functions: 
//   read and write FPGA hardware control registers.
//   read and write SDRAM data.
//   write SDRAM address register for processor SDRAM accesses.
//
//==========================================================================================================

module spi_interface(
    input wire clock,                   // master clock 40 MHz
    input wire reset,                   // active high synchronous reset input
    input wire spi_clk,                 // SPI clock
    input wire spi_cs_n,                // SPI active low chip select
    input wire spi_mosi,                // SPI controller data output, peripheral data input
    input wire [15:0] dram_readdata,    // 16-bit read data from DRAM controller
    input wire dram_readack,            // dram read acknowledge ==== maybe no longer needed?? ============
    input wire dram_writeack,           // dram read acknowledge
    input wire BUS_WT_PROTECT_L,        // write protect strobe from the bus
    input wire [7:0] Cylinder_Address,  // input to be able to read the Cylinder Address
    input wire Head_Select,             // input to be able to read the Head Select bit
    input wire Selected_Ready,          // input to be able to read Selected_Ready
    input wire BUS_RK11D_L,             // RK11 mode for Drive Select BUS_RK11D_L
    input wire [3:0] BUS_SEL_DR_L,      // Drive Select signals
    input wire [7:0] BUS_CYL_ADD_L,     // Cylinder Address
    input wire BUS_STROBE_L,            // Strobe to enable movement, gates the Cylinder Address or Restore line
    input wire BUS_HEAD_SELECT_L,       // Head Select
    //BUS_WT_PROTECT_L,                 // Write Protect
    input wire BUS_WT_DATA_CLK_L,       // Composite write data and write clock
    input wire BUS_WT_GATE_L,           // Write gate, when active enables write circuitry
    input wire BUS_RESTORE_L,           // Restore, moves heads to cylinder 0
    input wire BUS_RD_GATE_L,           // Read gate, when active enables read circuitry
    input wire [7:0] major_version,
    input wire [7:0] minor_version,
    input wire [3:0] Sector_Address,    // Sector Address to be read test visibility mode
    input wire strobe_selected_ready,
    input wire read_selected_ready,
    input wire write_selected_ready,
    input wire clkenbl_1usec,           // 1 usec clock enable input from the timing generator

    output reg spi_miso,                // SPI controller data input, peripheral data output
    output reg load_address_spi,        // enable from SPI to command the sdram controller to load address 8 bits at a time
    output reg [7:0] spi_serpar_reg,    // 8-bit serpar register used for writing to the sdram address register
    output reg dram_read_enbl_spi,      // read enable request to DRAM controller
    output reg dram_write_enbl_spi,     // write enable request to DRAM controller
    output reg [15:0] dram_writedata_spi, // 16-bit write data to DRAM controller
    output reg [2:0] Drive_Address,     // 3-bits internal drive address
    output reg File_Ready,              // disk contents have been copied from the microSD to the SDRAM.
    output reg Write_Protect,           // CPU register that indicates the drive write protect status.
    output reg Fault_Latch,             // included for future support. Software will always write zero to Fault_Latch.
    output reg cpu_dc_low,              // DC low signal driven by a CPU register
    output reg [7:0] preamble1_length,
    output reg [7:0] preamble2_length,
    output reg [15:0] data_length,
    output reg [7:0] postamble_length,
    output reg [4:0] number_of_sectors,
    output reg [7:0] bitclockdivider_clockphase,
    output reg [7:0] bitclockdivider_dataphase,
    output reg [7:0] bitpulse_width,
    output reg [15:0] microseconds_per_sector,
    output reg interface_test_mode,
    output reg command_interrupt,
    output reg Servo_Pulse_FPGA
);

//============================ Internal Connections ==================================

//`define EMULATOR_FPGA_CODE_VERSION  8'h51  // hex number that indicates the version of the FPGA code

reg [7:0] spiserialreg;
reg [3:0] metaspi;
reg [2:0] metawprot;
reg dramwrite_lowhigh;
reg dramread_lowhigh;
reg [4:0] spicount; // define as 5 bits instead of 4 to prevent the first bit from wrapping around at the end of transmitting the 16-bit data (8 addr + 8 data)
reg [7:0] serialaddress;
wire [7:0] muxed_read_data;
wire pre_spi_miso;
reg frdlyd;
reg toggle_wp;
reg [1:0] operation_id;
reg [7:0] servo_pw;
reg [10:0] counter_servo_20ms_period; // counts 1250 16-usec intervals for the 20 ms servo pulse period
reg [3:0] counter_16usec;

wire spi_start;

//============================ Start of Code =========================================

// SB_DFFS - D Flip-Flop, Set is asynchronous to the clock.
SB_DFFS SPI_DFFS_inst (
.Q(spi_start), // Registered Output, "Q" output of the DFF
.C(~spi_clk),  // rising-edge Clock, so with ~spi_clk as the input, Q changes on the falling edge of spi_clk
.D(1'b0),      // Data, clocks in a zero on the falling edge of spi_clk
.S(spi_cs_n)   // Asynchronous active-high Set, we perform async set of the DFF while spi_cs_n is inactive
);

assign muxed_read_data = (serialaddress == 8'h80) ? 8'h00 : 
                        ((serialaddress == 8'h81) ? Cylinder_Address[7:0] :
                        ((serialaddress == 8'h82) ? {Sector_Address[3:0], operation_id[1:0], Selected_Ready, Head_Select} :
                        ((serialaddress == 8'h83) ? 8'h00 :
                        ((serialaddress == 8'h84) ? 8'h00 :
                        ((serialaddress == 8'h85) ? 8'h00 :
                        ((serialaddress == 8'h89) ? {1'b0, 7'h0} : // bit 7 == 0 identifies the FPGA as an emulator, bits 6:0 are presently unused
                        ((serialaddress == 8'h90) ? major_version[7:0] :
                        ((serialaddress == 8'h91) ? minor_version[7:0] :
                        ((serialaddress == 8'h94) ? BUS_CYL_ADD_L[7:0] :
                        ((serialaddress == 8'h95) ? {BUS_RD_GATE_L, BUS_RESTORE_L, BUS_WT_GATE_L, BUS_WT_DATA_CLK_L, BUS_WT_PROTECT_L, BUS_HEAD_SELECT_L, BUS_STROBE_L, BUS_RK11D_L} :
                        ((serialaddress == 8'h96) ? {4'b0000, BUS_SEL_DR_L[3:0]} :
                        ((serialaddress == 8'ha0) ? {cpu_dc_low, 1'b0, Fault_Latch, File_Ready, 1'b0, Drive_Address[2:0]} : // read-back of register 0x0
                        ((serialaddress == 8'ha7) ? preamble1_length[7:0] :
                        ((serialaddress == 8'ha8) ? preamble2_length[7:0] :
                        ((serialaddress == 8'ha9) ? data_length[15:8] :
                        ((serialaddress == 8'haa) ? data_length[7:0] :
                        ((serialaddress == 8'hab) ? postamble_length[7:0] :
                        ((serialaddress == 8'hac) ? {3'b000, number_of_sectors[4:0]} : // commented out to reduce PLBs consumed so the design fits in the part
                        ((serialaddress == 8'had) ? bitclockdivider_clockphase[7:0] :
                        ((serialaddress == 8'hae) ? bitclockdivider_dataphase[7:0] :
                        ((serialaddress == 8'haf) ? bitpulse_width[7:0] :
                        ((serialaddress == 8'hb0) ? microseconds_per_sector[15:8] :
                        ((serialaddress == 8'hb1) ? microseconds_per_sector[7:0] :
                        ((serialaddress == 8'h88) ? (dramread_lowhigh ? dram_readdata[15:8] : dram_readdata[7:0]) : 8'b0))))))))))))))))))))))));
                        // dram_readdata[15:0] always has the data ready that was read at the dram_address.
                        // The DRAM word read function is triggered after the odd byte is read.
                        // The next word is requested after reading the high byte from register 0x88.

assign pre_spi_miso = ((spicount == 5'd7) & muxed_read_data[7]) | 
                      ((spicount == 5'd8) & muxed_read_data[6]) |
                      ((spicount == 5'd9) & muxed_read_data[5]) |
                      ((spicount == 5'd10) & muxed_read_data[4]) |
                      ((spicount == 5'd11) & muxed_read_data[3]) |
                      ((spicount == 5'd12) & muxed_read_data[2]) |
                      ((spicount == 5'd13) & muxed_read_data[1]) |
                      ((spicount == 5'd14) & muxed_read_data[0]);

always @ (posedge spi_clk)
begin : SPICLKPOSFUNCTIONS // block name
  // Reset the SPI bit counter using the DFF that is set when spi_cs_n is inactive
  // The SPI bit counter is used by a mux to serialize the SPI read data.
  spicount <= spi_start ? 5'd0 : spicount + 1;
  serialaddress <= (spicount == 6) ? {spiserialreg[6:0], spi_mosi} : serialaddress;

  if(spi_cs_n == 1'b0) begin
    spiserialreg[7:0] <= {spiserialreg[6:0], spi_mosi};
    //spiserialreg[7] <= spiserialreg[6];
    //spiserialreg[6] <= spiserialreg[5];
    //spiserialreg[5] <= spiserialreg[4];
    //spiserialreg[4] <= spiserialreg[3];
    //spiserialreg[3] <= spiserialreg[2];
    //spiserialreg[2] <= spiserialreg[1];
    //spiserialreg[1] <= spiserialreg[0];
    //spiserialreg[0] <= spi_mosi;
  end
  else begin
    spiserialreg[7:0] <= 8'hff;
  end
end

always @ (negedge spi_clk)
begin : SPICLKNEGFUNCTIONS // block name
  spi_miso <= pre_spi_miso;
end

always @ (posedge spi_cs_n)
begin : SPICSPOSFUNCTIONS // block name
    spi_serpar_reg <=  spiserialreg;
end

always @ (posedge clock)
begin : HSCLOCKFUNCTIONS // block name
  if(reset == 1'b1) begin
    dram_read_enbl_spi <= 1'b0;
    dram_write_enbl_spi <= 1'b0;
    Drive_Address <= 3'd0;
    File_Ready <= 1'b0;
    frdlyd <= 1'b0;
    Write_Protect <= 1'b0;
    Fault_Latch <= 1'b0;
    load_address_spi <= 1'b0;
    dramwrite_lowhigh <= 1'b0;
    dramread_lowhigh <= 1'b0;
    metaspi <= 4'b0000;
    metawprot <= 3'b000;
    cpu_dc_low <= 1'b0;
    toggle_wp <= 1'b0;
    preamble1_length <= 8'd120; //default for RK8-E, 120 bit times or 0x78
    preamble2_length <= 8'd82; //default for RK8-E, 82 bit times or 0x52
    data_length <= 16'h0c20; //default for RK8-E, 3072 bit times + 2 * 16 bits for Header & CRC = 0x0c20
    postamble_length <= 8'd36; //default for RK8-E, 36 bit times or 0x24
    number_of_sectors[4:0] <= 5'd16; //default for RK8-E, 16 sectors or 0x10
    bitclockdivider_clockphase <= 8'd14; //default for RK8-E, system clock divided by 14 for half a bit, clock phase
    bitclockdivider_dataphase <= 8'd14; //default for RK8-E, system clock divided by 14 for half a bit, data phase
    bitpulse_width <= 8'd6; //default for RK8-E, clock and data pulse width is 6 system clocks
    microseconds_per_sector <= 16'd2500; //default for RK8-E, 2.5 msec per sector, 2.5 * 16 = 40 msec per revolution, 0x09c4
    interface_test_mode <= 1'b0;
    operation_id <= 2'b00;
    command_interrupt <= 1'b0;
    servo_pw <= 8'd47; // 0.75 msec is 47, 16 usec intervals
    counter_servo_20ms_period <= 11'h0;
    counter_16usec <= 4'h0;
  end
  else begin
    counter_16usec <= clkenbl_1usec ? counter_16usec + 1 : counter_16usec;
    counter_servo_20ms_period <= (clkenbl_1usec & (counter_16usec == 15)) ? 
        ((counter_servo_20ms_period < 11'd1249) ? counter_servo_20ms_period + 1 : 11'd0) : counter_servo_20ms_period;
    Servo_Pulse_FPGA <= ({3'h0, servo_pw[7:0]} > counter_servo_20ms_period);
    
    command_interrupt <= strobe_selected_ready || read_selected_ready || write_selected_ready;
    operation_id <= strobe_selected_ready ? 2'h0 : (read_selected_ready ? 2'h1 : (write_selected_ready ? 2'h2 : operation_id));

    frdlyd <= File_Ready;
    metaspi[3:0] <= {metaspi[2:0], ~spi_cs_n};
    metawprot[2:0] <= {metawprot[1:0], (~BUS_WT_PROTECT_L & Selected_Ready)};

    //clear Write_Protect when there's a change in File_Ready
    //Write_Protect <= metawprot[2] | // if metawp then set wp 
                     //(~(File_Ready ^ frdlyd) & ~metawprot[2] &  (serialaddress == 8'h04) & ~metaspi[2] & metaspi[3] &  spi_serpar_reg[0] & ~Write_Protect) |
                     //(~(File_Ready ^ frdlyd) & ~metawprot[2] & ((serialaddress == 8'h04) | metaspi[2] |  ~metaspi[3] | ~spi_serpar_reg[0]) & Write_Protect);                
    // Q <= (Q | Set) & ~Reset
    // Set when metawprot[2] or (toggle_wp & ~Q)
    // Reset when (toggle_wp & Q) or (File_Ready ^ frdlyd)
    toggle_wp <= (serialaddress == 8'h04) & ~metaspi[2] & metaspi[3] &  spi_serpar_reg[0]; //toggle_wp is separated only so the code is more readable
    Write_Protect <= (Write_Protect | (metawprot[2] | (toggle_wp & ~Write_Protect))) & ~((toggle_wp & Write_Protect) | (File_Ready ^ frdlyd));
 
    // register address 0x00
    Drive_Address[2:0] <= ((serialaddress == 8'h00) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg[2:0] : Drive_Address[2:0];
    File_Ready <=         ((serialaddress == 8'h00) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg[4] : File_Ready;
    Fault_Latch <=        ((serialaddress == 8'h00) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg[5] : Fault_Latch;
    cpu_dc_low <=         ((serialaddress == 8'h00) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg[7] : cpu_dc_low;

    // register address 0x05
    load_address_spi   <= (serialaddress == 8'h05) & ~metaspi[2] & metaspi[3]; // command to load 8 bits of address from SPI

    // register address 0x06
    dram_writedata_spi[7:0] <=  ((serialaddress == 8'h06) && ~metaspi[2] && metaspi[3]) ? dram_writedata_spi[15:8] : dram_writedata_spi[7:0];
    dram_writedata_spi[15:8] <= ((serialaddress == 8'h06) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg : dram_writedata_spi[15:8];
    dram_write_enbl_spi <=       (serialaddress == 8'h06) & ~metaspi[2] & metaspi[3] & dramwrite_lowhigh;

    // register address 0x07
    preamble1_length <= ((serialaddress == 8'h07) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg[7:0] : preamble1_length;

    // register address 0x08
    preamble2_length <= ((serialaddress == 8'h08) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg[7:0] : preamble2_length;

    // register address 0x09
    data_length[15:8] <= ((serialaddress == 8'h09) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg[7:0] : data_length[15:8];

    // register address 0x0a
    data_length[7:0] <= ((serialaddress == 8'h0a) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg[7:0] : data_length[7:0];

    // register address 0x0b
    postamble_length <= ((serialaddress == 8'h0b) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg[7:0] : postamble_length;

    // register address 0x0c
    number_of_sectors <= ((serialaddress == 8'h0c) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg[4:0] : number_of_sectors;

    // register address 0x0d
    bitclockdivider_clockphase <= ((serialaddress == 8'h0d) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg[7:0] : bitclockdivider_clockphase;

    // register address 0x0e
    bitclockdivider_dataphase <= ((serialaddress == 8'h0e) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg[7:0] : bitclockdivider_dataphase;

    // register address 0x0f
    bitpulse_width <= ((serialaddress == 8'h0f) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg[7:0] : bitpulse_width;

    // register address 0x10
    microseconds_per_sector[15:8] <= ((serialaddress == 8'h10) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg[7:0] : microseconds_per_sector[15:8];

    // register address 0x11
    microseconds_per_sector[7:0] <= ((serialaddress == 8'h11) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg[7:0] : microseconds_per_sector[7:0];

    // register address 0x12
    servo_pw[7:0] <= ((serialaddress == 8'h12) && ~metaspi[2] && metaspi[3]) ? spi_serpar_reg[7:0] : servo_pw[7:0];

    // register address 0x20
    interface_test_mode <= ((serialaddress == 8'h20) && ~metaspi[2] && metaspi[3]) ? (spi_serpar_reg[7:0] == 8'h55) : interface_test_mode;

    // register address 0x88
    dram_read_enbl_spi <= (serialaddress == 8'h88) & ~metaspi[2] & metaspi[3] & dramread_lowhigh;

    // dram_readdata[15:0] always has the data ready that was read at the dram_address.
    // The read function is triggered after the odd byte is read.
    // The next word is requested after reading the high byte when the SPI address is 8'h88.
    // toggle respective lowhigh bits on a write or read, clear both bits on address load, otherwise lowhigh bits remain the same
    dramwrite_lowhigh <= ((serialaddress == 8'h06) && ~metaspi[2] && metaspi[3]) ? ~dramwrite_lowhigh : 
                        (((serialaddress == 8'h05) && ~metaspi[2] && metaspi[3]) ? 1'b0 : dramwrite_lowhigh);
    dramread_lowhigh  <= ((serialaddress == 8'h88) && ~metaspi[2] && metaspi[3]) ? ~dramread_lowhigh :
                        (((serialaddress == 8'h05) && ~metaspi[2] && metaspi[3]) ? 1'b0 : dramread_lowhigh);
  end
end // End of Block HSCLOCKFUNCTIONS

endmodule // End of Module spi_interface
