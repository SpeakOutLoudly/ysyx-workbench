module ysyx_20230612_RegisterFile #(ADDR_WIDTH = 1, DATA_WIDTH = 1) (
    input clk,
    input [DATA_WIDTH-1:0] wdata,
    input [ADDR_WIDTH-1:0] waddr,
    input wen,
    input inst_valid,

    input [ADDR_WIDTH-1:0] raddr_rs1,
    input [ADDR_WIDTH-1:0] raddr_rs2,
    
    output [DATA_WIDTH-1:0]rdata_rs1,
    output [DATA_WIDTH-1:0]rdata_rs2,
    output [DATA_WIDTH-1:0]halt_code
);
    reg [DATA_WIDTH-1:0] rf [2**ADDR_WIDTH-1:0];
    always @(posedge clk) begin
        if (inst_valid && wen && waddr != 0) rf[waddr] <= wdata;
    end
    assign rdata_rs1 = (raddr_rs1 == 0) ? 0 : rf[raddr_rs1];
    assign rdata_rs2 = (raddr_rs2 == 0) ? 0 : rf[raddr_rs2];
    assign halt_code = rf[10];
endmodule
