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

//TODO:
// aux pin 0/1/hiz/read/PWM/freq?
// fifo full/data ready IRQ
// operation complete IRQ
// MUX freqmeasure and PWM??


`ifndef __dispatch__
`define __dispatch__
module dispatch #(
    parameter FIFO_WIDTH = 16,
    parameter BP_PINS = 5
  )(
      input clock,
      input reset,
      //input from fifo and trigger
      output in_fifo_out_clock,
      input in_fifo_out_nempty,
      output in_fifo_out_pop,
      input wire [FIFO_WIDTH-1:0] in_fifo_out_data, //16bits!
      //output to fifo and triggers
      output wire out_fifo_in_clock,
      input wire out_fifo_in_full,
      output wire out_fifo_in_shift,
      output wire [FIFO_WIDTH-1:0] out_fifo_in_data,
      // pins (directions???)
      output wire [BP_PINS-1:0] bp_din,
      input wire [BP_PINS-1:0] bp_dout
      //error?

    //idle/busy?

  );

  reg in_fifo_out_pop, go;
  wire state;
  wire [7:0] dispatch_command = in_fifo_out_data[15:8];
  wire [7:0] dispatch_data = in_fifo_out_data[7:0];
  wire [7:0] out_data;
  assign in_fifo_out_clock=clock; //this will be a divided clock eventually
  assign out_fifo_in_clock=clock;
  assign out_fifo_in_data[15:8] = dispatch_command[7:0];
  assign out_fifo_in_data[7:0] = out_data[7:0];
  localparam  N = 3;
  reg [N:0] count;

  reg aux_pin_state;
  //assign bp_aux0=aux_pin_state;

  facade  #(
    .BP_PINS(BP_PINS)
    )FACADE(
      .clock(clock),
      .reset(reset),
      //configuration_register,
      .in_data(dispatch_data),
      .out_data(out_data),
      .go(go),
      .state(state),
      .data_ready(out_fifo_in_shift),
      // pins
      .bp_din(bp_din),
      .bp_dout(bp_dout)

    );

  initial begin
    count<=4'b1111;
    in_fifo_out_pop<=0;
  end

  always @(posedge clock)
  begin
      go<=0;

      //TODO: block until FIFO_OUT !full
      if(!count[N]) count<=count+1;
      else if(!state && count[N] && in_fifo_out_nempty) begin
        in_fifo_out_pop<=1;
      end else if(in_fifo_out_pop) begin
        in_fifo_out_pop<=0;
        case(dispatch_command)
          //pass data to facade
          8'h00:  begin go<=1; end
          // TODO: pass bitwise commands {}[] /\!._-^ to facade
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
