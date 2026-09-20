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
