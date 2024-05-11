//==========================================================================================================
// RK05 Emulator
// Global Clock and Reset Inputs to the chip
// File Name: clock_and_reset.v
// Functions: 
//   Receive and distribute the Clock and Reset signals.
//
//==========================================================================================================

module clock_and_reset(
    input wire clock,   // 40 MHz clock input pin
    input wire pin_reset_n, // active low reset pin

    output reg reset        // synchronized reset to internal modules
);

//============================ Internal Connections ==================================

reg [2:0] metareset;

//============================ Start of Code =========================================

always @ (posedge clock)
begin : DEMETARESET // block name
  reset <= metareset[2];
  metareset[2] <= metareset[1];
  metareset[1] <= metareset[0];
  metareset[0] <= ~pin_reset_n;
end

endmodule // End of Module clock_and_reset
