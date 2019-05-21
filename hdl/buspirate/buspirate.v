//------------------------------------------------------------------
//-- Bus Pirate peripheral tests
//--
//------------------------------------------------------------------
`include "iobuf.v"
`include "iobufphy.v"
`include "pwm.v"
module top (clock, reset,
            bufdir_mosi, bufod_mosi, bufio_mosi,
            bufdir_clock, bufod_clock, bufio_clock,
            bufdir_miso, bufod_miso, bufio_miso,
            bufdir_cs, bufod_cs, bufio_cs,
            bufdir_aux, bufod_aux, bufio_aux,
            lat_oe,lat,
            mc_oe, mc_ce, mc_we, mc_add,mc_data,
            irq0,irq1,
            sram_clock, sram0_cs, sram0_sio,sram1_cs, sram1_sio
            );

    localparam MC_DATA_WIDTH = 16;
    localparam MC_ADD_WIDTH = 6;

    input wire clock, reset;
    inout wire bufio_mosi,bufio_clock,bufio_miso,bufio_cs,bufio_aux;
    output wire bufdir_mosi, bufod_mosi, bufdir_clock, bufod_clock, bufdir_miso, bufod_miso, bufdir_cs, bufod_cs, bufdir_aux, bufod_aux;
    output wire lat_oe;
    input wire [7:0] lat;
    output wire mc_oe, mc_ce, mc_we;
    input wire [MC_ADD_WIDTH-1:0] mc_add;
    inout wire [MC_DATA_WIDTH-1:0] mc_data;
    inout wire irq0, irq1;
    output wire sram_clock, sram0_cs, sram1_cs;
    inout wire [3:0] sram0_sio, sram1_sio;

    // Tristate pin handling
    wire buftoe_mosi,buftoe_clock,buftoe_miso,buftoe_cs,buftoe_aux;
    wire buftdo_mosi,buftdo_clock,buftdo_miso,buftdo_cs,buftdo_aux;
    wire buftdi_mosi,buftdi_clock,buftdi_miso,buftdi_cs,buftdi_aux;
    // Memory controller interface
    wire [MC_DATA_WIDTH-1:0] mc_din;
    reg [MC_DATA_WIDTH-1:0] mc_dout;
    // Interrupts
    reg irq0_out, irq1_out;
    wire irq0_in, irq0_dir, irq1_in, irq1_dir;
    // Temporary stuff
    wire temp;
    wire pwm_out;

    localparam N = 24;
    reg [N:0] count; //count[N]
    //rst,					// reset
    //clkin,					// clock in
    //clkout,					// clock out
    //onperiod,				// #ticks period ontime
    //offperiod				// #ticks period offtime
    pwm AUX_PWM(reset, clock,pwm_out, 16'b0000000000000010,16'b0000000010000001);

    //                  oe      od    dir   din   dout bufdir bufod  the pins from the SB_IO block below
    //iobuf MOSI_BUF(, 1'b0, 1'b0, 1'b1,  temp,    bufdir_mosi,   bufod_mosi,  buftoe_mosi,buftdo_mosi,buftdi_mosi); //D2
    //iobuff CLOCK_BUFF(1'b0,    1'b0, 1'b0, 1'b0,   D9,    D8,   D7,  buff_data_oe[CLOCK],buff_data_dout[CLOCK],buff_data_din[CLOCK]); //D6
    //iobuff MISO_BUFF(count[N],1'b0,1'b0,1'b0, D6,    D5,   D4,  mosi_data_oe,mosi_data_dout,mosi_data_din);
    //iobuff CS(count[N],1'b0,1'b0,1'b0, D6,    D5,   D4,  mosi_data_oe,mosi_data_dout,mosi_data_din);
    iobuf AUX_BUF(1'b1, 1'b0, 1'b0, pwm_out, temp, bufdir_aux, bufod_aux, buftoe_aux, buftdo_aux,buftdi_aux);

  	always @(posedge clock)
  			count <= count + 1;


    //define the tristate data pin explicitly in the top module
    // Bus Pirate IO pins
    SB_IO #(
			.PIN_TYPE(6'b1010_01), //tristate
			.PULLUP(1'b0)          //no pullup
		) buf_tristate_mosi (
			.PACKAGE_PIN(bufio_mosi),//which pin
			.OUTPUT_ENABLE(buftoe_mosi),   //output enable wire
			.D_OUT_0(buftdo_mosi),        //data out wire
			.D_IN_0(buftdi_mosi)           //data in wire
		);
    SB_IO #(
			.PIN_TYPE(6'b1010_01), //tristate
			.PULLUP(1'b0)          //no pullup
		) buf_tristate_clock (
			.PACKAGE_PIN(bufio_clock),//which pin
			.OUTPUT_ENABLE(buftoe_clock),   //output enable wire
			.D_OUT_0(buftdo_clock),        //data out wire
			.D_IN_0(buftdi_clock)           //data in wire
		);
    SB_IO #(
      .PIN_TYPE(6'b1010_01), //tristate
      .PULLUP(1'b0)          //no pullup
    ) buf_tristate_miso (
      .PACKAGE_PIN(bufio_miso),//which pin
      .OUTPUT_ENABLE(buftoe_miso),   //output enable wire
      .D_OUT_0(buftdo_miso),        //data out wire
      .D_IN_0(buftdi_miso)           //data in wire
    );
    SB_IO #(
      .PIN_TYPE(6'b1010_01), //tristate
      .PULLUP(1'b0)          //no pullup
    ) buf_tristate_cs (
      .PACKAGE_PIN(bufio_cs),//which pin
      .OUTPUT_ENABLE(buftoe_cs),   //output enable wire
      .D_OUT_0(buftdo_cs),        //data out wire
      .D_IN_0(buftdi_cs)           //data in wire
    );
    SB_IO #(
      .PIN_TYPE(6'b1010_01), //tristate
      .PULLUP(1'b0)          //no pullup
    ) buf_tristate_aux (
      .PACKAGE_PIN(bufio_aux),//which pin
      .OUTPUT_ENABLE(buftoe_aux),   //output enable wire
      .D_OUT_0(buftdo_aux),        //data out wire
      .D_IN_0(buftdi_aux)           //data in wire
    );
    // Memory controller data pins
    SB_IO #(
      .PIN_TYPE(6'b1010_01),
      .PULLUP(1'b0)
    ) mc_io [MC_DATA_WIDTH-1:0] (
      .PACKAGE_PIN(mc_data),
      .OUTPUT_ENABLE(!mc_oe),
      .D_OUT_0(mc_dout),
      .D_IN_0(mc_din)
    );
    // Interrupt pins
    SB_IO #(
      .PIN_TYPE(6'b1010_01),
      .PULLUP(1'b0)
    ) irq0_io (
      .PACKAGE_PIN(irq0),
      .OUTPUT_ENABLE(irq0_dir),
      .D_OUT_0(irq0_out),
      .D_IN_0(irq0_in)
    );
    SB_IO #(
      .PIN_TYPE(6'b1010_01),
      .PULLUP(1'b0)
    ) irq1_io (
      .PACKAGE_PIN(irq1),
      .OUTPUT_ENABLE(irq1_dir),
      .D_OUT_0(irq1_out),
      .D_IN_0(irq1_in)
    );

endmodule
