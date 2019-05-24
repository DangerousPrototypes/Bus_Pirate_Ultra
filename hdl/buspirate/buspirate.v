//------------------------------------------------------------------
//-- Bus Pirate peripheral tests
//--
//------------------------------------------------------------------
`include "iobuf.v"
`include "iobufphy.v"
`include "pwm.v"
`include "spimaster.v"
`include "fifo.v"
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
    input wire mc_oe, mc_ce, mc_we;
    input wire [MC_ADD_WIDTH-1:0] mc_add;
    input wire [MC_DATA_WIDTH-1:0] mc_data;
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

    // PWM
    wire pwm_out;
    reg [15:0] pwm_on, pwm_off;

    // SPI master
    // sync signals
    wire spi_go;					// starts a SPI transmission
    wire spi_state;				// state of module (0=idle, 1=busy/transmitting)
    // data in/out
    wire [7:0] spi_din; 			// data in (will get transmitted)
    wire [7:0] spi_dout;				// data out (will get received)
    // spi signals
    wire spi_miso,spi_mosi,spi_clock,spi_cs;

    localparam N = 24;
    reg [N:0] count; //count[N]
    //rst,					// reset
    //clkin,					// clock in
    //clkout,					// clock out
    //onperiod,				// #ticks period ontime
    //offperiod				// #ticks period offtime
    pwm AUX_PWM(reset, clock,pwm_out, pwm_on,pwm_off);

    spimaster SPI_MASTER(
    // general control
    	reset,				// resets module to known state
    	clock,				// clock that makes everyhting tick
    // spi configuration
    	1'b1, //cpol,				// clock polarity
    	1'b0, //cpha,				// clock phase
    	1'b1, //cspol,				// CS polarity
    	1'b1, //autocs,				// assert CS automatically
    // sync signals
    	spi_go,					// starts a SPI transmission
    	spi_state,				// state of module (0=idle, 1=busy/transmitting)
    // data in/out
    	spi_din, 			// data in (will get transmitted)
    	spi_dout,				// data out (will get received)
    // spi signals
    	spi_miso,				// master in slave out
    	spi_mosi,				// master out slave in
    	spi_clock,				// SPI clock (= clkin/2)
    	spi_cs					// chip select
    	);

      //FIFO
      wire fifo_in_nempty, fifo_in_full,fifo_out_nempty;

      fifo FIFO_IN (
      	clock,
      	mc_we, //in_shift
      	mc_data, //16bits of data
      	fifo_in_full,
      	fifo_in_nempty,

      	spi_state, //input                  out_pop,
      	spi_din, //output     [WIDTH-1:0] out_data,
      	fifo_out_nempty //output reg             out_nempty

      );

    //              oe    od    dir   din   dout bufdir bufod  the pins from the SB_IO block below
    iobuf MOSI_BUF(1'b1, 1'b0, 1'b0, spi_mosi, temp, bufdir_mosi, bufod_mosi, buftoe_mosi, buftdo_mosi,buftdi_mosi);
    iobuf CLOCK_BUF(1'b1, 1'b0, 1'b0, spi_clock, temp, bufdir_clock, bufod_clock, buftoe_clock, buftdo_clock,buftdi_clock);
    iobuf MISO_BUF(1'b1, 1'b0, 1'b1, 1'b0, spi_miso, bufdir_miso, bufod_miso, buftoe_miso, buftdo_miso,buftdi_miso);
    iobuf CS_BUF(1'b1, 1'b0, 1'b0, spi_cs, temp, bufdir_cs, bufod_cs, buftoe_cs, buftdo_cs,buftdi_cs);
    iobuf AUX_BUF(1'b1, 1'b0, 1'b0, pwm_out, temp, bufdir_aux, bufod_aux, buftoe_aux, buftdo_aux,buftdi_aux);

  	always @(posedge clock)
  			count <= count + 1;

   assign spi_go=fifo_in_nempty;

    always @ (posedge mc_we)
      case(mc_add)
      6'h00:
        begin

        end
      6'h19:
        begin
        pwm_on=mc_data;
        pwm_off=mc_data;
        end
      endcase



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
  /*  SB_IO #(
      .PIN_TYPE(6'b1010_01),
      .PULLUP(1'b0)
    ) mc_io [MC_DATA_WIDTH-1:0] (
      .PACKAGE_PIN(mc_data),
      .OUTPUT_ENABLE(!mc_oe),
      .D_OUT_0(mc_dout),
      .D_IN_0(mc_din)
    );*/
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
