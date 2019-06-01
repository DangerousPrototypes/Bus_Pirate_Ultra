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

/*  facade FACADE (
      clock,
      reset,
      in_fifo_out_pop,
      in_fifo_out_nempty,
      out_fifo_in_shift,
      //configuration_register,

      in_data,
      out_data,

      // pins (directions???)
      bp_mosi,				// master in slave out
      bp_clock,				// master out slave in
      bp_miso,				// SPI clock (= clkin/2)
      bp_cs,					// chip select
      bp_aux0

    );*/
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

localparam  N = 6;
  reg [N:0] count;
`define RESET 3'b000
`define ACTION 3'b001
`define DELAY 3'b010
`define WAIT 3'b011
reg [2:0] state_f;

  initial begin
    count<=6'b111111;
    state_f<=`RESET;
  end

  assign in_fifo_out_pop=(state_f===`RESET && in_fifo_out_nempty);

  always @(posedge clock)
  begin
    case(state_f)
      `RESET: begin
        count<=0;
        if(in_fifo_out_pop) begin
          state_f=`ACTION;
        end
      end
      `ACTION: begin
        case(dispatch_command)
          //AUX (0/1/hiz/read/pwm/freqmeasure)
          //TODO: how to pre/post delay on dispatch commands
          8'h01:	aux_pin_state <= 0;
          8'h02:	aux_pin_state <= 1;

          //delays
          8'h03:	count<=dispatch_data;

          //pass bitwise commands {}[] /\!._-^ to facade
          //pass data to facade
          //TODO: wait for done signal from facade

        endcase
        state_f=`DELAY;
      end
      `DELAY: begin
        if(!count[N]) begin
          count<=count+1;
        end else begin
          state_f<=`RESET;
        end
      end
      `WAIT: begin
        //if(!spi_state) begin
        //  spi_go<=0;
          //state_f<=`DELAY;
        //end //TODO: get data out and back into the FIFO_OUT
      end
    endcase
  end

endmodule
`endif
