//==========================================================================================================
// RK05 Emulator
// seek to cylinder logic
// File Name: seek_to_cylinder.v
// Functions: 
//   Receive the bus strobe, bus cylinder address, and bus restore signals.
//   Seek to the cylinder address if the address is valid (less than 203),
//   by saving the cylinder address in Cylinder_Address.
//   Respond with bus address accepted, bus address invalid, bus RWS ready.
//
//==========================================================================================================

module seek_to_cylinder(
    input wire clock,             // master clock 40 MHz
    input wire reset,             // active high synchronous reset input
    input wire Selected_Ready,    // disk contents have been copied from the microSD to the SDRAM & drive selected & ~fault latch
    input wire BUS_STROBE_L,      // strobe to enable movement of the heads, gates cyl addr and restore signals
    input wire [7:0] BUS_CYL_ADD_L, // cylinder address 8-bit bus
    input wire BUS_RESTORE_L,     // restore, moves heads to cylinder zero
    input wire BUS_HEAD_SELECT_L, // head selection, upper or lower
    input wire clkenbl_index,     // index enable pulse

    output reg [7:0] Cylinder_Address, // internal register to store the valid cylinder address
    output reg Head_Select,            // internal register to store the head selection (upper or lower)
    output reg BUS_ADDRESS_ACCEPTED_H, // cylinder address accepted pulse
    output reg BUS_ADDRESS_INVALID_H,  // address invalid pulse
    //output reg BUS_SEEK_INCOMPLETE_H,  // seek incomplete signal, not used, always inactive
    output reg BUS_RWS_RDY_H,          // read/write/seek ready signal
    output reg oncylinder_indicator,   // active high signal to drive the On Cylinder front panel indicator
    output reg strobe_selected_ready   // synchronized strobe and selected_ready for command interrupt
);

//============================ Internal Connections ==================================
`define RWS_READY_DURATION 24'd255

reg [3:0] meta_bus_strobe;  // sampling and metastability reduction of Bus Strobe
reg [2:0] meta_head_select; // sampling and metastability reduction of Head Select
reg [7:0] addr_resp;        // counter to provide the proper pulse width of Address Accepted or Address Invalid
reg [2:0] on_cyl_counter;   // counter to produce a visible flicker of the On Cylinder indicator
wire BUS_RESTORE;           // active high bus restore signal so the equations below are more clear

//============================ Start of Code =========================================

assign BUS_RESTORE = ~BUS_RESTORE_L;    // make the active high internal signal Bus Restore
// BUS_RESTORE_L is stable prior to BUS_STROBE_L being active so we don't worry about metastability on BUS_RESTORE_L
always @ (posedge clock)
begin
    //BUS_SEEK_INCOMPLETE_H <= 1'b0;
    strobe_selected_ready <= meta_bus_strobe[3] && Selected_Ready;

    if(reset == 1'b1) begin
        Cylinder_Address <= 8'd0;
        Head_Select <= 1'b0;
        BUS_ADDRESS_ACCEPTED_H <= 1'b0;
        BUS_ADDRESS_INVALID_H <= 1'b0;
        meta_bus_strobe[3:0] <= 4'h0;
        meta_head_select[2:0] <= 3'h0;
        addr_resp <= 8'd0;
        BUS_RWS_RDY_H <= 1'b1;
        on_cyl_counter <= 0;
    end
    else begin
        BUS_RWS_RDY_H <= 1'b1;
        
        meta_bus_strobe[3:0] <= {meta_bus_strobe[2:0], ~BUS_STROBE_L};
        meta_head_select[2:0] <= {meta_head_select[1:0], ~BUS_HEAD_SELECT_L};
        Head_Select <= meta_head_select[2];

        Cylinder_Address <= (meta_bus_strobe[2] && ~meta_bus_strobe[3] && Selected_Ready) ? 
            (BUS_RESTORE ? 8'd0 : ((~BUS_CYL_ADD_L < 8'd203) ? ~BUS_CYL_ADD_L : Cylinder_Address)) : Cylinder_Address;

        on_cyl_counter <= BUS_ADDRESS_ACCEPTED_H ? 2 : (clkenbl_index ? ((on_cyl_counter == 0) ? 0 : on_cyl_counter - 1) : on_cyl_counter);
        oncylinder_indicator <= (on_cyl_counter == 0);

        addr_resp <= (meta_bus_strobe[2] && ~meta_bus_strobe[3] && Selected_Ready) ? 8'd200 : ((addr_resp == 8'd0) ? 8'd0 : addr_resp - 1);
 
        BUS_ADDRESS_ACCEPTED_H <= (addr_resp != 8'd0) && (BUS_RESTORE || (~BUS_CYL_ADD_L < 8'd203));
        BUS_ADDRESS_INVALID_H <= (addr_resp != 8'd0) && ~BUS_RESTORE && (~BUS_CYL_ADD_L > 8'd202);
    end
end

endmodule // End of Module sector_and_index
