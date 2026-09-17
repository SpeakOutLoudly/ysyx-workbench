// 取指阶段，包含 PC 更新和指令读取。
module ifu (
  input  wire        clk,
  input  wire        reset,
  input  wire        redirect_valid,
  input  wire [31:0] redirect_pc,
  output wire [31:0] pc,
  output wire [31:0] inst
);
endmodule

pc_update pu(
    .clk(clk),
    .reset(reset),
    .redirect_valid(redirect_valid),
    .redirect_pc(redirect_pc),
    .pc(pc)
);

instruction_memory inst_mem(
    .address(pc),
    .inst(inst)
);

// PC 更新子模块，只声明接口，暂不实现。
module pc_update (
  input  wire        clk,
  input  wire        reset,
  input  wire        redirect_valid,
  input  wire [31:0] redirect_pc,
  output wire [31:0] pc
);

  always @(posedge clk) begin
        if(reset) begin
            pc <= 0;
        end
        else begin
            if(redirect_valid)
                pc <= redirect_pc;
            else
                pc <= pc + 4;
        end
  end

endmodule

// 指令存储器子模块，只声明接口，暂不实现。
import "DPI-C" function int imem_read(input int address);

module instruction_memory (
  input  wire [31:0] address,
  output wire [31:0] inst
);

  assign inst = imem_read(address);
endmodule
