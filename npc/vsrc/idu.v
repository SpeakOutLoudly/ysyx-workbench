// 译码阶段，包含寄存器堆、立即数扩展和控制信号译码。
`include "sub/regs.v"
`include "sub/decoder.v"
module idu (
  input  wire        clk,
  input  wire        reset,
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
  // TODO: 实例化并连接 register_file、immediate_extender 和 decoder。

  wire [2:0]imm_type;
  assign rd = inst[11:7];
  // 寄存器堆子模块
  RegisterFile #(.ADDR_WIDTH(4), .DATA_WIDTH(32)) regfile(
      .clk(clk),
      .wdata(wb_data),
      .waddr(wb_rd),
      .wen(wb_we),
      .raddr_rs1(inst[18:15]),    // 直接取低4位截断。
      .raddr_rs2(inst[23:20]),
      .rdata_rs1(rs1_data),
      .rdata_rs2(rs2_data)
  );

  immediate_extender extender(
    .inst(inst),
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



// 立即数扩展子模块，只声明接口，暂不实现。符号位拓展
// imm_type:
// 000 I-type 高12位直接拓展
// 001 S-type inst[31:25] | inst[11:7]
// 010 B-type inst[31] | inst[7] | inst[30:25] | inst[11:8]
// 011 J-type
// 100 U-type
module immediate_extender (
  input  wire [31:0] inst,
  input  wire [2:0]  imm_type,
  output reg  [31:0] imm
);

  always @(*) begin
    case (imm_type)
      3'b000: begin // I-type: imm[11:0] = inst[31:20]
        imm = {{20{inst[31]}}, inst[31:20]};
      end
      3'b001: begin // S-type: imm[11:5] | imm[4:0]
        imm = {{20{inst[31]}}, inst[31:25], inst[11:7]};
      end
      3'b010: begin // B-type: imm[12|10:5|4:1|11|0]
        imm = {{19{inst[31]}}, inst[31], inst[7], inst[30:25],
               inst[11:8], 1'b0};
      end
      3'b011: begin // J-type: imm[20|10:1|11|19:12|0]
        imm = {{11{inst[31]}}, inst[31], inst[19:12], inst[20],
               inst[30:21], 1'b0};
      end
      3'b100: begin // U-type: imm[31:12] | 12'b0
        imm = {inst[31:12], 12'b0};
      end
      default: begin
        imm = 32'b0;
      end
    endcase
  end
endmodule
