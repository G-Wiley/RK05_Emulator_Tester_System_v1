//==========================================================================================================
// RK05 Emulator
// Drive Select Logic
// File Name: drive_select.v
// Functions: 
//   Activate the global Selected signal based on: BUS_RK11D_L, BUS_SEL_DR_L[3:0], and Drive_Address[2:0].
//   The BUS_SEL_DR_L[3:0] are individual select signals (one-hot) when BUS_RK11D-L is high (inactive).
//   The BUS_SEL_DR_L[2:0] are 3-bit binary encoded drive address signals when BUS_RK11D-L is low (active).
//
//==========================================================================================================

module drive_select(
    input wire BUS_RK11D_L,         // identifies as RK11-D controller with binary address selection, External Bus
    input wire [3:0] BUS_SEL_DR_L,  // 4 bits that are either individual select inputs or binary encoded drive address, External Bus
    input wire [2:0] Drive_Address, // 3 bits, internal signals that specify the drive address
    input wire RK05F_Mode,          // indicates that the drive is in RK05F fixed disk mode with 406 cylinders

    output wire Selected            // active high output enable signal indicates that the drive is selected
);

//============================ Internal Connections ==================================

wire [2:0] bus_addr;
wire valid_addr;
wire RK11;

//============================ Start of Code =========================================

assign RK11 = ~BUS_RK11D_L;
assign bus_addr[2] = RK11 & ~BUS_SEL_DR_L[2];
assign bus_addr[1] = (RK11 & ~BUS_SEL_DR_L[1]) | (~RK11 & (~BUS_SEL_DR_L[3] | ~BUS_SEL_DR_L[2]));
assign bus_addr[0] = (RK11 & ~BUS_SEL_DR_L[0]) | (~RK11 & (~BUS_SEL_DR_L[3] | ~BUS_SEL_DR_L[1]));
assign valid_addr = RK11 | (~RK11 & ~BUS_SEL_DR_L[0] & BUS_SEL_DR_L[1]  & BUS_SEL_DR_L[2]  & BUS_SEL_DR_L[3])
                         | (~RK11 & BUS_SEL_DR_L[0]  & ~BUS_SEL_DR_L[1] & BUS_SEL_DR_L[2]  & BUS_SEL_DR_L[3])
                         | (~RK11 & BUS_SEL_DR_L[0]  & BUS_SEL_DR_L[1]  & ~BUS_SEL_DR_L[2] & BUS_SEL_DR_L[3])
                         | (~RK11 & BUS_SEL_DR_L[0]  & BUS_SEL_DR_L[1]  & BUS_SEL_DR_L[2]  & ~BUS_SEL_DR_L[3]);
assign Selected = valid_addr & (bus_addr == Drive_Address);
//assign Selected = 1'b1; // ************ for debug ************************

endmodule // End of Module drive_select
