`ifndef __IOBUFF__
`define __IOBUFF__
module iobuff (
      //interface
      input wire oe,
      input wire od,
      input wire dir,
      input wire din,
      output wire dout,
      //hardware driver
      output wire bufdir, //74LVC1T45 DIR pin LOW for Hi-Z
      output wire bufod, //74LVC1G07 OD pin HIGH for Hi-Z
      output wire bufdat_tristate_oe, //tristate data pin output enable
      output wire bufdat_tristate_dout, //tristate data pin data out
      input wire bufdat_tristate_din  //tristate data pin data in
    );

    assign dout=bufdat_tristate_din;        //D6 tristate data pin data in (should track D3)
    assign bufdir=(oe&&!od&&!dir)?1'b1:1'b0; //D5 74LVC1T45 direction pin H=1=output, L=0=input
    assign bufod=(oe&&od)?din:1'b1;         //D4 74LVC1G07 open drain H=HiZ, L=GND
    assign bufdat_tristate_oe = bufdir;     //D3 tristate data pin output enable H=1=output L=0=input
    assign bufdat_tristate_dout = din;      //D3 tristate data pin data out value (1 or 0)

endmodule
`endif
