`include "sub/mac.v"

// 访存阶段，通过 SoC SimpleBus 发出请求并处理读响应。

module ysyx_20230612_lsu (
  input  wire [31:0] address,
  input  wire [31:0] store_data,
  input  wire [2:0]  funct3,
  input  wire        mem_read,
  input  wire        mem_write,
  output wire [31:0] io_addr,
  output wire [1:0]  io_size,
  output wire        io_wen,
  output wire [31:0] io_wdata,
  output wire [3:0]  io_wmask,
  input  wire [31:0] io_rdata,
  output reg  [31:0] load_data
);
  // 由访存控制子模块生成访问宽度、写掩码和加载扩展方式。
  wire [3:0] write_mask;
  wire [1:0] access_size;
  wire [31:0] selected_data;
  wire [31:0] shifted_store_data;
  wire [4:0] shift_amount;
  wire unsigned_load;

  wire [31:0] raw_data;

  ysyx_20230612_mac mac(
    .funct3(funct3),
    .address_offset(address[1:0]),
    .mem_read(mem_read),
    .mem_write(mem_write),
    .write_mask(write_mask),
    .access_size(access_size),
    .unsigned_load(unsigned_load)
  );

  // 原 DPI-C 访存调用留作参考；响应和读数据现由 SoC 输入。
  // if (active && mem_read)
  //   pmem_read(address, 1'b0, req_valid, raw_data, read_resp_valid);
  // else if (active && mem_write)
  //   pmem_write(address, shifted_store_data, dpi_write_mask,
  //              req_valid, write_resp_valid);
  assign io_addr  = address;
  assign io_size  = access_size;
  assign io_wen   = mem_write;
  assign io_wdata = shifted_store_data;
  assign io_wmask = write_mask;
  assign raw_data = io_rdata;

  // address[1:0] 表示目标数据位于 32 位字中的字节位置。
  assign shift_amount = {address[1:0], 3'b000};
  assign selected_data = raw_data >> shift_amount;

  // Store 数据需要移动到 write_mask 对应的字节位置。
  assign shifted_store_data = store_data << shift_amount;

  always @(*) begin
    load_data = 32'b0;
    if (mem_read) begin
      case (access_size)
        2'b00: begin // Byte: LB/LBU
          if (unsigned_load)
            load_data = {24'b0, selected_data[7:0]};
          else
            load_data = {{24{selected_data[7]}}, selected_data[7:0]};
        end
        2'b01: begin // Half word: LH/LHU
          if (unsigned_load)
            load_data = {16'b0, selected_data[15:0]};
          else
            load_data = {{16{selected_data[15]}}, selected_data[15:0]};
        end
        2'b10: begin // Word: LW
          load_data = selected_data;
        end
        default: begin
          load_data = 32'b0;
        end
      endcase
    end
  end

endmodule
