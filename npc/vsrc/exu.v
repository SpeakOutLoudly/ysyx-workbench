// 执行阶段，负责指令计算、比较以及 PC 重定向，包含 ALU。
`include "sub/mux.v"

module exu (
  input  wire [31:0] pc,
  input  wire [31:0] rs1_data,
  input  wire [31:0] rs2_data,
  input  wire [31:0] imm,
  input  wire [2:0]  funct3,
  input  wire [4:0]  alu_op,
  input  wire [1:0]  op1_sel,
  input  wire        alu_src_imm,
  input  wire        branch,
  input  wire        jal,
  input  wire        jalr,

  output wire [31:0] alu_result,
  output wire [31:0] store_data,
  output wire        redirect_valid,
  output wire [31:0] redirect_pc
);
  wire [31:0] operand_a, operand_b;

  MuxKey #(3, 2, 32) operand_a_mux(
    .out(operand_a),
    .key(op1_sel),
    .lut({2'b00, rs1_data, 2'b01, 32'b0, 2'b10, pc})
  );

  // alu_src_imm = 0 时选择 rs2，等于 1 时选择立即数。
  MuxKey #(2, 1, 32) operand_b_mux (
    .out (operand_b),
    .key (alu_src_imm),
    .lut ({1'b1, imm, 1'b0, rs2_data})
  );

  alu u_alu (
    .operand_a (operand_a),
    .operand_b (operand_b),
    .alu_op    (alu_op),
    .result    (alu_result)
  );

  // Store 写入内存的数据始终来自 rs2，不经过 ALU 操作数选择器。
  assign store_data = rs2_data;

  branch_unit u_branch_unit (
    .pc             (pc),
    .rs1_data       (rs1_data),
    .rs2_data       (rs2_data),
    .imm            (imm),
    .funct3         (funct3),
    .branch         (branch),
    .jal            (jal),
    .jalr           (jalr),
    .redirect_valid (redirect_valid),
    .redirect_pc    (redirect_pc)
  );
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
