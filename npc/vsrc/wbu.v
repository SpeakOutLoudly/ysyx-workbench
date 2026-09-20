// 写回阶段，选择最终结果并生成寄存器写回总线。
`include "sub/mux.v"

module wbu (
  input  wire [31:0] pc,
  input  wire [31:0] alu_result,
  input  wire [31:0] load_data,
  input  wire [1:0]  wb_sel,
  input  wire        reg_write,
  input  wire [4:0]  rd,

  output wire        wb_we,
  output wire [4:0]  wb_rd,
  output wire [31:0] wb_data
);

  wire [31:0] pc_plus_4;

  assign pc_plus_4 = pc + 4;
  assign wb_rd = rd;
  assign wb_we = reg_write;
  
  MuxKey #(3, 2, 32) writeback_mux (
    .out (wb_data),
    .key (wb_sel),
    .lut ({2'b00, alu_result, 2'b01, load_data, 2'b10, pc_plus_4})
  );

endmodule

