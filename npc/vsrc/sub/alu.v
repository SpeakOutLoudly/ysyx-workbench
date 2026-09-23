// ALU 子模块。alu_op[4:1] 选择运算，alu_op[0] 区分 ADD/SUB 或 SRL/SRA。
module ysyx_20230612_alu (
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
      4'b0001: result = operand_a << operand_b[4:0]; // SLL/SLLI
      4'b0010: result = {31'b0, $signed(operand_a) < $signed(operand_b)}; // SLT/SLTI
      4'b0011: result = {31'b0, operand_a < operand_b}; // SLTU/SLTIU
      4'b0101: begin // SRL/SRLI 或 SRA/SRAI
        if (alu_op[0])
          result = $signed(operand_a) >>> operand_b[4:0];
        else
          result = operand_a >> operand_b[4:0];
      end
      default: result = 32'b0;
    endcase
  end
endmodule
