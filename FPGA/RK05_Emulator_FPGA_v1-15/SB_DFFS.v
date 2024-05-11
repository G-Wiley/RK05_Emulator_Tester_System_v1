//------------------------------------------------
// Functional definition of the Lattice SB_DFFS
// Created so we can simulate higher level functions in Vivado that instantiate the SB_DFFR
// File Name: SB_DFFR.v
// Functions: 
//   just a D flip flop with asynchronous Set, built from 3-input NAND gates.
//   The logic function is copied from the TI Data Book SN7474 logic diagram.
//------------------------------------------------
module SB_DFFS(
Q,  // Registered output
C,  // Clock input
D,  // "D" input to the DFF
S   // Asynchronous set input
);

//-------------------Input Ports-------------------------------------
input C;
input D;
input S;

//-------------------Output Ports-------------------------------------
output Q;

//-------------------Input ports Data Type-------------------------------------
wire C;
wire D;
wire S;

//-------------------Output ports Data Type-------------------------------------
wire Q;

//-------------------Code Starts Here-------------------------------------
wire resetn;
wire i1;
wire i2;
wire i3;
wire i4;
wire qn;

assign resetn = 1'b1;
assign #1 i1 = ~(~S & i4 & i2);
assign #1 i2 = ~(i1 & resetn & C);
assign #1 i3 = ~(i2 & C & i4);
assign #1 i4 = ~(i3 & resetn & D);
assign #1 Q =  ~(~S & i2 & qn);
assign #1 qn = ~(Q & resetn & i3);

endmodule // End of Module SB_DFFS