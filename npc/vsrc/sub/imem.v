module imem (
  input  wire        reset,
  input  wire        fetch_valid,
  input  wire [31:0] pc,
  output reg  [31:0] inst
);

  always @(*) begin
    if (reset)
      inst = 32'h00000013;
    else if(fetch_valid)
      inst = pmem_read(pc);
    else      // nop 指令，此时不会更新时序逻辑状态，所以无影响
      inst = 32'h00000013;
  end
endmodule
