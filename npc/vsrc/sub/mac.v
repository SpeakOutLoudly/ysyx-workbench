// 访存控制子模块，只声明接口，暂不实现。
// memory access controler
module mac (
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
