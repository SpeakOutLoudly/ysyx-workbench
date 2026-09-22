`include "sub/mac.v"

// 访存阶段，对Mem进行操作，这里 Mem 用 C++写。
`ifndef PMEM_READ
`define PMEM_READ 
import "DPI-C" function int pmem_read(input int raddr);
`endif

import "DPI-C" function void pmem_write(
  input int waddr, input int wdata, input byte wmask);

module lsu (
  input  wire        clk,
  input  wire [31:0] address,
  input  wire [31:0] store_data,
  input  wire [2:0]  funct3,
  input  wire        mem_read,
  input  wire        mem_write,
  output reg  [31:0] load_data
);
  // 由访存控制子模块生成访问宽度、写掩码和加载扩展方式，再通过 DPI-C 访存。
  wire [3:0] write_mask;
  wire [1:0] access_size;
  wire [31:0] selected_data;
  wire [31:0] shifted_store_data;
  wire [4:0] shift_amount;
  wire [7:0] dpi_write_mask;
  wire unsigned_load;

  reg [31:0] raw_data;

  mac mac(
    .funct3(funct3),
    .address_offset(address[1:0]),
    .mem_read(mem_read),
    .mem_write(mem_write),
    .write_mask(write_mask),
    .access_size(access_size),
    .unsigned_load(unsigned_load)
  );

  // DPI-C Memory 总是读取包含目标地址的整个 32 位对齐字。
  always @(*) begin
    raw_data = 32'b0;

    if (mem_read)
      raw_data = pmem_read(address);
  end

  // address[1:0] 表示目标数据位于 32 位字中的字节位置。
  assign shift_amount = {address[1:0], 3'b000};
  assign selected_data = raw_data >> shift_amount;

  // Store 数据需要移动到 write_mask 对应的字节位置。
  assign shifted_store_data = store_data << shift_amount;
  assign dpi_write_mask = {4'b0, write_mask};

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

  // 简单 Memory 模型中重复写入同一值没有副作用；后续接入 MMIO 时应改为时钟沿写入。
  always @(posedge clk) begin
    if (mem_write && write_mask != 4'b0000)
      pmem_write(address, shifted_store_data, dpi_write_mask);
  end
endmodule

