module ysyx_20230612_RegisterFile #(ADDR_WIDTH = 1, DATA_WIDTH = 1) (
    input clk,
    input reset,
    input [DATA_WIDTH-1:0] wdata,
    input [ADDR_WIDTH-1:0] waddr,
    input wen,
    input inst_valid,

    input [ADDR_WIDTH-1:0] raddr_rs1,
    input [ADDR_WIDTH-1:0] raddr_rs2,
    input csr_wen,
    input [11:0] csr_addr,
    input [31:0] csr_in,
    output reg [31:0] csr_out,
    
    output [DATA_WIDTH-1:0]rdata_rs1,
    output [DATA_WIDTH-1:0]rdata_rs2,
    output [DATA_WIDTH-1:0]halt_code
);
    reg [DATA_WIDTH-1:0] rf [2**ADDR_WIDTH-1:0];
    reg [31:0] csr_rf [0:1];

    always @(posedge clk) begin
        if (reset) begin
            csr_rf[0] <= 32'h79737978; // mvendorid
            csr_rf[1] <= 32'h134b1d4; // marchid
        end else if (inst_valid) begin
            if (wen && waddr != 0) rf[waddr] <= wdata;
            if (csr_wen) begin
                case (csr_addr)
                    12'hf11: csr_rf[0] <= csr_in;
                    12'hf12: csr_rf[1] <= csr_in;
                    default: ;
                endcase
            end
        end
    end

    always @(*) begin
        case (csr_addr)
            12'hf11: csr_out = csr_rf[0];
            12'hf12: csr_out = csr_rf[1];
            default: csr_out = 32'b0;
        endcase
    end
    assign rdata_rs1 = (raddr_rs1 == 0) ? 0 : rf[raddr_rs1];
    assign rdata_rs2 = (raddr_rs2 == 0) ? 0 : rf[raddr_rs2];
    assign halt_code = rf[10];
endmodule
