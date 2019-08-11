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
  wire bp_active;

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
    .mc_data(mc_data),
    .bp_active(bp_active)
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

//pause BPSM
mc_add = 6'h03; //hl|dir
mc_data_reg <= 16'b10000000;
repeat(6)@(posedge clk);
mc_we=0;
repeat(6)@(posedge clk);
mc_we=1;
repeat(6)@(posedge clk);

      mc_add = 6'h07;

      mc_data_reg=16'hFE00; //IO pins low
      repeat(3)@(posedge clk);
      mc_we=0;
      repeat(3)@(posedge clk);
      mc_we=1;
repeat(3)@(posedge clk);
      mc_data_reg=16'h81FF; //IO pins high
      repeat(3)@(posedge clk);
      mc_we=0;
      repeat(3)@(posedge clk);
      mc_we=1;
repeat(3)@(posedge clk);
      mc_data_reg=16'h8100; //IO pins low
      repeat(3)@(posedge clk);
      mc_we=0;
      repeat(3)@(posedge clk);
      mc_we=1;
repeat(3)@(posedge clk);

      //FIFO
      mc_add = 6'h07;
      mc_data_reg=16'h08aa;
      repeat(3)@(posedge clk);
      mc_we=0;
      repeat(3)@(posedge clk);
      mc_we=1;
      repeat(3)@(posedge clk);
      mc_add = 6'h07;
      mc_data_reg=16'h08ff;
      repeat(3)@(posedge clk);
      mc_we=0;
      repeat(3)@(posedge clk);
      mc_we=1;
      repeat(3)@(posedge clk);
      mc_add = 6'h07;
      mc_data_reg=16'h0800;
      repeat(3)@(posedge clk);
      mc_we=0;
      repeat(3)@(posedge clk);
      mc_we=1;
      repeat(3)@(posedge clk);

      mc_data_reg=16'h840F;
      repeat(3)@(posedge clk);
      mc_we=0;
      repeat(3)@(posedge clk);
      mc_we=1;
      repeat(3)@(posedge clk);


      mc_data_reg=16'h81FF; //IO pins high
      repeat(3)@(posedge clk);
      mc_we=0;
      repeat(3)@(posedge clk);
      mc_we=1;
repeat(3)@(posedge clk);
      mc_data_reg=16'hFF00; //IO pins low
      repeat(3)@(posedge clk);
      mc_we=0;
      repeat(3)@(posedge clk);
      mc_we=1;
repeat(3)@(posedge clk);
      //trigger BP SM
      mc_add = 6'h03; //hl|dir
      mc_data_reg <= 16'b00000000;
      repeat(6)@(posedge clk);
      mc_we=0;
      repeat(6)@(posedge clk);
      mc_we=1;
      repeat(6)@(posedge clk);

      repeat(200)@(posedge clk);
      repeat(100) @(posedge clk);
      $finish;
    end

endmodule
