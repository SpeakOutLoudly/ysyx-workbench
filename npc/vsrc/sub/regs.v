module RegisterFile #(ADDR_WIDTH = 1, DATA_WIDTH = 1) (
    input clk,
    input [DATA_WIDTH-1:0] wdata,
    input [ADDR_WIDTH-1:0] waddr,
    input wen

    input [DATA_WIDTH-1:0] raddr_rs1,
    input [DATA_WIDTH-1:0] raddr_rs2,
    
    output [ADDR_WIDTH-1:0]rdata_rs1,
    output [ADDR_WIDTH-1:0]rdata_rs2
);
    reg [DATA_WIDTH-1:0] rf [2**ADDR_WIDTH-1:0];
    always @(posedge clk) begin
        if (wen) rf[waddr] <= wdata;
    end
    assign rdata_rs1 = rf[raddr_rs1];
    assign rdata_rs2 = rf[raddr_rs2];
endmodule