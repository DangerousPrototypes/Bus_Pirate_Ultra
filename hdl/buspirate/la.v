//------------------------------------------------------------------
//-- Passthrough connection to
//--
//------------------------------------------------------------------
`ifndef __LA__
`define __LA__
/*module la (
      input wire clock,
      input wire reset,
      input wire go,
      input wire write,
      input wire [5:0] data,
      // SRAM control
      output wire sram_cs,
      output wire sram_clock,
      output wire sram_oe,
      output reg [3:0] sram_din,
      input wire [3:0] sram_dout
      // 74xx573 latch
      output reg lat_oe,
      input wire [7:0] lat
    );

    reg [8:0] sample_count;

    assign lat_oe=1'b0; // latch is connected to the FPGA only and is always enabled
    assign sram_din=(write)?data[3:0]:lat[3:0];
    assign sram_clock=(go)?clock:data[4];
    assign sram_cs=data[5];

    always @(clock) begin



    //write to SRAM (pass through data MC=>SRAMs)
    //read from SRAM (setup then clock words?)
    //capture (latch open, clock till overflow)

    //23AA1024:
    // [ 0x38 ] over SPI to enter quad mode
    // [ 0x02 0 0 0 	+ latch open //setup write in quad mode
    // [ 0x03 0 0 0 r  //read command, address, read one dummy byte



endmodule*/
`endif
