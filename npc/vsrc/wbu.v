// 写回阶段，选择最终结果并生成寄存器写回总线。
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
  // TODO: 实例化并连接 writeback_mux。
endmodule

// 写回数据选择子模块，只声明接口，暂不实现。
module writeback_mux (
  input  wire [31:0] pc_plus_4,
  input  wire [31:0] alu_result,
  input  wire [31:0] load_data,
  input  wire [1:0]  wb_sel,
  output wire [31:0] wb_data
);
endmodule
