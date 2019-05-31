//------------------------------------------------------------------
//-- Bus Pirate peripheral tests
//--
//------------------------------------------------------------------
`include "iobuf.v"
`include "iobufphy.v"
`include "pwm.v"
`include "spimaster.v"
`include "fifo.v"
`include "synchronizer.v"
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
    wire [MC_DATA_WIDTH-1:0] mc_dout;
    reg [MC_DATA_WIDTH-1:0] mc_dout_reg;



    // Interrupts
    reg irq0_out, irq1_out;
    wire irq0_in, irq0_dir, irq1_in, irq1_dir;
    // Temporary stuff
    wire temp, temp1, temp2, temp3, temp4, temp5;

    // PWM
    wire pwm_out, pwm_reset;
    reg [15:0] pwm_on, pwm_off;

    //pwm AUX_PWM(reset||pwm_reset, clock,pwm_out, pwm_on,pwm_off);

    //three control lines for each pin
    wire bp_mosi_din, bp_clock_din, bp_miso_din, bp_cs_din, bp_aux0_din; //input into the buffer (output pin direction)
    wire bp_mosi_dout, bp_clock_dout, bp_miso_dout, bp_cs_dout, bp_aux0_dout;//output from the buffer (input pin direction)
    wire bp_mosi_dir, bp_clock_dir, bp_miso_dir, bp_cs_dir, bp_aux0_dir;

      //FIFO
      wire in_fifo_in_nempty, in_fifo_in_full, in_fifo_out_nempty,in_fifo_in_shift,in_fifo_out_pop;
      wire out_fifo_in_nempty, out_fifo_in_full, out_fifo_out_nempty;
      wire [MC_DATA_WIDTH-1:0] out_fifo_out_data, out_fifo_in_data, in_fifo_out_data;
      wire out_fifo_in_shift;

      fifo FIFO_IN (
      	.clock(clock),
      	.in_shift(in_fifo_in_shift),
      	.in_data(mc_din),
      	.in_full(in_fifo_in_full),
      	.in_nempty(in_fifo_in_nempty),

      	.out_pop(in_fifo_out_pop),
      	.out_data(in_fifo_out_data),
      	.out_nempty(in_fifo_out_nempty)
      ), FIFO_OUT (
        .clock(clock),
      	.in_shift(out_fifo_in_shift), //???
      	.in_data(out_fifo_in_data), // in data
      	.in_full(out_fifo_in_full), //output
      	.in_nempty(out_fifo_in_nempty), //output

      	.out_pop(out_fifo_out_pop), //input out_pop,
      	.out_data(out_fifo_out_data), //out data,
      	.out_nempty(out_fifo_out_nempty) //output reg out_nempty
        );

        dispatch DISPATCH (
          clock,
          reset,
          //input from fifo and trigger
          in_fifo_out_nempty,
          in_fifo_out_pop,
          in_fifo_out_data, //16bits!

          //output to fifo and triggers
          //out_fifo_in_nempty,
          //out_fifo_in_full,
          //out_fifo_in_shift,
          //out_fifo_in_data,

          // pins (directions???)
          bp_mosi_din,				// master in slave out
          bp_clock_din,				// master out slave in
          bp_miso_dout,				// SPI clock (= clkin/2)
          bp_cs_din,					// chip select
          bp_aux0_din

          //error?

          //idle/busy?

          );


    //              oe    od    dir   din   dout bufdir bufod  the pins from the SB_IO block below
    iobuf MOSI_BUF(1'b1, 1'b0, 1'b0, bp_mosi_din, temp1, bufdir_mosi, bufod_mosi, buftoe_mosi, buftdo_mosi,buftdi_mosi);
    iobuf CLOCK_BUF(1'b1, 1'b0, 1'b0, bp_clock_din, temp2, bufdir_clock, bufod_clock, buftoe_clock, buftdo_clock,buftdi_clock);
    iobuf MISO_BUF(1'b1, 1'b0, 1'b1, 1'b0, bp_miso_dout, bufdir_miso, bufod_miso, buftoe_miso, buftdo_miso,buftdi_miso);
    iobuf CS_BUF(1'b1, 1'b0, 1'b0, bp_cs_din, temp3, bufdir_cs, bufod_cs, buftoe_cs, buftdo_cs,buftdi_cs);
    iobuf AUX_BUF(1'b1, 1'b0, 1'b0, bp_aux0_din, temp4, bufdir_aux, bufod_aux, buftoe_aux, buftdo_aux,buftdi_aux);

    //wires tied to the memory controller WE and OE signals
    wire mc_we_sync, mc_oe_sync;
    sync MC_WE_SYNC(clock, mc_we, mc_we_sync);
    sync MC_OE_SYNC(clock, mc_oe, mc_oe_sync);
    assign in_fifo_in_shift=(mc_add===6'h00)?mc_we_sync:1'b0; //follow the we signal
    assign mc_dout=(mc_add===6'h00&&!mc_oe)?out_fifo_out_data:mc_dout_reg;
    assign out_fifo_out_pop=(mc_add===6'h00)?mc_oe_sync:1'b0;//follow the oe signal
    assign pwm_reset=(mc_add===6'h1a)?!mc_we_sync:1'b0; //reset pwm counters after write to pwm

    //writes
    always @(posedge clock)
    begin
      if(mc_we_sync) begin
      case(mc_add)
        6'h19: pwm_on <= mc_din;// pwm on-time register
        6'h1a: pwm_off <= mc_din;	// pwm off-time register
      endcase
      end else if (mc_oe_sync) begin
      case(mc_add)
        6'h19:									// pwm on-time register
          begin
            mc_dout_reg<= pwm_on;
          end
        6'h1a:									// pwm off-time register
          begin
            mc_dout_reg<=pwm_off;
          end
      endcase
      end
    end

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
