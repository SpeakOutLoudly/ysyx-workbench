`include "vsrc/ifu.v"
`include "vsrc/idu.v"
`include "vsrc/exu.v"
`include "vsrc/lsu.v"
`include "vsrc/wbu.v"
// 组合 5个 unit

module top(
  input  wire        clk,
  input  wire        reset,
  output wire        ebreak,
  output wire [31:0] debug_pc,
  output wire [31:0] halt_code,
  output wire [31:0] debug_inst
);
  wire [31:0] if_pc;
  wire [31:0] if_inst;

  wire [31:0] id_rs1_data;
  wire [31:0] id_rs2_data;
  wire [31:0] id_imm;
  wire [4:0]  id_rd;
  wire [2:0]  id_funct3;
  wire [4:0]  id_alu_op;
  wire        id_alu_src_imm;
  wire        id_mem_read;
  wire        id_mem_write;
  wire        id_reg_write;
  wire [1:0]  id_wb_sel;
  wire [1:0]  id_op1_sel;
  wire        id_branch;
  wire        id_jal;
  wire        id_jalr;

  wire [31:0] ex_alu_result;
  wire [31:0] ex_store_data;
  wire        ex_redirect_valid;
  wire [31:0] ex_redirect_pc;

  wire [31:0] ls_load_data;

  wire        wb_we;
  wire [4:0]  wb_rd;
  wire [31:0] wb_data;

  ifu u_ifu (
    .clk            (clk),
    .reset          (reset),
    .redirect_valid (ex_redirect_valid),
    .redirect_pc    (ex_redirect_pc),
    .pc             (if_pc),
    .inst           (if_inst)
  );

  idu u_idu (
    .clk         (clk),
    .inst        (if_inst),
    .wb_we       (wb_we),
    .wb_rd       (wb_rd),
    .wb_data     (wb_data),
    .rs1_data    (id_rs1_data),
    .rs2_data    (id_rs2_data),
    .imm         (id_imm),
    .rd          (id_rd),
    .funct3      (id_funct3),
    .alu_op      (id_alu_op),
    .alu_src_imm (id_alu_src_imm),
    .mem_read    (id_mem_read),
    .mem_write   (id_mem_write),
    .reg_write   (id_reg_write),
    .wb_sel      (id_wb_sel),
    .op1_sel     (id_op1_sel),
    .branch      (id_branch),
    .jal         (id_jal),
    .jalr        (id_jalr),
    .ebreak      (ebreak),
    .halt_code   (halt_code)
  );

  exu u_exu (
    .pc             (if_pc),
    .rs1_data       (id_rs1_data),
    .rs2_data       (id_rs2_data),
    .imm            (id_imm),
    .funct3         (id_funct3),
    .alu_op         (id_alu_op),
    .op1_sel        (id_op1_sel),
    .alu_src_imm    (id_alu_src_imm),
    .branch         (id_branch),
    .jal            (id_jal),
    .jalr           (id_jalr),
    .alu_result     (ex_alu_result),
    .store_data     (ex_store_data),
    .redirect_valid (ex_redirect_valid),
    .redirect_pc    (ex_redirect_pc)
  );

  lsu u_lsu (
    .address    (ex_alu_result),
    .store_data (ex_store_data),
    .funct3     (id_funct3),
    .mem_read   (id_mem_read),
    .mem_write  (id_mem_write),
    .load_data  (ls_load_data)
  );

  wbu u_wbu (
    .pc         (if_pc),
    .alu_result (ex_alu_result),
    .load_data  (ls_load_data),
    .wb_sel     (id_wb_sel),
    .reg_write  (id_reg_write),
    .rd         (id_rd),
    .wb_we      (wb_we),
    .wb_rd      (wb_rd),
    .wb_data    (wb_data)
  );

  assign debug_pc = if_pc;
  assign debug_inst = if_inst;
endmodule
