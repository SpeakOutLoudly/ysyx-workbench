import "DPI-C" function int imem_read(input int address);
module imem (
  input  wire [31:0] address,
  output wire [31:0] inst
);

  assign inst = imem_read(address);
endmodule
