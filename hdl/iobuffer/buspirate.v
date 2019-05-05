//------------------------------------------------------------------
//-- Hello world example for the iCE40-HX8K board
//-- Turn on all the leds
//------------------------------------------------------------------
`include "iobuffer.v"
module top (input clk,
            inout D2,
            output wire D3,
            output wire D4,
            output wire D5,
            inout D6,
            output wire D7,
            output wire D8,
            output wire D9);

    localparam N = 24;
    localparam MOSI = 1;
    localparam CLOCK = 3;
    localparam MISO = 2;
    localparam  CS = 1;
    localparam  AUX =0;
    reg [N:0] count;
    wire [MOSI:AUX] buff_data_oe;
    wire [MOSI:AUX] buff_data_dout;
    wire [MOSI:AUX] buff_data_din;

    //                  oe      od    dir   din   dout bufdir bufod  the pins from the SB_IO block below
    iobuff MOSI_BUFF(count[N], 1'b0, 1'b0, 1'b1,   D5,    D4,   D3,  buff_data_oe[MOSI],buff_data_dout[MOSI],buff_data_din[MOSI]); //D2
    //iobuff CLOCK_BUFF(1'b0,    1'b0, 1'b0, 1'b0,   D9,    D8,   D7,  buff_data_oe[CLOCK],buff_data_dout[CLOCK],buff_data_din[CLOCK]); //D6
    //iobuff MISO_BUFF(count[N],1'b0,1'b0,1'b0, D6,    D5,   D4,  mosi_data_oe,mosi_data_dout,mosi_data_din);
    //iobuff CS(count[N],1'b0,1'b0,1'b0, D6,    D5,   D4,  mosi_data_oe,mosi_data_dout,mosi_data_din);
    iobuff AUX_BUFF(1'b0,    1'b0, 1'b0, 1'b0,   D9,    D8,   D7,  buff_data_oe[AUX],buff_data_dout[AUX],buff_data_din[AUX]); //D6

  	always @(posedge clk)
  			count <= count + 1;

    //define the tristate data pin explicitly in the top module
    SB_IO #(
			.PIN_TYPE(6'b1010_01), //tristate
			.PULLUP(1'b0)          //no pullup
		) buff_data_pins [MOSI:AUX] (
			.PACKAGE_PIN({D2,D6}),//which pin
			.OUTPUT_ENABLE(buff_data_oe),   //output enable wire
			.D_OUT_0(buff_data_dout),        //data out wire
			.D_IN_0(buff_data_din)           //data in wire
		);

endmodule

/*module iobuff (
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

endmodule*/
