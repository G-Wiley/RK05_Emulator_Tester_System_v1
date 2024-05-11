//==========================================================================================================
// RK05 Emulator
// read disk from the BUS
// File Name: bus_disk_read.v
// Functions: 
//   emulates reading the disk from the interface bus.
//   When BUS_RD_GATE_L goes active (low) then read data from the SDRAM and generate the read data and read clock waveforms. 
//
//==========================================================================================================

module bus_disk_read(
    input wire clock,             // master clock 40 MHz
    input wire reset,             // active high synchronous reset input
    input wire BUS_RD_GATE_L,     // Read gate, when active enables read circuitry
    input wire clkenbl_read_bit,  // enable for disk read clock
    input wire clkenbl_read_data, // enable for disk read data
    input wire clock_pulse,       // clock pulse with proper 160 us width from drive
    input wire data_pulse,        // data pulse with proper 160 us width from drive
    input wire [15:0] dram_readdata, // 16-bit read data from DRAM controller
    input wire Selected_Ready,    // disk contents have been copied from the microSD to the SDRAM & drive selected & ~fault latch
    input wire [7:0] Cylinder_Address, // register that stores the valid cylinder address
    input wire [7:0] preamble2_length, // Preamble2 length, in bits
    input wire [15:0] data_length,     // length of the data field (in bits)
    input wire clkenbl_sector,         // sector enable pulse

    output reg dram_read_enbl_busread, // read enable request to DRAM controller
    output reg BUS_RD_DATA_H,          // Read data pulses
    output reg BUS_RD_CLK_H,           // Read clock pulses
    output reg load_address_busread,   // enable to command the sdram controller to load the address from sector, head select and cylinder
    output reg read_indicator,         // active high signal to drive the RD front panel indicator
    output reg read_selected_ready     // read strobe and selected_ready for command interrupt
);

//============================ Internal Connections ==================================

// state definitions and values for the read state
`define BRST0 3'd0 // 0 - off
`define BRST1 3'd1 // 1 - send Preamble
`define BRST2 3'd2 // 2 - send Sync
//`define BRST3 3'd3 // 3 - send Header
`define BRST4 3'd4 // 4 - send Data & CRC
`define BRST5 3'd5 // 5 - send Postamble
reg [2:0] bus_read_state; // read state machine state variable

// PDP-8  RK8-E  12-bit words, 256 12-bit words + 1 16-bit CRC per sector, 16 sectors: Num_Sectors_is_16 = 1, Word_18bit_Mode = 0
// PDP-11 RK11-D 16-bit words, 256 16-bit words + 1 16-bit CRC per sector, 12 sectors: Num_Sectors_is_16 = 0, Word_18bit_Mode = 0
// PDP-11 RK11-E 18-bit words, 256 16-bit words + 1 16-bit CRC per sector, 12 sectors: Num_Sectors_is_16 = 0, Word_18bit_Mode = 0
// for now, only support the RK8-E and RK11-D modes, which operate at 1.44 Mbps

reg [7:0] bus_read_count; // count bits in a word or Preamble length
reg [3:0] metagate; // de-metastable flops for read gate
reg [15:0] psreg; // parallel-to-serial register, send data LSB first
reg [11:0] wordcount; // counter to keep track of the number of data & CRC words, in 16-bit increments
reg [5:0] read_tick_counter; // counter to produce a visible flicker of the RD indicator

//============================ Start of Code =========================================

