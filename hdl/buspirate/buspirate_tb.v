`define DUMPSTR(x) `"x.vcd`"

module buspirate_tb();

  parameter DURATION = 10;

  localparam MC_DATA_WIDTH = 16;
  localparam MC_ADD_WIDTH = 6;

  reg clk, rst;

  wire mosi_state, mosi_contention,bufdir_mosi,bufod_mosi,bufio_mosi;
  reg mosi_input;
  wire clock_state, clock_contention,bufdir_clock,bufod_clock,bufio_clock;
  reg clock_input;
  wire miso_state, miso_contention,bufdir_miso,bufod_miso,bufio_miso;
  reg miso_input;
  wire cs_state, cs_contention,bufdir_cs,bufod_cs,bufio_cs;
  reg cs_input;
  wire aux_state, aux_contention,bufdir_aux,bufod_aux,bufio_aux;
  reg aux_input;

  wire lat_oe;
  wire [7:0] lat;
  wire mc_oe, mc_ce, mc_we;
  wire [MC_ADD_WIDTH-1:0] mc_add;
  wire [MC_DATA_WIDTH-1:0] mc_data;
  wire irq0, irq1;
  wire sram_clock, sram0_cs, sram1_cs;
  wire [3:0] sram0_sio, sram1_sio;

  top buspirate(
    .clock(clk),
    .reset(rst),
    .bufdir_mosi(bufdir_mosi),
    .bufod_mosi(bufod_mosi),
    .bufio_mosi(bufio_mosi),
    .bufdir_clock(bufdir_clock),
    .bufod_clock(bufod_clock),
    .bufio_clock(bufio_clock),
    .bufdir_miso(bufdir_miso),
    .bufod_miso(bufod_miso),
    .bufio_miso(bufio_miso),
    .bufdir_cs(bufdir_cs),
    .bufod_cs(bufod_cs),
    .bufio_cs(bufio_cs),
    .bufdir_aux(bufdir_aux),
    .bufod_aux(bufod_aux),
    .bufio_aux(bufio_aux),
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
    iobufphy AUX(
        .iopin_state(aux_state),
        .iopin_contention(aux_contention),
        .iopin_input(aux_input),
      //hardware driver (reversed input/outputs from above)
        .bufdir(bufdir_aux),
        .bufod(bufod_aux),
        .bufio(bufio_aux)
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
    /*  oe=1'b0; //initial values
      od=1'b0;
      dir=1'b0;
      din=1'b0;
      iopin_input=1'bz;*/
      @(negedge rst); // wait for reset
      repeat(1000) @(posedge clk);
      $finish;
    end

endmodule
