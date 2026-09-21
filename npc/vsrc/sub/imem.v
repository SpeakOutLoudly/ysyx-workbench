`ifndef PMEM_READ
`define PMEM_READ
import "DPI-C" function int pmem_read(input int raddr);
`endif
module imem (
  input  wire        reset,
  input  wire [31:0] address,
  output reg  [31:0] inst
);

  always @(*) begin
    if (reset)
      inst = 32'h00000013;
    else
      inst = pmem_read(address);
  end
endmodule
