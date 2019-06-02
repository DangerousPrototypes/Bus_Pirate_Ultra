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

module top #(
  parameter MC_DATA_WIDTH = 16,
  parameter MC_ADD_WIDTH = 6,
  parameter BP_PINS = 5,
  parameter FIFO_WIDTH = 16,
  parameter FIFO_DEPTH = 4
)(
  input wire clock,
  input wire reset,
  inout wire [BP_PINS-1:0] bufio,
  output wire [BP_PINS-1:0] bufdir, bufod,
  output wire lat_oe,
  input wire [7:0] lat,
  input wire mc_oe, mc_ce, mc_we,
  input wire [MC_ADD_WIDTH-1:0] mc_add,
  inout wire [MC_DATA_WIDTH-1:0] mc_data,
  inout wire irq0, irq1,
  output wire sram_clock, sram0_cs, sram1_cs,
  inout wire [3:0] sram0_sio, sram1_sio
);
    // Tristate pin handling
    // Bus Pirate IO pins
    wire [BP_PINS-1:0] buftoe, buftdo, buftdi;
    // Memory controller interface
    wire [MC_DATA_WIDTH-1:0] mc_din;
    wire [MC_DATA_WIDTH-1:0] mc_dout;
    reg [MC_DATA_WIDTH-1:0] mc_dout_reg;
    // Interrupts
    reg irq0_out, irq1_out;
    wire irq0_in, irq0_dir, irq1_in, irq1_dir;

    // BP IO pin control wires
    wire [BP_PINS-1:0] bp_oe,bp_od,bp_dir,bp_din,bp_dout;

    // Temporary stuff
    wire temp, temp1, temp2, temp3, temp4, temp5;

    // PWM
    //TODO: move to pin module????
    //TODO: N:1 mux freq measure and PWM on IO pins?
    wire pwm_out, pwm_reset;
    reg [15:0] pwm_on, pwm_off;

    //pwm AUX_PWM(reset||pwm_reset, clock,pwm_out, pwm_on,pwm_off);

    //FIFO
    wire in_fifo_in_nempty, in_fifo_in_full, in_fifo_out_nempty,in_fifo_in_shift,in_fifo_out_pop,out_fifo_out_pop;
    wire out_fifo_in_nempty, out_fifo_in_full, out_fifo_out_nempty;
    wire [MC_DATA_WIDTH-1:0] out_fifo_out_data, out_fifo_in_data, in_fifo_out_data;
    wire out_fifo_in_shift, in_fifo_out_clock, out_fifo_in_clock;
    reg [MC_DATA_WIDTH-1:0] reg_io0;

    iobuf BUF[BP_PINS-1:0] (
      //interface
      //TODO: oe master control!
      .oe(1'b1), //bp_oe//output enable 1=true
      .od(reg_io0[12:8]), //open drain 1=true
      .dir(reg_io0[4:0]),//direction 1=input
      .din(bp_din),//data in (value when buffer is output)
      .dout(bp_dout),//data out (value when buffer is input)
      //hardware driver
      .bufdir(bufdir), //74LVC1T45 DIR pin LOW for Hi-Z
      .bufod(bufod), //74LVC1G07 OD pin HIGH for Hi-Z
      .bufdat_tristate_oe(buftoe), //tristate data pin output enable
      .bufdat_tristate_dout(buftdo), //tristate data pin data out
      .bufdat_tristate_din(buftdi)  //tristate data pin data in
      );

    //wires tied to the memory controller WE and OE signals
    wire mc_we_sync, mc_oe_sync;
    sync MC_WE_SYNC(clock, mc_we, mc_we_sync);
    sync MC_OE_SYNC(clock, mc_oe, mc_oe_sync);
    assign in_fifo_in_shift=(mc_add===6'h00)?mc_we_sync:1'b0; //follow the we signal
    assign mc_dout=(mc_add===6'h00&&!mc_oe)?out_fifo_out_data:mc_dout_reg;
    assign out_fifo_out_pop=(mc_add===6'h00)?mc_oe_sync:1'b0;//follow the oe signal
    assign pwm_reset=(mc_add===6'h1a)?!mc_we_sync:1'b0; //reset pwm counters after write to pwm

    initial begin
      reg_io0=16'b0000000000000100;
    end

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



        fifo #(
          .WIDTH(FIFO_WIDTH),
          .DEPTH(FIFO_DEPTH)
          ) FIFO_IN (
        	.in_clock(clock),
        	.in_shift(in_fifo_in_shift),
        	.in_data(mc_din),
        	.in_full(in_fifo_in_full),
        	.in_nempty(in_fifo_in_nempty),

          .out_clock(in_fifo_out_clock),
        	.out_pop(in_fifo_out_pop),
        	.out_data(in_fifo_out_data),
        	.out_nempty(in_fifo_out_nempty)
        ), FIFO_OUT (
          .in_clock(out_fifo_in_clock),
        	.in_shift(out_fifo_in_shift), //???
        	.in_data(out_fifo_in_data), // in data
        	.in_full(out_fifo_in_full), //output
        	.in_nempty(out_fifo_in_nempty), //output

          .out_clock(clock),
        	.out_pop(out_fifo_out_pop), //input out_pop,
        	.out_data(out_fifo_out_data), //out data,
        	.out_nempty(out_fifo_out_nempty) //output reg out_nempty
        );

        dispatch #(
          .FIFO_WIDTH(FIFO_WIDTH)
        ) DISPATCH (
          clock,
          reset,
          //input from fifo and trigger
          in_fifo_out_clock,
          in_fifo_out_nempty,
          in_fifo_out_pop,
          in_fifo_out_data,
          //output to fifo and triggers
          out_fifo_in_clock,
          out_fifo_in_full,
          out_fifo_in_shift,
          out_fifo_in_data,
          // pins (directions???)
          bp_din,
          bp_dout
          //error?
          //idle/busy?
        );

    //define the tristate data pin explicitly in the top module
    // Bus Pirate IO pins
    SB_IO #(
      .PIN_TYPE(6'b1010_01),
      .PULLUP(1'b0)
    ) bp_io [BP_PINS-1:0] (
      .PACKAGE_PIN(bufio),
      .OUTPUT_ENABLE(buftoe),
      .D_OUT_0(buftdo),
      .D_IN_0(buftdi)
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
