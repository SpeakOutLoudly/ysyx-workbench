// 译码阶段，包含寄存器堆、立即数扩展和控制信号译码。
`include "sub/RegisterFile.v"
`include "sub/decoder.v"
`include "sub/imm_extender.v"
module ysyx_20230612_idu (
  input  wire        clk,
  input  wire        reset,
  input  wire [31:0] inst,

  input  wire        commit_valid,
  input  wire        wb_we,
  input  wire [4:0]  wb_rd,
  input  wire [31:0] wb_data,
  input  wire [31:0] csr_in,

  output wire [31:0] rs1_data,
  output wire [31:0] rs2_data,
  output wire [31:0] csr_data,
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
  output wire        ebreak,
  output wire [31:0] halt_code
);
  // TODO: 实例化并连接 register_file、imm_extender 和 decoder。

  wire [2:0]imm_type;
  wire csr_wen;
  assign rd = inst[11:7];
  // 寄存器堆子模块
  ysyx_20230612_RegisterFile #(.ADDR_WIDTH(5), .DATA_WIDTH(32)) regfile(
      .clk(clk),
      .reset(reset),
      .wdata(wb_data),
      .waddr(wb_rd),
      .wen(wb_we),
      .inst_valid(commit_valid),
      .raddr_rs1(inst[19:15]),
      .raddr_rs2(inst[24:20]),
      .csr_wen(csr_wen),
      .csr_addr(inst[31:20]),
      .csr_in(csr_in),
      .csr_out(csr_data),
      .rdata_rs1(rs1_data),
      .rdata_rs2(rs2_data),
      .halt_code(halt_code)
  );

  ysyx_20230612_imm_extender extender(
    .inst(inst[31:7]),
    .imm_type(imm_type),
    .imm(imm)
  );
  // 控制信号译码子模块，只声明接口，暂不实现。
  ysyx_20230612_decoder de(
      .inst(inst),
      .funct3(funct3),
      .imm_type(imm_type),
      .alu_op(alu_op),
      .alu_src_imm(alu_src_imm),
      .mem_read(mem_read),
      .mem_write(mem_write),
      .reg_write(reg_write),
      .csr_wen(csr_wen),
      .wb_sel(wb_sel),
      .op1_sel(op1_sel),
      .branch(branch),
      .jal(jal),
      .jalr(jalr),
      .ebreak(ebreak)
  );

endmodule
