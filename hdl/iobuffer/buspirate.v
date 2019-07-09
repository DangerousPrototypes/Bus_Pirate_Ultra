//------------------------------------------------------------------
//-- Bus Pirate peripheral tests
//--
//------------------------------------------------------------------
`include "iobuf.v"
`include "iobufphy.v"
module top (input clock,
            inout bufio,
            output wire bufdir,
            output wire bufod);

    localparam N = 24;
    reg [N:0] count;
    wire buf_data_oe;
    wire buf_data_dout;
    wire buf_data_din;
    wire dout;

    //                  oe      od  dir din dout       bufdir bufod      the pins from the SB_IO block below
    iobuf MOSI_BUF(1'b1, 1'b0, 1'b0, count[N],dout,    bufdir,  bufod,  buf_data_oe,buf_data_dout,buf_data_din);

  	always @(posedge clock)
  			count <= count + 1;

    //define the tristate data pin explicitly in the top module
    SB_IO #(
			.PIN_TYPE(6'b1010_01), //tristate
			.PULLUP(1'b0)          //no pullup
		) buf_data_pins (
			.PACKAGE_PIN(bufio),//which pin
			.OUTPUT_ENABLE(buf_data_oe),   //output enable wire
			.D_OUT_0(buf_data_dout),        //data out wire
			.D_IN_0(buf_data_din)           //data in wire
		);

endmodule
