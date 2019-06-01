//thoughts:
//0x00XX = data to spi
//0x01XX = [
//0x02XX = ]

`ifndef __facade__
`define __facade__
module facade (
    input wire clock,
    input wire reset,

    //input wire [15:0] configuration_register,
    input wire [7:0] in_data,
    //out_data,
    input wire go,
    output wire state,
    // pins (directions???)
    output wire bp_mosi,				// master in slave out
    output wire bp_clock,				// master out slave in
    input wire bp_miso,				// SPI clock (= clkin/2)
    output wire bp_cs					// chip select
    //bp_aux0

  );

  // SPI master
  // sync signals
  wire spi_go;					// starts a SPI transmission
  wire spi_state;				// state of module (0=idle, 1=busy/transmitting)
  reg spi_state_last;

  // data in/out
  wire [7:0] spi_dout;				// data out (will get received)

  assign spi_din=in_data;
  //assign in_fifo_out_pop=!spi_state;
  //assign spi_go=(in_fifo_out_nempty && !spi_state)? 1'b1:1'b0; //if pending FIFO and SPI idle
  //assign out_fifo_in_shift=((spi_state_last===1'b1)&&(spi_state===1'b0));

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
    go,					// starts a SPI transmission
  state,				// state of module (0=idle, 1=busy/transmitting)
  // data in/out
    in_data, 			// data in (will get transmitted)
    spi_dout,				// data out (will get received)
  // spi signals
    bp_miso,				// master in slave out
    bp_mosi,				// master out slave in
    bp_clock,				// SPI clock (= clkin/2)
    bp_cs					// chip select
    );

    always @(posedge clock)
    begin
      spi_state_last<=spi_state;
    end
endmodule

`endif
