// 立即数扩展子模块，只声明接口，暂不实现。符号位拓展
// imm_type:
// 000 I-type 高12位直接拓展
// 001 S-type inst[31:25] | inst[11:7]
// 010 B-type inst[31] | inst[7] | inst[30:25] | inst[11:8]
// 011 J-type
// 100 U-type
module ysyx_20230612_imm_extender (
  input  wire [31:7] inst,
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
