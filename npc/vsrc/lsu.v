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
  output reg  [31:0] load_data
);
  // 由访存控制子模块生成访问宽度、写掩码和加载扩展方式，再通过 DPI-C 访存。
  wire [3:0] write_mask;
  wire [1:0] access_size;
  wire [31:0] raw_data, selected_data;
  wire [31:0] shifted_store_data;
  wire [4:0] shift_amount;
  wire [7:0] dpi_write_mask;
  wire unsigned_load;

  memory_access_control mac(
    .funct3(funct3),
    .address_offset(address[1:0]),
    .mem_read(mem_read),
    .mem_write(mem_write),
    .write_mask(write_mask),
    .access_size(access_size),
    .unsigned_load(unsigned_load)
  );

  // DPI-C Memory 总是读取包含目标地址的整个 32 位对齐字。
  assign raw_data = mem_read ? pmem_read(address) : 32'b0;

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
  always @(*) begin
    if (mem_write && write_mask != 4'b0000)
      pmem_write(address, shifted_store_data, dpi_write_mask);
  end
endmodule

// 访存控制子模块，只声明接口，暂不实现。
module memory_access_control (
  input  wire [2:0] funct3,
  input  wire [1:0] address_offset,
  input  wire       mem_read,
  input  wire       mem_write,
  output reg  [3:0] write_mask,
  output reg  [1:0] access_size,
  output reg        unsigned_load
);
  // access_size: 00=1 Byte，01=2 Bytes，10=4 Bytes。
  always @(*) begin
    write_mask = 4'b0000;
    access_size = 2'b00;
    unsigned_load = 1'b0;

    if (mem_read) begin
      case (funct3)
        3'b000: begin // LB
          access_size = 2'b00;
          unsigned_load = 1'b0;
        end
        3'b001: begin // LH
          access_size = 2'b01;
          unsigned_load = 1'b0;
        end
        3'b010: begin // LW
          access_size = 2'b10;
          unsigned_load = 1'b0;
        end
        3'b100: begin // LBU
          access_size = 2'b00;
          unsigned_load = 1'b1;
        end
        3'b101: begin // LHU
          access_size = 2'b01;
          unsigned_load = 1'b1;
        end
        default: begin
          access_size = 2'b00;
          unsigned_load = 1'b0;
        end
      endcase
    end
    else if (mem_write) begin
      case (funct3)
        3'b000: begin // SB
          access_size = 2'b00;
          write_mask = 4'b0001 << address_offset;
        end
        3'b001: begin // SH，只允许地址偏移 0 或 2
          access_size = 2'b01;
          if (address_offset == 2'b00)
            write_mask = 4'b0011;
          else if (address_offset == 2'b10)
            write_mask = 4'b1100;
        end
        3'b010: begin // SW，只允许 4 字节对齐地址
          access_size = 2'b10;
          if (address_offset == 2'b00)
            write_mask = 4'b1111;
        end
        default: begin
          access_size = 2'b00;
          write_mask = 4'b0000;
        end
      endcase
    end
  end
endmodule
