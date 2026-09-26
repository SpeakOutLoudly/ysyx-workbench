// 执行阶段，负责指令计算、比较以及 PC 重定向，包含 ALU。
`include "sub/mux.v"
`include "sub/alu.v"
`include "sub/branch_unit.v"
module ysyx_20230612_exu (
  input  wire [31:0] pc,
  input  wire [31:0] rs1_data,
  input  wire [31:0] rs2_data,
  input  wire [31:0] csr_data,
  input  wire [31:0] imm,
  input  wire [2:0]  funct3,
  input  wire [4:0]  alu_op,
  input  wire [1:0]  op1_sel,
  input  wire        alu_src_imm,
  input  wire        branch,
  input  wire        jal,
  input  wire        jalr,

  output wire [31:0] alu_result,
  output wire [31:0] store_data,
  output wire        redirect_valid,
  output wire [31:0] redirect_pc
);
  wire [31:0] operand_a, operand_b;

  ysyx_20230612_MuxKey #(4, 2, 32) operand_a_mux(
    .out(operand_a),
    .key(op1_sel),
    .lut({2'b00, rs1_data, 2'b01, 32'b0, 2'b10, pc, 2'b11, csr_data})
  );

  // 普通指令用 alu_src_imm 选择 rs2/立即数；CSRRS 用 rs1。
  wire [31:0] normal_operand_b;
  ysyx_20230612_MuxKey #(2, 1, 32) operand_b_mux (
    .out (normal_operand_b),
    .key (alu_src_imm),
    .lut ({1'b1, imm, 1'b0, rs2_data})
  );
  assign operand_b = alu_op[4] ? rs1_data : normal_operand_b;

  ysyx_20230612_alu u_alu (
    .operand_a (operand_a),
    .operand_b (operand_b),
    .alu_op    (alu_op),
    .result    (alu_result)
  );

  // Store 写入内存的数据始终来自 rs2，不经过 ALU 操作数选择器。
  assign store_data = rs2_data;

  ysyx_20230612_branch_unit u_branch_unit (
    .pc             (pc),
    .rs1_data       (rs1_data),
    .rs2_data       (rs2_data),
    .imm            (imm),
    .funct3         (funct3),
    .branch         (branch),
    .jal            (jal),
    .jalr           (jalr),
    .redirect_valid (redirect_valid),
    .redirect_pc    (redirect_pc)
  );
endmodule
