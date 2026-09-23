`include "sub/pc_update.v"
// 原 DPI-C 取指模块已由 SoC 的 IFU 响应端口替代。
// `include "sub/imem.v"
// 取指阶段，包含 PC 更新和指令读取。
module ysyx_20230612_ifu (
  input  wire        clk,
  input  wire        reset,
  input  wire        ifu_resp_valid,
  input  wire [31:0] ifu_rdata,
  input  wire        lsu_resp_valid,
  input  wire        redirect_valid,
  input  wire [31:0] redirect_pc,
  output wire        commit_valid,
  output wire        ifu_req_valid,
  output wire        lsu_req_valid,
  output wire [31:0] pc,
  output wire [31:0] inst
);
    wire mem_access;
    reg [31:0] inst_reg;
    reg [1:0] state, nstate;

    always @(posedge clk) begin
        if (reset) begin
            state <= _fetch_req;
            inst_reg <= 32'h00000013;
        end
        else begin
            state <= nstate;
            if (((state == _fetch_req) || (state == _fetch_wait)) &&
                ifu_resp_valid)
                inst_reg <= ifu_rdata;
        end
    end

    always @(*) begin
        nstate = state;
        case (state)
        _fetch_req: begin
            // 当前接口默认存储器总能接收请求；同时兼容零周期响应。
            if (ifu_resp_valid)
                nstate = _exec;
            else
                nstate = _fetch_wait;
        end
        _fetch_wait: begin
            if (ifu_resp_valid)
                nstate = _exec;
        end
        _exec: begin
            if (!mem_access || lsu_resp_valid)
                nstate = _fetch_req;
            else
                nstate = _dmem_wait;
        end
        _dmem_wait: begin
            if (lsu_resp_valid)
                nstate = _fetch_req;
        end
        default: nstate = _fetch_req;
        endcase
    end

    assign inst = inst_reg;
    assign mem_access = (inst_reg[6:0] == 7'b0000011) || (inst_reg[6:0] == 7'b0100011);
    assign commit_valid = !reset &&
                          (((state == _exec) && !mem_access) ||
                           ((state == _exec) && mem_access && lsu_resp_valid) ||
                           ((state == _dmem_wait) && lsu_resp_valid));
    assign ifu_req_valid = !reset && (state == _fetch_req);
    assign lsu_req_valid = !reset && (state == _exec) && mem_access;

    ysyx_20230612_pc_update pu(
        .clk(clk),
        .reset(reset),
        .commit_valid(commit_valid),
        .redirect_valid(redirect_valid),
        .redirect_pc(redirect_pc),
        .pc(pc)
    );


endmodule
