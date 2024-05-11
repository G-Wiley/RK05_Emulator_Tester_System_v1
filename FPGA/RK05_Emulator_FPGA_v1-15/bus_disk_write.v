//==========================================================================================================
// RK05 Emulator
// write disk from the BUS
// File Name: bus_disk_write.v
// Functions: 
//   emulates writing to the disk from the interface bus.
//   When BUS_WT_GATE_L goes active (low) then extract serial data from the BUS_WT_DATA_CLK_L and write it to the SDRAM. 
//
//==========================================================================================================

module bus_disk_write(
    input wire clock,              // master clock 40 MHz
    input wire reset,              // active high synchronous reset input
    input wire BUS_WT_GATE_L,      // Write gate, when active enables write circuitry
    input wire BUS_WT_DATA_CLK_L,  // Composite write data and write clock
    input wire Selected_Ready,     // disk contents have been copied from the microSD to the SDRAM & drive selected & ~fault latch
    input wire [15:0] data_length, // length of the data field (in bits)
    input wire clkenbl_sector,     // sector enable pulse

    output reg dram_write_enbl_buswrite,       // read enable request to DRAM controller
    output reg [15:0] dram_writedata_buswrite, // 16-bit write data to DRAM controller
    output reg load_address_buswrite,          // enable to command the sdram controller to load the address from sector, head select and cylinder
    output reg write_indicator,                // active high signal to drive the WT front panel indicator
    output wire [7:0] bdw_debug,              // for debugging, visibility to module signals at the top level
    output reg write_selected_ready           // for command interrupt
    //output wire [2:0] bdw_state_output
); // End of port list

//============================ Internal Connections ==================================

// state definitions and values for the read state
`define BWST0 3'd0 // 0 - off
`define BWST1 3'd1 // 1 - receive Preamble and Sync
//`define BWST2 3'd2 // 2 - receive Header
`define BWST3 3'd3 // 3 - receive Data & CRC
`define BWST4 3'd4 // 4 - receive Postamble

// PDP-8  RK8-E  12-bit words, 256 12-bit words + 1 16-bit CRC per sector, 16 sectors: Num_Sectors_is_16 = 1, Word_18bit_Mode = 0
// PDP-11 RK11-D 16-bit words, 256 16-bit words + 1 16-bit CRC per sector, 12 sectors: Num_Sectors_is_16 = 0, Word_18bit_Mode = 0
// PDP-11 RK11-E 18-bit words, 256 16-bit words + 1 16-bit CRC per sector, 12 sectors: Num_Sectors_is_16 = 0, Word_18bit_Mode = 0
// for now, only support the RK8-E and RK11-D modes, which operate at 1.44 Mbps

reg [2:0] bus_write_state; // read state machine state variable
reg [3:0] bus_write_count; // count bits in a word
reg [11:0] wordcount; // counter to keep track of the number of data & CRC words, in 16-bit increments
reg [15:0] sp_reg; // parallel-to-serial register, receive data LSB first
//reg clkenbl_write_bit;  // enable for disk write clock
reg [4:0] datsep_count;    // data separator counter
reg catch_one;       // latch the data pulse if it happens
reg [3:0] metawrgate; // de-metastable flops for write gate
reg [3:0] metaclkdata; // de-metastable flops for composite clock and data
reg [5:0] write_tick_counter; // counter to produce a visible flicker of the WT indicator
reg [5:0] wt_gate_delay_counter; // counter to delay Write Gate to get past the RK8E glitch on BUS_WT_DATA_CLK_L before looking for the Sync bit
// the RK8E outputs a glitch on BUS_WT_DATA_CLK_L at the time when BUS_WT_GATE_L goes active (low)
// This glitch is about half a bit time before the first real BUS_WT_DATA_CLK_L clock pulse, which sometimes can appear to be a Sync Bit.
// We want to ignore events on BUS_WT_DATA_CLK_L for a short time after BUS_WT_GATE_L goes active to avoid falsely detecting Sync.
reg write_gate_safe; // synchronous signal that indicates Write Gate is active, and safely delayed from the glitch

