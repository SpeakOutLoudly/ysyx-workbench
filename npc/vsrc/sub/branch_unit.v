// 分支和跳转子模块，只声明接口，暂不实现。
module ysyx_20230612_branch_unit (
  input  wire [31:0] pc,
  input  wire [31:0] rs1_data,
  input  wire [31:0] rs2_data,
  input  wire [31:0] imm,
  input  wire [2:0]  funct3,
  input  wire        branch,
  input  wire        jal,
  input  wire        jalr,
  output reg         redirect_valid,
  output reg  [31:0] redirect_pc
);
  reg branch_taken;

  // 根据 funct3 判断条件分支是否成立。
  always @(*) begin
    branch_taken = 1'b0;

    case (funct3)
      3'b000: branch_taken = (rs1_data == rs2_data);                  // BEQ
      3'b001: branch_taken = (rs1_data != rs2_data);                  // BNE
      3'b100: branch_taken = ($signed(rs1_data) < $signed(rs2_data)); // BLT
      3'b101: branch_taken = ($signed(rs1_data) >= $signed(rs2_data));// BGE
      3'b110: branch_taken = (rs1_data < rs2_data);                   // BLTU
      3'b111: branch_taken = (rs1_data >= rs2_data);                  // BGEU
      default: branch_taken = 1'b0;
    endcase
  end

  // 普通指令不重定向；跳转和成立的条件分支才修改 PC。
  always @(*) begin
    redirect_valid = 1'b0;
    redirect_pc = 32'b0;

    if (jalr) begin
      redirect_valid = 1'b1;
      redirect_pc = (rs1_data + imm) & 32'hfffffffe;
    end
    else if (jal) begin
      redirect_valid = 1'b1;
      redirect_pc = pc + imm;
    end
    else if (branch && branch_taken) begin
      redirect_valid = 1'b1;
      redirect_pc = pc + imm;
    end
  end
endmodule
