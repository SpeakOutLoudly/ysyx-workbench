module ysyx_20230612_imem (
  input  wire        reset,
  input  wire        active,
  input  wire        req_valid,
  input  wire [31:0] pc,
  output reg         resp_valid,
  output reg  [31:0] inst
);

  always @(*) begin
    inst = 32'h00000013;
    resp_valid = 1'b0;
    // 旧 DPI-C 取指调用；当前 IFU 直接使用 SoC 的 io_ifu_rdata。
    // if (!reset && active)
    //   pmem_read(pc, 1'b1, req_valid, inst, resp_valid);
  end
endmodule
