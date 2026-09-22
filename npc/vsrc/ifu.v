// 指令存储器子模块，只声明接口，暂不实现。
`include "sub/pc_update.v"
`include "sub/imem.v"
// 取指阶段，包含 PC 更新和指令读取。
module ifu (
  input  wire        clk,
  input  wire        reset,
  input  wire        redirect_valid,
  input  wire [31:0] redirect_pc,
  output wire        commit_valid,
  output wire        load_read_en,
  output reg [31:0]  pc,
  output wire [31:0] inst
);
    wire is_load;
    wire fetch_valid;
    reg [1:0] state, nstate;

    always @(posedge clk) begin
        if (reset)
        state <= _wait;
        else
        state <= nstate;
    end

    always @(*) begin
        case(state)
        _wait:  nstate = _idle;
        _idle:  begin
            if (is_load) begin
                nstate = _sload;
            end
            else
                nstate = _wait;
        end
        _sload: nstate = _wait;
        default: nstate = _wait;
        endcase
    end

    assign is_load = (inst[6:0] == 7'b0000011);
    assign commit_valid = !reset &&
                          ((state == _idle && !is_load) || (state == _sload));
    assign load_read_en = !reset && (state == _sload) && is_load;
    // 没有指令寄存器：PC 在提交前保持不变，三个状态都需看到同一条指令。
    assign fetch_valid = !reset &&
                         ((state == _wait) || (state == _idle) ||
                          (state == _sload));

    pc_update pu(
        .clk(clk),
        .reset(reset),
        .commit_valid(commit_valid),
        .redirect_valid(redirect_valid),
        .redirect_pc(redirect_pc),
        .pc(pc)
    );

    imem inst_mem(
        .reset(reset),
        .fetch_valid(fetch_valid),
        .pc(pc),
        .inst(inst)
    );

endmodule


