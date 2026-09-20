// PC 更新子模块，只声明接口，暂不实现。
module pc_update (
  input  wire        clk,
  input  wire        reset,
  input  wire        redirect_valid,
  input  wire [31:0] redirect_pc,
  output reg [31:0] pc
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
