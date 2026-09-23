// 指令存储器子模块，只声明接口，暂不实现。
`include "sub/pc_update.v"
`include "sub/imem.v"
// 取指阶段，包含 PC 更新和指令读取。
module ifu (
  input  wire        clk,
  input  wire        reset,
  input  wire        lsu_respValid,
  input  wire        redirect_valid,
  input  wire [31:0] redirect_pc,
  output wire        commit_valid,
  output wire        lsu_reqValid,
  output reg [31:0]  pc,
  output wire [31:0] inst
);
    wire mem_access;
    wire ifu_reqValid;
    wire respValid, ifu_respValid;
    reg [1:0] state, nstate;

    always @(posedge clk) begin
        if (reset)
        state <= _idle;
        else
        state <= nstate;
    end

    always @(*) begin
        case(state)
        _wait:  begin
            if(respValid)
                nstate = _idle;
            else
                nstate = _wait;
        end
        _idle:  begin
            if (mem_access) begin
                nstate = _wait;
            end
            else
                nstate = _idle;
        end
        default: nstate = _idle;
        endcase
    end

    assign respValid = ifu_respValid || lsu_respValid;
    assign mem_access = (inst[6:0] == 7'b0000011) || (inst[6:0] == 7'b0100011);
    assign commit_valid = !reset &&
                          ((state == _idle && !mem_access) || (state == _wait && respValid));
    
    // 没有指令寄存器：PC 在提交前保持不变，三个状态都需看到同一条指令。
    assign ifu_reqValid = !reset && (state == _idle);
    assign lsu_reqValid = !reset && (state == _idle) && mem_access;

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
        .ifu_reqValid(ifu_reqValid),
        .pc(pc),
        .ifu_respValid(ifu_respValid),
        .inst(inst)
    );

endmodule


