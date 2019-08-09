`ifndef __RAM__
`define __RAM__
module ram (
  din,
  write_en,
  waddr,
  wclk,
  raddr,
  rclk,
  dout
  );//512x8

parameter DEPTH = 256;
parameter WIDTH = 16;
localparam integer ABITS = $clog2(DEPTH);
input [ABITS-1:0] waddr, raddr;
input [WIDTH-1:0] din;
input write_en, wclk, rclk;
output reg [WIDTH-1:0] dout;
reg [WIDTH-1:0] mem [DEPTH-1:0];

always @(posedge wclk) // Write memory.
  begin
  if (write_en)
    mem[waddr] <= din; // Using write address bus.
  end

always @(posedge rclk) // Read memory.
  begin
    dout <= mem[raddr]; // Using read address bus.
  end

endmodule
`endif
