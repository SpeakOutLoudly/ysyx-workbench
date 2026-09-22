// 控制信号译码子模块，只声明接口，暂不实现。
module decoder (
  input  wire [31:0] inst,
  output wire [2:0]  funct3,
  output reg  [2:0]  imm_type,
  output reg  [4:0]  alu_op,
  output reg         alu_src_imm,
  output reg         mem_read,
  output reg         mem_write,
  output reg         reg_write,
  output reg  [1:0]  wb_sel,
  output reg  [1:0]  op1_sel,
  output reg         branch,
  output reg         jal,
  output reg         jalr,
  output wire        ebreak
);
    wire [6:0]opcode;
    //wire [6:0]funct7;

    assign opcode = inst[6:0];
    //assign funct7 = inst[31:25];
    assign funct3 = inst[14:12];
    assign ebreak = (inst == 32'h00100073) ? 1 : 0;
    
    // assign alu_src_imm = (inst[4:2] == 3'b100) ? 1 : 0;
    always @(*) begin
        // 防止出现锁存器
        imm_type   = 3'b000;
        alu_op     = 5'b00000;
        alu_src_imm = 1'b0;
        mem_read   = 1'b0;
        mem_write  = 1'b0;
        reg_write  = 1'b0;
        wb_sel     = 2'b00;
        op1_sel     = 2'b00;
        branch     = 1'b0;
        jal        = 1'b0;
        jalr       = 1'b0;
        case(opcode)
            7'b0010011: begin       // OP-IMM
                alu_op = {1'b0, funct3, ((funct3 == 3'b101) ? inst[30] : 1'b0)};
                reg_write = 1'b1;
                alu_src_imm = 1'b1;
                wb_sel = 2'b00; // 写回 alu结果
            end
            7'b0110011: begin       // OP
                alu_op = {1'b0, funct3, inst[30]};
                reg_write = 1'b1;
            end
            7'b1100011: begin       // B-type
                branch = 1;
                imm_type = 3'b010;
                reg_write = 0;
            end
            7'b1101111: begin       // jal
                jal       = 1;
                imm_type  = 3'b011;
                reg_write = 1;
                wb_sel    = 2'b10;
            end
            7'b1100111: begin       // jalr
                alu_src_imm = 1'b1;
                jalr = 1'b1;
                reg_write = 1'b1;
                wb_sel = 2'b10; // 写回PC+4
            end
            7'b0000011: begin       // Load
                alu_op = 5'b00000;  // 加法
                alu_src_imm = 1'b1;
                mem_read = 1'b1;
                reg_write = 1'b1;
                wb_sel = 2'b01; // 写回读出的内存值
            end
            7'b0100011: begin       // Store
                alu_op = 5'b00000;
                alu_src_imm = 1'b1;
                mem_write = 1'b1;
                imm_type = 3'b001;
            end
            7'b0110111: begin       // U-type lui
                alu_op = 5'b00000;
                alu_src_imm = 1'b1;
                reg_write = 1'b1;
                wb_sel = 2'b00;
                op1_sel = 2'b01;    // op1 为0
                imm_type = 3'b100;
            end
            7'b0010111: begin       // U-type auipc
                alu_op = 5'b00000;
                alu_src_imm = 1'b1;
                reg_write = 1'b1;
                wb_sel = 2'b00;
                op1_sel = 2'b10;    // op1 为pc
                imm_type = 3'b100;
            end
            default: begin          // 未知指令
                alu_op = {1'b0, funct3, 1'b0};
                reg_write = 1'b0;
                alu_src_imm = 1'b1;
                wb_sel = 2'b00; 
            end
        endcase
    end
endmodule
