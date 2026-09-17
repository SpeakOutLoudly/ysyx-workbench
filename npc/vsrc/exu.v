// 执行阶段，负责指令计算、比较以及 PC 重定向，包含 ALU。
`include "sub/mux.v"

module exu (
  input  wire [31:0] pc,
  input  wire [31:0] rs1_data,
  input  wire [31:0] rs2_data,
  input  wire [31:0] imm,
  input  wire [2:0]  funct3,
  input  wire [4:0]  alu_op,
  input  wire        alu_src_imm,
  input  wire        branch,
  input  wire        jal,
  input  wire        jalr,

  output wire [31:0] alu_result,
  output wire [31:0] store_data,
  output wire        redirect_valid,
  output wire [31:0] redirect_pc
);
  // TODO: 实例化并连接 alu 和 branch_unit。
endmodule

// ALU 子模块。alu_op[4:1] 选择运算，alu_op[0] 区分 ADD/SUB。
module alu (
  input  wire [31:0] operand_a,
  input  wire [31:0] operand_b,
  input  wire [4:0]  alu_op,
  output reg  [31:0] result
);
  always @(*) begin
    result = 32'b0;

    case (alu_op[4:1])
      4'b0000: begin
        if (alu_op[0]) begin
          result = operand_a - operand_b; // SUB
        end
        else begin
          result = operand_a + operand_b; // ADD
        end
      end
      4'b0100: result = operand_a ^ operand_b; // XOR
      4'b0110: result = operand_a | operand_b; // OR
      4'b0111: result = operand_a & operand_b; // AND
      default: result = 32'b0;
    endcase
  end
endmodule

// 分支和跳转子模块，只声明接口，暂不实现。
module branch_unit (
  input  wire [31:0] pc,
  input  wire [31:0] rs1_data,
  input  wire [31:0] rs2_data,
  input  wire [31:0] imm,
  input  wire [2:0]  funct3,
  input  wire        branch,
  input  wire        jal,
  input  wire        jalr,
  output wire        redirect_valid,
  output wire [31:0] redirect_pc
);
endmodule
