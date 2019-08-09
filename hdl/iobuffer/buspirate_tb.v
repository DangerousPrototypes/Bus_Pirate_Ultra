`timescale 1ns/1ps
`define DUMPSTR(x) `"x.vcd`"

module buspirate_tb();

  parameter DURATION = 10;
  parameter MC_DATA_WIDTH = 16;
  parameter MC_ADD_WIDTH = 6;
  parameter LA_WIDTH = 8;
  parameter LA_CHIPS = 2;
  parameter BP_PINS = 5;
  parameter FIFO_WIDTH = 16;
  parameter FIFO_DEPTH = 256;

  reg clk, rst;

  wire [BP_PINS-1:0] bpio_io;
  wire [BP_PINS-1:0] bpio_dir, bpio_od,bpio_contention, bpio_state;
  reg [BP_PINS-1:0] bpio_test_input; //load test input values here
  wire [LA_CHIPS-1:0] sram_clock;
  wire [LA_CHIPS-1:0] sram_cs;
  wire [LA_WIDTH-1:0] sram_sio;
  reg [LA_WIDTH-1:0] sram_sio_d;
  wire lat_oe;
  reg [LA_WIDTH-1:0] lat;
  reg mcu_clock;
  reg mcu_cs;
  reg mcu_mosi; //sio0
  wire mcu_miso; //sio1
  reg mc_oe, mc_ce, mc_we;
  reg [MC_ADD_WIDTH-1:0] mc_add;
  wire [MC_DATA_WIDTH-1:0] mc_data;
  reg [MC_DATA_WIDTH-1:0] mc_data_reg;

  assign mc_data=(mc_oe)?mc_data_reg:16'hzzzz;

  assign sram_sio=(!mc_oe)?sram_sio_d:8'hzz;


  top #(
    .MC_DATA_WIDTH(MC_DATA_WIDTH),
    .MC_ADD_WIDTH(MC_ADD_WIDTH),
    .LA_WIDTH(LA_WIDTH),
    .LA_CHIPS(LA_CHIPS),
    .BP_PINS(BP_PINS),
    .FIFO_WIDTH(FIFO_WIDTH),
    .FIFO_DEPTH(FIFO_DEPTH)
    )buspirate(
    .clock(clk),
    //.reset(rst),
    .bpio_io(bpio_io),
    .bpio_dir(bpio_dir),
    .bpio_od(bpio_od),
    .sram_clock(sram_clock),
    .sram_cs(sram_cs),
    .sram_sio(sram_sio),
    .lat_oe(lat_oe),
    .lat(lat),
    .mcu_clock(mcu_clock),
    .mcu_mosi(mcu_mosi),
    .mcu_miso(mcu_miso),
    .mc_oe(mc_oe),
    .mc_ce(mc_ce),
    .mc_we(mc_we),
    .mc_add(mc_add),
    .mc_data(mc_data)
    );

    //this simulates the 74LVC logic buffers so we can see the results in simulation
    //the output from the iobuff "hardware driver" goes into here instead of physical hardware
    iobufphy BP_BUF[BP_PINS-1:0](
        .iopin_state(bpio_state),
        .iopin_contention(bpio_contention),
        .iopin_input(bpio_test_input),
      //hardware driver (reversed input/outputs from above)
        .bufdir(bpio_dir),
        .bufod(bpio_od),
        .bufio(bpio_io)
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
      bpio_test_input<=5'b11111;
      mc_we<=1;
      mc_oe<=1;
      mc_ce<=0;
      @(negedge rst); // wait for reset
      repeat(10) @(posedge clk);




      //IO pins setup
      mc_add = 6'h00; //od|oe
      mc_data_reg <= 16'h00FB;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);
      mc_add = 6'h01; //hl|dir
      mc_data_reg <= 16'h0004;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);
bpio_test_input<=5'b11111;

mc_data_reg=16'h8000;
repeat(6)@(posedge clk);
mc_we=0;
repeat(6)@(posedge clk);
mc_we=1;
repeat(10)@(posedge clk);

      //zSRAM SPI debug
      mc_add = 6'h07;
      mc_data_reg=16'h08ff;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(50)@(posedge clk);
      mc_add = 6'h07;
      mc_data_reg=16'h8001;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(200)@(posedge clk);




      //PWM
      mc_add = 6'h05;
      mc_data_reg <= 16'h0001;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);
      mc_add = 6'h06;
      mc_data_reg <= 16'h0001;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);

      //LA sample count
      mc_add = 6'h04;
      mc_data_reg=16'h0010;
      lat<=8'hAA;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;

      //SRAM quad, read, CS low...
      mc_add = 6'h03;
      //mc_data_reg <= 16'h0003;
      mc_data_reg=16'b0000000000000001;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;

      mc_add = 6'h02;
      //mc_data_reg = 16'hzzzz;
      sram_sio_d=8'haa;
      repeat(6)@(posedge clk);
      mc_oe=0;
      repeat(6)@(posedge clk);
      mc_oe=1;
      repeat(6)@(posedge clk);
      sram_sio_d=8'h55;
      repeat(6)@(posedge clk);
      mc_oe=0;
      repeat(6)@(posedge clk);
      mc_oe=1;
      repeat(12)@(posedge clk);
      //start LA
      mc_add = 6'h03;
      //mc_data_reg <= 16'h0003;
      mc_data_reg=16'b0000000000001001;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;







/*
      mc_add = 6'h10;
      mc_data_reg <= 16'b0000000001010000;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);
      mc_add = 6'h10;
      mc_data_reg <= 16'b0000000001000000;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);
      mc_data_reg <= 16'b0000000001101010;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);
      mc_data_reg <= 16'b0000000001001010;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);
      mc_data_reg <= 16'b0000000001011010;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);
      mc_data_reg <= 16'b0000000000001010;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);
      @(posedge clk)
      mc_oe=0;
      repeat(6)@(posedge clk);
      $display("%h",mc_data);
      repeat(6)@(posedge clk);
      mc_oe=1;
      repeat(6)@(posedge clk);
      @(posedge clk)
      mc_oe=0;
      repeat(6)@(posedge clk);
      $display("%h",mc_data);
      repeat(6)@(posedge clk);
      mc_oe=1;
      repeat(6)@(posedge clk);












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
*/

      repeat(100) @(posedge clk);
      $finish;
    end

endmodule
