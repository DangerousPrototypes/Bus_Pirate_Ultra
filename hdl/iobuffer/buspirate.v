//------------------------------------------------------------------
//-- Bus Pirate peripheral tests
//--
//------------------------------------------------------------------
`include "iobuf.v"
`include "iobufphy.v"
module top (input clock,
            inout bufio,
            output wire bufdir,
            output wire bufod,
            output wire sram0_clock,
            output wire sram0_cs,
            inout[3:0] sram0_sio,
            input wire mcu_clock,
            input wire mcu_cs,
            //inout mcu_mosi, //sio0
            //inout mcu_miso, //sio1
            //inout mcu_sio2,
            //inout mcu_sio3,
            inout[3:0] mcu_sio,
            input wire mcu_quadmode,
            input wire mcu_direction,
            input wire mcu_arm,
            output wire lat_oe,
            input wire [3:0] lat
            );

    localparam N = 23;
    reg [N:0] count;
    wire buf_data_oe;
    wire buf_data_dout;
    wire buf_data_din;
    wire dout;

    reg [9:0] sample_counter;

  //wire [3:0] sram0_sio_tdo,sram0_sio_tdi, mcu_sio_tdo, mcu_sio_tdi;// sram0_sio_toe, mcu_sio_toe;
  //wire sram0_sio_toe, mcu_sio_toe;
  wire [3:0] sram0_sio_tdi,mcu_sio_tdi;
  wire [3:0] mcu_sio_tdo, sram0_sio_tdo;

    //                  oe      od  dir din dout       bufdir bufod      the pins from the SB_IO block below
    iobuf MOSI_BUF(1'b1, 1'b0, 1'b0, count[N],dout,    bufdir,  bufod,  buf_data_oe,buf_data_dout,buf_data_din);

    //SRAM test:
    //straight through SPI

    assign sram0_clock=mcu_clock;
    assign sram0_cs=mcu_cs;

    //assign mcu_sio_tdi=(mcu_arm) ? lat : sram0_sio_tdo;
    //assign sram0_sio_tdi=mcu_arm?lat:mcu_sio_tdo;

    assign mcu_sio_tdo=(mcu_arm) ? lat : sram0_sio_tdi;
    assign sram0_sio_tdo=(mcu_arm) ? lat : mcu_sio_tdi;
    //reg [3:0] sram_sio_tdo_d
    //assign sram0_sio_tdo = sram_sio_tdo_d;

    //LATCH OE
    assign lat_oe=1'b0;


    //LA`
    //latch enabled
    //latch Y connection to sram
    //sram Y connection to MCU
    //counter with reset?
    //mcu_quadmode H=quad mode, L = SPI mode
    //mcu_direction H = output from MCU to SRAM, L=output from SRAM to MCU

  	always @(posedge clock)
  			count <= count + 1;


    //define the tristate data pin explicitly in the top module
    //TODO: OE based on the combination of mcu_quadmode and mcu_direction
    //SRAM MOSI
    SB_IO #(
      .PIN_TYPE(6'b1010_01), //tristate
      .PULLUP(1'b0)          //no pullup
    ) sram0mosi (
      .PACKAGE_PIN(sram0_sio[0]),//which pin
      .OUTPUT_ENABLE(mcu_quadmode?mcu_direction:1'b1),   //output enable wire
      .D_OUT_0(sram0_sio_tdo[0]),        //data out wire
      .D_IN_0(sram0_sio_tdi[0])           //data in wire
    );
    //SRAM MISO
    SB_IO #(
			.PIN_TYPE(6'b1010_01), //tristate
			.PULLUP(1'b0)          //no pullup
		) sram0miso(
			.PACKAGE_PIN(sram0_sio[1]),//which pin
			.OUTPUT_ENABLE(mcu_quadmode?mcu_direction:1'b0),   //output enable wire
			.D_OUT_0(sram0_sio_tdo[1]),        //data out wire
			.D_IN_0(sram0_sio_tdi[1])           //data in wire
		);
    //SRAM SIO2/3
    SB_IO #(
			.PIN_TYPE(6'b1010_01), //tristate
			.PULLUP(1'b0)          //no pullup
		) sram0sio23[1:0] (
			.PACKAGE_PIN(sram0_sio[3:2]),//which pin
			.OUTPUT_ENABLE(mcu_quadmode && mcu_direction ),   //quadmode = 1 and direction = 1
			.D_OUT_0(sram0_sio_tdo[3:2]),        //data out wire
			.D_IN_0(sram0_sio_tdi[3:2])           //data in wire
		);
    //MCU MOSI
    SB_IO #(
      .PIN_TYPE(6'b1010_01), //tristate
      .PULLUP(1'b0)          //no pullup
    ) mcumiso (
      .PACKAGE_PIN(mcu_sio[0]),//which pin
      .OUTPUT_ENABLE(mcu_quadmode?!mcu_direction:1'b0),   //output enable wire
      .D_OUT_0(mcu_sio_tdo[0]),        //data out wire
      .D_IN_0(mcu_sio_tdi[0])           //data in wire
    );
    //MCU MISO
    SB_IO #(
      .PIN_TYPE(6'b1010_01), //tristate
      .PULLUP(1'b0)          //no pullup
    ) mcumosi (
      .PACKAGE_PIN(mcu_sio[1]),//which pin
      .OUTPUT_ENABLE(mcu_quadmode?!mcu_direction:1'b1),   //output enable wire
      .D_OUT_0(mcu_sio_tdo[1]),        //data out wire
      .D_IN_0(mcu_sio_tdi[1])           //data in wire
    );
    //MCU sram0sio23
    SB_IO #(
      .PIN_TYPE(6'b1010_01), //tristate
      .PULLUP(1'b0)          //no pullup
    ) mcusio23[1:0] (
      .PACKAGE_PIN(mcu_sio[3:2]),//which pin
      .OUTPUT_ENABLE(mcu_quadmode && !mcu_direction),   //quadmode = 1 and direction = 0
      .D_OUT_0(mcu_sio_tdo[3:2]),        //data out wire
      .D_IN_0(mcu_sio_tdi[3:2])           //data in wire
    );



    /*
    //SRAM SIO2/3
    SB_IO #(
			.PIN_TYPE(6'b1010_01), //tristate
			.PULLUP(1'b0)          //no pullup
		) sram0sio23[2:0] (
			.PACKAGE_PIN(sram0_sio[3:2]),//which pin
			.OUTPUT_ENABLE(sram0_sio_toe),   //output enable wire
			.D_OUT_0(sram0_sio_tdo[3:2]),        //data out wire
			.D_IN_0(sram0_sio_tdi[3:2])           //data in wire
		);

    SB_IO #(
      .PIN_TYPE(6'b1010_01), //tristate
      .PULLUP(1'b0)          //no pullup
    ) mcusio[3:0] (
      .PACKAGE_PIN(mcu_sio),//which pin
      .OUTPUT_ENABLE(mcu_sio_toe),   //output enable wire
      .D_OUT_0(mcu_sio_tdo),        //data out wire
      .D_IN_0(mcu_sio_tdi)           //data in wire
    );
    */

    SB_IO #(
      .PIN_TYPE(6'b1010_01), //tristate
      .PULLUP(1'b0)          //no pullup
    ) bufdatapin (
      .PACKAGE_PIN(bufio),//which pin
      .OUTPUT_ENABLE(buf_data_oe),   //output enable wire
      .D_OUT_0(buf_data_dout),        //data out wire
      .D_IN_0(buf_data_din)           //data in wire
    );

endmodule
