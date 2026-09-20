// 译码阶段，包含寄存器堆、立即数扩展和控制信号译码。
`include "sub/RegisterFile.v"
`include "sub/decoder.v"
`include "sub/imm_extender.v"
module idu (
  input  wire        clk,
  input  wire [31:0] inst,

  input  wire        wb_we,
  input  wire [4:0]  wb_rd,
  input  wire [31:0] wb_data,

  output wire [31:0] rs1_data,
  output wire [31:0] rs2_data,
  output wire [31:0] imm,
  output wire [4:0]  rd,
  output wire [2:0]  funct3,

  output wire [4:0]  alu_op,
  output wire        alu_src_imm,
  output wire        mem_read,
  output wire        mem_write,
  output wire        reg_write,
  output wire [1:0]  wb_sel,
  output wire [1:0]  op1_sel,
  output wire        branch,
  output wire        jal,
  output wire        jalr,
  output wire        ebreak
);
  // TODO: 实例化并连接 register_file、imm_extender 和 decoder。

  wire [2:0]imm_type;
  assign rd = inst[11:7];
  // 寄存器堆子模块
  RegisterFile #(.ADDR_WIDTH(5), .DATA_WIDTH(32)) regfile(
      .clk(clk),
      .wdata(wb_data),
      .waddr(wb_rd),
      .wen(wb_we),
      .raddr_rs1(inst[19:15]),    // 直接取低4位截断。
      .raddr_rs2(inst[24:20]),
      .rdata_rs1(rs1_data),
      .rdata_rs2(rs2_data)
  );

  imm_extender extender(
    .inst(inst[31:7]),
    .imm_type(imm_type),
    .imm(imm)
  );
  // 控制信号译码子模块，只声明接口，暂不实现。
  decoder de(
      .inst(inst),
      .funct3(funct3),
      .imm_type(imm_type),
      .alu_op(alu_op),
      .alu_src_imm(alu_src_imm),
      .mem_read(mem_read),
      .mem_write(mem_write),
      .reg_write(reg_write),
      .wb_sel(wb_sel),
      .op1_sel(op1_sel),
      .branch(branch),
      .jal(jal),
      .jalr(jalr),
      .ebreak(ebreak)
  );

endmodule
