//------------------------------------------------
// Functional definition of the Lattice SB_DFFS
// Created so we can simulate higher level functions in Vivado that instantiate the SB_DFFR
// File Name: SB_IO.v
// Functions: 
//   just a stub function for the bi-directional I/O pad so we can simulate at the top level.
//------------------------------------------------
module SB_IO(
PACKAGE_PIN,  // Registered output
OUTPUT_ENABLE,  // Clock input
D_OUT_0,  // "D" input to the DFF
D_IN_0,   // Asynchronous set input
LATCH_INPUT_VALUE, 
CLOCK_ENABLE,
INPUT_CLK,
OUTPUT_CLK,
D_OUT_1,
D_IN_1
);

parameter PIN_TYPE = 6'b101001,
PULLUP = 1'b0,
NEG_TRIGGER = 1'b0;

//-------------------Input Ports-------------------------------------
input OUTPUT_ENABLE;
input D_OUT_0;
input D_IN_0;
input LATCH_INPUT_VALUE;
input CLOCK_ENABLE;
input INPUT_CLK;
input OUTPUT_CLK;
input D_OUT_1;
input D_IN_1;
//-------------------Output Ports-------------------------------------
output PACKAGE_PIN;

//-------------------Input ports Data Type-------------------------------------
wire OUTPUT_ENABLE;
wire D_OUT_0;
wire D_IN_0;
wire LATCH_INPUT_VALUE;
wire CLOCK_ENABLE;
wire INPUT_CLK;
wire OUTPUT_CLK;
wire D_OUT_1;
wire D_IN_1;

//-------------------Output ports Data Type-------------------------------------
wire PACKAGE_PIN;

//-------------------Code Starts Here-------------------------------------
wire resetn;
wire i1;
wire i2;
wire i3;
wire i4;
wire qn;

assign PACKAGE_PIN = 1'b1;

endmodule // End of Module SB_IO