//============================ Start of Code =========================================

//assign bdw_debug[15:0] = sp_reg[15:0];
assign bdw_debug[7:0] = {5'd0, bus_write_state[2:0]};

//assign bdw_state_output[2:0] = bus_write_state[2:0];

always @ (posedge clock)
begin : DISKWRITE // block name
  //write_selected_ready = metawrgate[3] && Selected_Ready;
  write_selected_ready <= write_gate_safe && Selected_Ready;

  if(reset==1'b1) begin
    bus_write_state <= `BWST0;
    dram_write_enbl_buswrite <= 1'b0;
    dram_writedata_buswrite <= 16'd0;
    load_address_buswrite <= 1'b0;
    bus_write_count <= 4'd0;
    wordcount <= 12'd0;
    sp_reg <= 16'd0;
    //clkenbl_write_bit <= 1'b0;
    datsep_count <= 5'd0;
    catch_one <= 1'b0;
    metawrgate <= 4'b0000;
    metaclkdata <= 4'b0000;
    write_tick_counter <= 0;
    wt_gate_delay_counter <= 6'd0;
    write_gate_safe <= 0;
  end
  else begin
    metawrgate[3:0] <= {metawrgate[2:0], ~BUS_WT_GATE_L};
    metaclkdata[3:0] <= {metaclkdata[2:0], ~BUS_WT_DATA_CLK_L};

    wt_gate_delay_counter <= metawrgate[3] ? (wt_gate_delay_counter == 6'd63 ? 6'd63 : wt_gate_delay_counter + 4'd1) : 4'd0;
    write_gate_safe <= (wt_gate_delay_counter == 6'd63) & metawrgate[3];

    //write_tick_counter <= (metawrgate[3] && Selected_Ready) ? 20 : (clkenbl_sector ? ((write_tick_counter == 0) ? 0 : write_tick_counter - 1) : write_tick_counter);
    write_tick_counter <= (write_gate_safe && Selected_Ready) ? 20 : (clkenbl_sector ? ((write_tick_counter == 0) ? 0 : write_tick_counter - 1) : write_tick_counter);
    write_indicator <= (write_tick_counter != 0);

    // Data Separator Counter: counts down, nominally from 27 to 0. Used for finding the edge and center of bit intervals. Locks on to transitions on BUS_WT_DATA_CLK_L
    // If count is less than 4 then the pulse is early and it is a clock pulse so set the count to 27.
    // If the pulse is late then the count holds at zero until the clock pulse arrives, at which time the count is set to 27.
    datsep_count <= (metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4)) ? 5'd27 : (datsep_count != 0 ? datsep_count - 1 : 5'd0);
    //datsep_count <= reset ? 5'd0 : ((metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4)) ? 5'd27 : (datsep_count != 0 ? datsep_count - 1 : 5'd0)); // reset not necessary here
    
    // If the pulse appears in the center of the bit interval, between counts of 7 and 19, then catch_one is set.
    // If the pulse appears outside of the 7 to 19 window then reset catch_one.
    catch_one <= metaclkdata[2] & ~metaclkdata[3] ? (datsep_count > 7) && (datsep_count < 19) : catch_one;
    
    // When the pulse is a clock pulse then shift the captured data bit into bit 15 of the serial-to-parallel converter register
    sp_reg[15:0] <= metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4) ?  {catch_one, sp_reg[15:1]} : sp_reg[15:0];
    //sp_reg <= metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4) ?  sp_reg >> 1 : sp_reg;
    //sp_reg[15] <= metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4) ? catch_one : sp_reg[15]; 

    case(bus_write_state)
    `BWST0: begin     // 0 - reception is off, waiting for the write gate
      //bus_write_state <= (Selected_Ready & metawrgate[3]) ? `BWST1 : `BWST0;
      bus_write_state <= (Selected_Ready & write_gate_safe) ? `BWST1 : `BWST0;
      dram_write_enbl_buswrite <= 1'b0;
      dram_writedata_buswrite <= 16'd0;
      load_address_buswrite <= 1'b0;
      bus_write_count <= 4'd0; // set to zero, not used until BWST1
      wordcount <= 12'd0; // set to zero, not used until BWST1
      //clkenbl_write_bit <= 1'b0;
     end
    `BWST1: begin     // 1 - receive Preamble and Sync
      //bus_write_state <= metaclkdata[2] & ~metaclkdata[3] & catch_one ? `BWST2 : `BWST1;
      //bus_write_state <= metawrgate[3] ? (metaclkdata[2] & ~metaclkdata[3] & catch_one ? `BWST3 : `BWST1) : `BWST0;
      bus_write_state <= write_gate_safe ? (metaclkdata[2] & ~metaclkdata[3] & catch_one ? `BWST3 : `BWST1) : `BWST0;
      dram_write_enbl_buswrite <= 1'b0;
      dram_writedata_buswrite <= 16'd0;
      load_address_buswrite <= metaclkdata[2] & ~metaclkdata[3] & catch_one;
      bus_write_count <= metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4) & catch_one ? 4'd15 : 4'd0; 
      //wordcount <= 12'd0; // set to zero, not used until BWST2
      wordcount <= data_length[15:4]; // set to the number of words to be transferred, which is data_length/16
      //clkenbl_write_bit <= metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4);
     end
    //`BWST2: begin     // 2 - receive Header
      //bus_write_state <= metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4) & (bus_write_count == 0) ? `BWST3 : `BWST2;
      //dram_write_enbl_buswrite <= 1'b0;
      //dram_writedata_buswrite <= 16'd0;
      //load_address_buswrite <= 1'b0;
      //bus_write_count <= metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4) ? (bus_write_count == 0 ? 4'd15 : bus_write_count - 1): bus_write_count;
      //wordcount <= data_length[15:4];
      //clkenbl_write_bit <= metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4);
     //end
    `BWST3: begin     // 3 - receive Header, Data & CRC
      //bus_write_state <= metawrgate[3] ? (metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4) & (bus_write_count == 0) & (wordcount == 0) ? `BWST4 : `BWST3) : `BWST0;
      bus_write_state <= write_gate_safe ? (metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4) & (bus_write_count == 0) & (wordcount == 0) ? `BWST4 : `BWST3) : `BWST0;
      dram_write_enbl_buswrite <= metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4) & (bus_write_count == 0);
      dram_writedata_buswrite <= dram_write_enbl_buswrite ? sp_reg : dram_writedata_buswrite;
      load_address_buswrite <= 1'b0;
      bus_write_count <= metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4) ? (bus_write_count == 0 ? 4'd15 : bus_write_count - 1): bus_write_count;
      wordcount <= metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4) & (bus_write_count == 0) ? wordcount - 1: wordcount;
      //clkenbl_write_bit <= metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4);
     end
    `BWST4: begin     // 4 - receive Postamble
      //bus_write_state <= metawrgate[3] ? `BWST4 : `BWST0;
      bus_write_state <= write_gate_safe ? `BWST4 : `BWST0;
      dram_write_enbl_buswrite <= 1'b0;
      dram_writedata_buswrite <= dram_write_enbl_buswrite ? sp_reg : dram_writedata_buswrite;
      load_address_buswrite <= 1'b0;
      bus_write_count <= metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4) ? (bus_write_count == 0 ? 4'd15 : bus_write_count - 1): bus_write_count;
      wordcount <= 12'd0;
      //clkenbl_write_bit <= metaclkdata[2] & ~metaclkdata[3] & (datsep_count < 4);
     end
    endcase

  end
end // End of Block DISKREAD

endmodule // End of Module bus_disk_read
