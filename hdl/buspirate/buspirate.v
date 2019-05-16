//------------------------------------------------------------------
//-- Bus Pirate peripheral tests
//--
//------------------------------------------------------------------
`include "iobuf.v"
`include "iobufphy.v"
module top (input clk,
            inout[1:0] a,
            output wire D3,
            output wire D4,
            output wire D5,
            //inout D6,
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
    wire [MOSI:AUX] buf_data_oe;
    wire [MOSI:AUX] buf_data_dout;
    wire [MOSI:AUX] buf_data_din;

    //                  oe      od    dir   din   dout bufdir bufod  the pins from the SB_IO block below
    iobuf MOSI_BUF(count[N], 1'b0, 1'b0, 1'b1,   D5,    D4,   D3,  buf_data_oe[MOSI],buf_data_dout[MOSI],buf_data_din[MOSI]); //D2
    //iobuff CLOCK_BUFF(1'b0,    1'b0, 1'b0, 1'b0,   D9,    D8,   D7,  buff_data_oe[CLOCK],buff_data_dout[CLOCK],buff_data_din[CLOCK]); //D6
    //iobuff MISO_BUFF(count[N],1'b0,1'b0,1'b0, D6,    D5,   D4,  mosi_data_oe,mosi_data_dout,mosi_data_din);
    //iobuff CS(count[N],1'b0,1'b0,1'b0, D6,    D5,   D4,  mosi_data_oe,mosi_data_dout,mosi_data_din);
    iobuf AUX_BUF(1'b0,    1'b0, 1'b0, 1'b0,   D9,    D8,   D7,  buf_data_oe[AUX],buf_data_dout[AUX],buf_data_din[AUX]); //D6

  	always @(posedge clk)
  			count <= count + 1;

    //define the tristate data pin explicitly in the top module
    SB_IO #(
			.PIN_TYPE(6'b1010_01), //tristate
			.PULLUP(1'b0)          //no pullup
		) buf_data_pins [MOSI:AUX] (
			.PACKAGE_PIN(a),//which pin
			.OUTPUT_ENABLE(buf_data_oe),   //output enable wire
			.D_OUT_0(buf_data_dout),        //data out wire
			.D_IN_0(buf_data_din)           //data in wire
		);

endmodule
