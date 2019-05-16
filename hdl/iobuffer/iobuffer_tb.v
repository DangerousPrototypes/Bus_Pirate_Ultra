`define DUMPSTR(x) `"x.vcd`"

module iobuffer_tb();

  parameter DURATION = 10;

  reg clk, rst;

  wire iopin_state, iopin_contention;
  reg iopin_input;
  //interface
   reg oe; // enable 1=true
   reg od; //open drain 1=true
   reg dir;//direction 1=
   reg din;//data in (value when buffer is )
   wire dout;//data out (value when buffer is )
  //hardware driver
   wire bufdir; //74LVC1T45 DIR pin LOW for Hi-Z
   wire bufod; //74LVC1G07 OD pin HIGH for Hi-Z
   wire bufdat_tristate_oe; //tristate data pin  enable
   wire bufdat_tristate_dout; //tristate data pin data out
   wire bufdat_tristate_din;  //tristate data pin data in

//an instance of the bus pirate io buffer driver
  iobuff DUT(
    //interface
      .oe(oe), //.module(testbench)
      .od(od),
      .dir(dir),
      .din(din),
      .dout(dout),
    //hardware driver
      .bufdir(bufdir),
      .bufod(bufod),
      .bufdat_tristate_oe(bufdat_tristate_oe),
      .bufdat_tristate_dout(bufdat_tristate_dout),
      .bufdat_tristate_din(bufdat_tristate_din)
    );

    //this simulates the 74LVC logic buffers so we can see the results in simulation
    //the output from the iobuff "hardware driver" goes into here instead of physical hardware
    iobuffphy SYM(
        .iopin_state(iopin_state),
        .iopin_contention(iopin_contention),
        .iopin_input(iopin_input),
      //hardware driver (reversed input/outputs from above)
        .bufdir(bufdir),
        .bufod(bufod),
        .bufdat_tristate_oe(bufdat_tristate_oe),
        .bufdat_tristate_dout(bufdat_tristate_dout),
        .bufdat_tristate_din(bufdat_tristate_din)
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
      $dumpvars(0, iobuffer_tb);
      oe=1'b0; //initial values
      od=1'b0;
      dir=1'b0;
      din=1'b0;
      iopin_input=1'bz;
      @(negedge rst); // wait for reset
      oe=1'b0;
      od=1'b0;
      dir=1'b0;
      din=1'b0;
      repeat(1) @(posedge clk);
      oe=1'b1;
      od=1'b0;
      dir=1'b0;
      din=1'b0;
      repeat(1) @(posedge clk);
      oe=1'b1;
      od=1'b0;
      dir=1'b0;
      din=1'b1;
      repeat(1) @(posedge clk);
      oe=1'b1;
      od=1'b1;
      dir=1'b0;
      din=1'b1;
      repeat(1) @(posedge clk);
      oe=1'b1;
      od=1'b1;
      dir=1'b0;
      din=1'b0;
      repeat(1) @(posedge clk);
      $finish;
    end

endmodule
