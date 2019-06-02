`timescale 1ns/1ps
`define DUMPSTR(x) `"x.vcd`"

module buspirate_tb();

  parameter DURATION = 10;
  parameter BP_PINS = 5;

  localparam MC_DATA_WIDTH = 16;
  localparam MC_ADD_WIDTH = 6;

  reg clk, rst;

  // PHY simulates the IO buffer hardware
  wire [BP_PINS-1:0] bufio, bufdir, bufod, contention, state;
  reg [BP_PINS-1:0] test_input; //load test input values here

  wire lat_oe;
  wire [7:0] lat;
  reg mc_oe, mc_ce, mc_we;
  reg [MC_ADD_WIDTH-1:0] mc_add;
  wire [MC_DATA_WIDTH-1:0] mc_data;
  reg [MC_DATA_WIDTH-1:0] mc_data_reg;
  wire irq0, irq1;
  wire sram_clock, sram0_cs, sram1_cs;
  wire [3:0] sram0_sio, sram1_sio;

  assign mc_data=(mc_oe)?mc_data_reg:16'hzzzz;

  top buspirate(
    .clock(clk),
    .reset(rst),
    .bufio(bufio),
    .bufdir(bufdir),
    .bufod(bufod),
    .lat_oe(lat_oe),
    .lat(lat),
    .mc_oe(mc_oe),
    .mc_ce(mc_ce),
    .mc_we(mc_we),
    .mc_add(mc_add),
    .mc_data(mc_data),
    .irq0(irq0),
    .irq1(irq1),
    .sram_clock(sram_clock),
    .sram0_cs(sram0_cs),
    .sram0_sio(sram0_sio),
    .sram1_cs(sram1_cs),
    .sram1_sio(sram1_sio)
    );

    //this simulates the 74LVC logic buffers so we can see the results in simulation
    //the output from the iobuff "hardware driver" goes into here instead of physical hardware
    iobufphy BP_BUF[BP_PINS-1:0](
        .iopin_state(state),
        .iopin_contention(contention),
        .iopin_input(test_input),
      //hardware driver (reversed input/outputs from above)
        .bufdir(bufdir),
        .bufod(bufod),
        .bufio(bufio)
      );

    initial begin
      clk = 1'b0;
      rst = 1'b1;
      repeat(4) #10 clk = ~clk;
      rst = 1'b0;
      forever #10 clk = ~clk; // generate a clock
    end

    initial begin
      $dumpfile(`DUMPSTR(`VCD_OUTPUT));
      $dumpvars(0, buspirate_tb);
      test_input=5'b11111;
      mc_we=1;
      mc_oe=1;
      @(negedge rst); // wait for reset
      repeat(10) @(posedge clk);

      mc_add = 6'h19;
      mc_data_reg <= 16'h0003;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);
      mc_add = 6'h1a;
      mc_data_reg <= 16'h0003;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);

      mc_add = 6'h00;
      mc_data_reg <= 16'h0055;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);
      mc_data_reg <= 16'h0020;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);
      mc_data_reg <= 16'h0002;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);
      mc_data_reg <= 16'h0303;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);

      // Read test
      mc_add = 6'h00;
      @(posedge clk)
      mc_oe=0;
      repeat(6)@(posedge clk);
      $display("%h",mc_data);
      repeat(6)@(posedge clk);
      mc_oe=1;


      repeat(100) @(posedge clk);
      $finish;
    end

endmodule