always @ (posedge clock)
begin : DISKREAD // block name
  read_selected_ready <= Selected_Ready && metagate[3];

  if(reset==1'b1) begin
    dram_read_enbl_busread <= 1'b0;
    BUS_RD_DATA_H <= 1'b0;
    BUS_RD_CLK_H <= 1'b0;
    load_address_busread <= 1'b0;
    bus_read_state <= `BRST0;
    bus_read_count <= 8'd0;
    metagate <= 4'b0000;
    wordcount <= 12'd0;
    psreg <= 16'd0;
    read_tick_counter <= 0;
  end
  else begin
    //metagate[3] <= #1 metagate[2];
    //metagate[2] <= #1 metagate[1];
    //metagate[1] <= #1 metagate[0];
    //metagate[0] <= #1 ~BUS_RD_GATE_L;
    metagate[3:0] <= {metagate[2:0], ~BUS_RD_GATE_L};
    //bus_read_state <= ~Selected_Ready ? `BRST0 : bus_read_state;

    read_tick_counter <= (metagate[3] && Selected_Ready) ? 20 : (clkenbl_sector ? ((read_tick_counter == 0) ? 0 : read_tick_counter - 1) : read_tick_counter);
    read_indicator <= (read_tick_counter != 0);

    case(bus_read_state)
    `BRST0: begin     // transmission is off, waiting for the read gate
      bus_read_state <= (Selected_Ready && metagate[3] && (clkenbl_read_bit || clkenbl_read_data)) ? `BRST1 : `BRST0;
      bus_read_count <= (Selected_Ready && metagate[3] && (clkenbl_read_bit || clkenbl_read_data)) ? preamble2_length : 8'd0; // ready to send preamble2_length bits of Preamble before Sync
      //bus_read_count <= (Selected_Ready && metagate[3] && (clkenbl_read_bit || clkenbl_read_data)) ? 8'd13 : 8'd0; // use a short Preamble for debugging
      BUS_RD_CLK_H <= 1'b0; // send all-zeros, no clocks, when off
      BUS_RD_DATA_H <= 1'b0;  // send all-zeros, no data pulses, when off
      psreg <= 16'd0;
      wordcount <= 12'd0;
      load_address_busread <= 1'b0;
      dram_read_enbl_busread <= 1'b0; 
     end
    `BRST1: begin     // send Preamble of all 0's
      BUS_RD_CLK_H <= clock_pulse;
      BUS_RD_DATA_H <= 1'b0;  // send all-zeros in the Preamble
      bus_read_count <= clkenbl_read_data ? bus_read_count - 1 : bus_read_count;
      bus_read_state <= metagate[3] ? (((bus_read_count == 8'd1) && clkenbl_read_data) ? `BRST2 : `BRST1) : `BRST0;
      psreg <= 16'd0;
      wordcount <= 12'd0;
      load_address_busread <= (bus_read_count == 8'd1) & clkenbl_read_data; // load sdram address register at end of Preamble
      // first read will occur automatically after the address register is loaded and be ready in time for sending the first word
      dram_read_enbl_busread <= 1'b0;
     end
    `BRST2: begin     // send the Sync bit
      BUS_RD_CLK_H <= clock_pulse;
      BUS_RD_DATA_H <= data_pulse; // send a one for the Sync bit
      bus_read_count <= 8'd16;
      //bus_read_state <= clkenbl_read_data ? `BRST3 : `BRST2;
      bus_read_state <= metagate[3] ? (clkenbl_read_data ? `BRST4 : `BRST2) : `BRST0;
      //psreg <= {5'b00000, Cylinder_Address, 3'b000};
      psreg <= clkenbl_read_data ? dram_readdata : psreg;
      //wordcount <= 12'd0;
      wordcount <= data_length[15:4];
      load_address_busread <= 1'b0;
      dram_read_enbl_busread <= 1'b0; 
     end
    //`BRST3: begin     // send the Header, not sure if it should be MSB-first or LSB-first
      //BUS_RD_CLK_H <= clock_pulse;
      //BUS_RD_DATA_H <= data_pulse & psreg[15]; // send serialized bits from the parallel-to-serial register, MSB first, note: should have been LSB first
      //bus_read_count <= clkenbl_read_data ? ((bus_read_count == 1) ? 8'd15 : bus_read_count - 1) : bus_read_count;
      //bus_read_state <= ((bus_read_count == 8'd0) && clkenbl_read_data) ? `BRST4 : `BRST3;
      //psreg <= clkenbl_read_data ? (bus_read_count == 8'd0 ? dram_readdata : psreg << 1) : psreg; // shifting left for MSB first in the header
      //wordcount <= data_length[15:4];
      //load_address_busread <= 1'b0;
      //dram_read_enbl_busread <= 1'b0; 
     //end
    `BRST4: begin     // send Data & CRC
      BUS_RD_CLK_H <= clock_pulse;
      BUS_RD_DATA_H <= data_pulse & psreg[0]; // send serialized bits from the parallel-to-serial register, LSB first
      bus_read_count <= clkenbl_read_data ? ((bus_read_count == 8'd1) ? 8'd16 : bus_read_count - 1) : bus_read_count;
      // decrement if clkenbl_read_data == 1 and rollover from 0 to 15 if the count is at zero, otherwise hold at the present count
      bus_read_state <= metagate[3] ? (((bus_read_count == 8'd1) && (wordcount == 12'd1) && clkenbl_read_data) ? `BRST5 : `BRST4) : `BRST0;
      psreg <= clkenbl_read_data ? ((bus_read_count == 8'd1) ? dram_readdata : psreg >> 1) : psreg;
      wordcount <= ((bus_read_count == 8'd1) && clkenbl_read_data) ? wordcount - 1 : wordcount;
      load_address_busread <= 1'b0;
      dram_read_enbl_busread <= (bus_read_count == 8'd15) & (wordcount != 12'd1) & clkenbl_read_data; // request the next word from the sdram
     end
    `BRST5: begin     // send Postamble while read gate is active
      BUS_RD_CLK_H <= clock_pulse;
      BUS_RD_DATA_H <= 1'b0; // send all-zeros in the Postamble
      bus_read_count <= 8'd0;
      bus_read_state <= (~metagate[3] & ~clock_pulse) ? `BRST0 : `BRST5; // go off when read gate goes inactive and clock pulse is done
      // this is so we don't chop the clock pulse (make a short clock pulse) if it's active when read gate goes inactive
      psreg <= 16'd0;
      wordcount <= 12'd0;
      load_address_busread <= 1'b0;
      dram_read_enbl_busread <= 1'b0; // request the next word from the sdram
     end
    endcase

  end
end // End of Block DISKREAD

endmodule // End of Module bus_disk_read
