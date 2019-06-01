//thoughts:
// handed off to peripheral facade
//0x00XX = data to peripheral
//0x01XX = [
//0x02XX = ]
//0x03XX = ??

//handled locally in the dispatcher
//0x10XX = delay
// aux pin 0/1/hiz(read) pwm/freq/etc
//output enable

`ifndef __dispatch__
`define __dispatch__
module dispatch (
      input clock,
      input reset,
      //input from fifo and trigger
      output in_fifo_out_clock,
      input in_fifo_out_nempty,
      output in_fifo_out_pop,
      input      [16-1:0] in_fifo_out_data, //16bits!

      //output to fifo and triggers
      //out_fifo_in_nempty,
    //  out_fifo_in_full,
    //  out_fifo_in_shift,
    //  out_fifo_in_data,

      // pins (directions???)
      output bp_mosi,				// master in slave out
      output bp_clock,				// master out slave in
      input bp_miso,				// SPI clock (= clkin/2)
      output bp_cs,					// chip select
      output bp_aux0  //aux pin

      //error?

      //idle/busy?

  );

  //reg in_fifo_out_pop;
  //assign in_fifo_out_pop=(in_fifo_out_nempty&&count[N]);

  reg aux_pin_state;
  assign bp_aux0=aux_pin_state;
  //reg in_fifo_out_clock;
  assign in_fifo_out_clock=clock; //this will be a divided clock eventually

  reg nready;
  reg in_data_ready;
  reg in_fifo_out_nempty_d, dispatch_go;
  wire [7:0] dispatch_command = in_fifo_out_data[15:8];
  wire [7:0] dispatch_data = in_fifo_out_data[7:0];

  localparam  N = 3;
  reg [N:0] count;
  `define RESET 3'b000
  `define ACTION 3'b001
  `define DELAY 3'b010
  `define WAIT 3'b011
  reg [2:0] state_f;
  reg go;

  facade FACADE (
      clock,
      reset,

      //configuration_register,
      dispatch_data,
      //out_data,
      go,
      state,

      // pins (directions???)
      bp_mosi,				// master in slave out
      bp_clock,				// master out slave in
      bp_miso,				// SPI clock (= clkin/2)
      bp_cs					// chip select
      //bp_aux0

    );

  initial begin
    count<=4'b1111;
    state_f<=`RESET;
    in_fifo_out_pop<=0;
  end

reg in_fifo_out_pop;

  always @(posedge clock)
  begin
      go<=0;
      if(!count[N]) count<=count+1;
      else if(!state && count[N] && in_fifo_out_nempty) begin
        in_fifo_out_pop<=1;
      end else if(in_fifo_out_pop) begin
        in_fifo_out_pop<=0;
        case(dispatch_command)
          //pass data to facade
          //pass bitwise commands {}[] /\!._-^ to facade
          //TODO: wait for done signal from facade, get SPI byte
          8'h00:  begin go<=1; end
          //AUX (0/1/hiz/read/pwm/freqmeasure)
          8'h01:	begin aux_pin_state <= 0; end
          8'h02:	begin aux_pin_state <= 1; end
          //delays
          8'h03:	count<=dispatch_data;
        endcase
      end
  end

endmodule
`endif
