`define DUMPSTR(x) `"x.vcd`"

module iobuffer_tb();

  parameter DURATION = 10;

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
   reg bufdat_tristate_din;  //tristate data pin data in

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

    initial begin
      $dumpfile(`DUMPSTR(`VCD_OUTPUT));
      $dumpvars(0, iobuffer_tb);
      oe=1'b0;
      od=1'b0;
      dir=1'b0;
      din=1'b0;
      bufdat_tristate_din=din;
      #20
      oe=1'b1;
      od=1'b0;
      dir=1'b0;
      din=1'b0;
      #20
      oe=1'b1;
      od=1'b0;
      dir=1'b0;
      din=1'b1;
      #20
      oe=1'b1;
      od=1'b1;
      dir=1'b0;
      din=1'b1;
      #20
      oe=1'b1;
      od=1'b1;
      dir=1'b0;
      din=1'b0;
      #20
      $finish;
    end

endmodule
