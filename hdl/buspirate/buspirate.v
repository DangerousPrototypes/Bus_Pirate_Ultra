//------------------------------------------------------------------
//-- Bus Pirate peripheral tests
//--
//------------------------------------------------------------------
`include "iobuf.v"
`include "iobufphy.v"
module top (clk,
            bufdir_mosi, bufod_mosi, bufio_mosi,
            bufdir_clock, bufod_clock, bufio_clock,
            bufdir_miso, bufod_miso, bufio_miso,
            bufdir_cs, bufod_cs, bufio_cs,
            bufdir_aux, bufod_aux, bufio_aux,
            lat_oe,lat,
            mc_add,
            mc_data,
            irq0,irq1,
            sram_clock, sram0_cs, sram0_sio,sram1_cs, sram1_sio
            );

    input clk;
    inout wire bufio_mosi,bufio_clock,bufio_miso,bufio_cs,bufio_aux;
    output wire bufdir_mosi, bufod_mosi, bufdir_clock, bufod_clock, bufdir_miso, bufod_miso, bufdir_cs, bufod_cs, bufdir_aux, bufod_aux;
    output wire lat_oe;
    input wire [7:0] lat;
    input wire [5:0] mc_add;
    inout wire [15:0] mc_data;
    inout wire irq0, irq1;
    output wire sram_clock, sram0_cs, sram1_cs;
    inout wire [3:0] sram0_sio, sram1_sio;

    //tristate pin handling
    wire buftoe_mosi,buftoe_clock,buftoe_miso,buftoe_cs,buftoe_aux;
    wire buftdo_mosi,buftdo_clock,buftdo_miso,buftdo_cs,buftdo_aux;
    wire buftdi_mosi,buftdi_clock,buftdi_miso,buftdi_cs,buftdi_aux;

    localparam N = 24;
    reg [N:0] count;
    //                  oe      od    dir   din   dout bufdir bufod  the pins from the SB_IO block below
    iobuf MOSI_BUF(count[N], 1'b0, 1'b0, 1'b1,  irq0,    bufdir_mosi,   bufod_mosi,  buftoe_mosi,buftdo_mosi,buftdi_mosi); //D2
    //iobuff CLOCK_BUFF(1'b0,    1'b0, 1'b0, 1'b0,   D9,    D8,   D7,  buff_data_oe[CLOCK],buff_data_dout[CLOCK],buff_data_din[CLOCK]); //D6
    //iobuff MISO_BUFF(count[N],1'b0,1'b0,1'b0, D6,    D5,   D4,  mosi_data_oe,mosi_data_dout,mosi_data_din);
    //iobuff CS(count[N],1'b0,1'b0,1'b0, D6,    D5,   D4,  mosi_data_oe,mosi_data_dout,mosi_data_din);
    //iobuf AUX_BUF(1'b0,    1'b0, 1'b0, 1'b0,   D9,    D8,   D7,  buf_data_oe[AUX],buf_data_dout[AUX],buf_data_din[AUX]); //D6

  	always @(posedge clk)
  			count <= count + 1;

    //define the tristate data pin explicitly in the top module
    SB_IO #(
			.PIN_TYPE(6'b1010_01), //tristate
			.PULLUP(1'b0)          //no pullup
		) buf_data_pins (
			.PACKAGE_PIN(bufio_mosi),//which pin
			.OUTPUT_ENABLE(buftoe_mosi),   //output enable wire
			.D_OUT_0(buftdo_mosi),        //data out wire
			.D_IN_0(buftdi_mosi)           //data in wire
		);

endmodule
