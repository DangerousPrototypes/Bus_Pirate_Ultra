//------------------------------------------------------------------
//-- Input/output buffer hardware physical layer simulation
//-- Very stupid simulation to easily spot pin behavior
//-- Basic contention detection when 74LVC1T45 is input and FPGA pin is set to output
//-- 74LVC1T45 is a bidirectional buffer. LOW is input to FPGA and HiZ on IO header
//-- 74LVC1G07 is an open drain output buffer. HIGH is HiZ, LOW is GND
//--
//------------------------------------------------------------------
`ifndef __IOBUFFPHY__
`define __IOBUFFPHY__
module iobuffphy (
      //interface
      output wire iopin_state, //is pin 0/1/HiZ?
      output wire iopin_contention, //if buffer direction is input and pin is output this is a problem!!
      input wire iopin_input, //put your test values here to simulate external stimulation of the buffer
      //hardware driver
      input wire bufdir, //74LVC1T45 DIR pin LOW for Hi-Z
      input wire bufod, //74LVC1G07 OD pin HIGH for Hi-Z
      input wire bufdat_tristate_oe, //tristate data pin output enable
      input wire bufdat_tristate_dout, //tristate data pin data out
      output wire bufdat_tristate_din  //tristate data pin data in
    );

    assign iopin_state=(!bufdir && bufod)?1'bz:(!bufdir&&!bufod)?1'b0:bufdat_tristate_dout;
    assign bufdat_tristate_din=iopin_input; //for simulating pin input
    assign iopin_contention=(!bufdir&&bufdat_tristate_oe); //if dir is low and fpga pin is output, then contention!!!!



endmodule
`endif
