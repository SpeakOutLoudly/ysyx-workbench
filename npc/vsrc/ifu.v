// 指令存储器子模块，只声明接口，暂不实现。
`include "sub/pc_update.v"
`include "sub/imem.v"
// 取指阶段，包含 PC 更新和指令读取。
module ifu (
  input  wire        clk,
  input  wire        reset,
  input  wire        redirect_valid,
  input  wire [31:0] redirect_pc,
  output reg [31:0] pc,
  output wire [31:0] inst
);

pc_update pu(
    .clk(clk),
    .reset(reset),
    .redirect_valid(redirect_valid),
    .redirect_pc(redirect_pc),
    .pc(pc)
);

imem inst_mem(
    .address(pc),
    .inst(inst)
);

endmodule




