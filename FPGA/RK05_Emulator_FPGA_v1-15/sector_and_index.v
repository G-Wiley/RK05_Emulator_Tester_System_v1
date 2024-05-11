//==========================================================================================================
// RK05 Emulator
// sector and index pulse timing generators
// File Name: sector_and_index.v
// Functions: 
//   Divide the 1 microsecond clock to generate a sector pulse enable signal.
//   After number_of_sectors sector pulses, generate an index pulse offset by 600 usec.
//   The Sector interval is defined by the parameter microseconds_per_sector.
//
//==========================================================================================================

module sector_and_index(
    input wire clock,                          // master clock 40 MHz
    input wire reset,                          // active high synchronous reset input
    input wire clkenbl_1usec,                  // 1 usec clock enable input from the timing generator
    input wire [4:0] number_of_sectors,
    input wire [15:0] microseconds_per_sector, // number of microseconds per sector to generate sector timing

    // note that in the present code, clkenbl_sector and clkenbl_index are not connected anywhere in the top level code
    // we could remove them as outputs from sector_and_index.v
    output reg clkenbl_sector,      // enable for disk read clock
    output reg clkenbl_index,       // enable for disk read data
    output reg bus_sector_pulse,    // active-high 2 usec sector pulse
    output reg bus_index_pulse,     // active-high 2 usec index pulse
    output reg [4:0] Sector_Address //counter that specifies which sector is present "under the heads"
);

//============================ Internal Connections ==================================

reg [15:0] int_usec_in_sector;

//============================ Start of Code =========================================

always @ (posedge clock)
begin : SECTORCOUNTERS // block name
  if(reset == 1'b1) begin
    int_usec_in_sector <= 16'd1;
    Sector_Address <= 5'd0;
    clkenbl_sector <= 1'b0;
    clkenbl_index <=  1'b0;
    bus_sector_pulse <= 1'b0;
    bus_index_pulse <=  1'b0;
  end
  else begin
    // assert the index enable 600 usec into the last sector
    clkenbl_index <=    ((int_usec_in_sector == 16'd600) && (Sector_Address == (number_of_sectors - 1)) && clkenbl_1usec);
    // Q <= (S | Q) & ~R;
    bus_index_pulse <= (((int_usec_in_sector == 16'd600) && (Sector_Address == (number_of_sectors - 1)) && clkenbl_1usec) | bus_index_pulse) & ~((int_usec_in_sector == 16'd602) && clkenbl_1usec);

    // at the end of the sector: assert the sector enable, wrap the usec in sector counter, increment the sector address
    clkenbl_sector <= ((int_usec_in_sector == microseconds_per_sector) && clkenbl_1usec);
    // Q <= (S | Q) & ~R;
    bus_sector_pulse <= (((int_usec_in_sector == microseconds_per_sector) && clkenbl_1usec) | bus_sector_pulse) & ~((int_usec_in_sector == 16'd2) && clkenbl_1usec);

    int_usec_in_sector <= clkenbl_1usec ? ((int_usec_in_sector == microseconds_per_sector) ? 16'd1 : int_usec_in_sector + 1) : int_usec_in_sector;
    Sector_Address <= ((int_usec_in_sector == microseconds_per_sector) && clkenbl_1usec) ? ((Sector_Address == (number_of_sectors - 1)) ? 5'd0 : Sector_Address + 1) : Sector_Address;
  end
end // End of Block COUNTERS

endmodule // End of Module sector_and_index
