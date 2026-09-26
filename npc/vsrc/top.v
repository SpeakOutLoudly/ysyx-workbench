`include "vsrc/defines.vh"
`include "vsrc/ifu.v"
`include "vsrc/idu.v"
`include "vsrc/exu.v"
`include "vsrc/lsu.v"
`include "vsrc/wbu.v"
// 组合 5个 unit

module ysyx_20230612(
  input  wire        clock,
  input  wire        reset,
  output wire        io_ifu_reqValid,
  output wire [31:0] io_ifu_addr,
  input  wire        io_ifu_respValid,
  input  wire [31:0] io_ifu_rdata,
  output wire        io_lsu_reqValid,
  output wire [31:0] io_lsu_addr,
  output wire [1:0]  io_lsu_size,
  output wire        io_lsu_wen,
  output wire [31:0] io_lsu_wdata,
  output wire [3:0]  io_lsu_wmask,
  input  wire        io_lsu_respValid,
  input  wire [31:0] io_lsu_rdata,
  output wire        ebreak,
  output wire        exec_valid,
  output wire [31:0] debug_pc,
  output wire [31:0] halt_code,
  output wire [31:0] debug_inst
);
  wire [31:0] if_pc;
  wire [31:0] if_inst;
  wire        if_commit_valid;
  wire        ls_req_valid;

  wire [31:0] id_rs1_data;
  wire [31:0] id_rs2_data;
  wire [31:0] id_csr_data;
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

  ysyx_20230612_ifu u_ifu (
    .clk            (clock),
    .reset          (reset),
    .ifu_resp_valid (io_ifu_respValid),
    .ifu_rdata      (io_ifu_rdata),
    .ifu_req_valid  (io_ifu_reqValid),
    .lsu_resp_valid (io_lsu_respValid),
    .redirect_valid (ex_redirect_valid),
    .redirect_pc    (ex_redirect_pc),
    .commit_valid   (if_commit_valid),
    .lsu_req_valid  (ls_req_valid),
    .pc             (if_pc),
    .inst           (if_inst)
  );

  assign io_ifu_addr = if_pc;
  assign io_lsu_reqValid = ls_req_valid;

  ysyx_20230612_idu u_idu (
    .clk         (clock),
    .reset       (reset),
    .inst        (if_inst),
    .commit_valid   (if_commit_valid),
    .wb_we       (wb_we),
    .wb_rd       (wb_rd),
    .wb_data     (wb_data),
    .csr_in      (ex_alu_result),
    .rs1_data    (id_rs1_data),
    .rs2_data    (id_rs2_data),
    .csr_data    (id_csr_data),
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

  ysyx_20230612_exu u_exu (
    .pc             (if_pc),
    .rs1_data       (id_rs1_data),
    .rs2_data       (id_rs2_data),
    .csr_data       (id_csr_data),
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

  ysyx_20230612_lsu u_lsu (
    .address    (ex_alu_result),
    .store_data (ex_store_data),
    .funct3     (id_funct3),
    .mem_read   (id_mem_read),
    .mem_write  (id_mem_write),
    .io_addr    (io_lsu_addr),
    .io_size    (io_lsu_size),
    .io_wen     (io_lsu_wen),
    .io_wdata   (io_lsu_wdata),
    .io_wmask   (io_lsu_wmask),
    .io_rdata   (io_lsu_rdata),
    .load_data  (ls_load_data)
  );

  ysyx_20230612_wbu u_wbu (
    .pc         (if_pc),
    .alu_result (ex_alu_result),
    .load_data  (ls_load_data),
    .csr_data   (id_csr_data),
    .wb_sel     (id_wb_sel),
    .reg_write  (id_reg_write),
    .rd         (id_rd),
    .wb_we      (wb_we),
    .wb_rd      (wb_rd),
    .wb_data    (wb_data)
  );

  assign debug_pc = if_pc;
  assign debug_inst = if_inst;
  assign exec_valid = !reset && (if_commit_valid);

`ifdef SOC_SIM
  // 仿真时只在 ebreak 指令提交的时钟沿结束程序。
  always @(posedge clock) begin
    if (exec_valid && ebreak) begin
      if (halt_code == 32'b0)
        $finish;
      else
        $fatal(1, "HIT BAD TRAP: halt_code=%0d", halt_code);
    end
  end
`endif
endmodule
