// 访存阶段，对Mem进行操作，这里 Mem 用 C++写。
import "DPI-C" function int pmem_read(input int raddr);
import "DPI-C" function void pmem_write(
  input int waddr, input int wdata, input byte wmask);

module lsu (
  input  wire [31:0] address,
  input  wire [31:0] store_data,
  input  wire [2:0]  funct3,
  input  wire        mem_read,
  input  wire        mem_write,
  output wire [31:0] load_data
);
  // TODO: 根据 funct3 生成访问宽度、写掩码以及加载数据扩展方式。
endmodule

// 访存控制子模块，只声明接口，暂不实现。
module memory_access_control (
  input  wire [2:0] funct3,
  input  wire [1:0] address_offset,
  input  wire       mem_read,
  input  wire       mem_write,
  output wire [3:0] write_mask,
  output wire [1:0] access_size,
  output wire       unsigned_load
);
endmodule
