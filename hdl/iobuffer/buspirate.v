//------------------------------------------------------------------
//-- Bus Pirate peripheral tests
//--
//------------------------------------------------------------------
`include "iobuf.v"
`include "iobufphy.v"
`include "synchronizer.v"
`include "pwm.v"
`include "pll.v"
`include "registers.v"
`include "spimaster.v"
`include "fifo.v"
//`include "ram.v"
`define SIMULATION

module top #(
  parameter MC_DATA_WIDTH = 16,
  parameter MC_ADD_WIDTH = 6,
  parameter LA_WIDTH = 8,
  parameter LA_CHIPS = 2,
  parameter BP_PINS = 5,
  parameter FIFO_WIDTH = 16,
  parameter FIFO_DEPTH = 512
) (
  input clock,
  inout wire [BP_PINS-1:0] bpio_io,
  output wire [BP_PINS-1:0] bpio_dir, bpio_od,
  output wire[LA_CHIPS-1:0] sram_clock,
  output wire[LA_CHIPS-1:0] sram_cs,
  inout[LA_WIDTH-1:0] sram_sio,
  output wire lat_oe,
  input wire [LA_WIDTH-1:0] lat,
  input wire mcu_clock,
  input wire mcu_cs,
  input wire mcu_mosi, //sio0
  output wire mcu_miso, //sio1
  input wire mc_oe, mc_ce, mc_we,
  input wire [MC_ADD_WIDTH-1:0] mc_add,
  inout [MC_DATA_WIDTH-1:0] mc_data
  );
    //pll PLL_2F(clock,pll_clk,locked);
    //wires tied to the memory controller WE and OE signals
    wire mc_we_sync,mc_oe_sync,mc_ce_sync;
    sync MC_WE_SYNC(clock, mc_we, mc_we_sync);
    sync MC_OE_SYNC(clock, mc_oe, mc_oe_sync);
    sync MC_CE_SYNC(clock, mc_ce, mc_ce_sync);
    // memory regs
    reg [MC_DATA_WIDTH-1:0] wreg [32:0];
    reg [MC_DATA_WIDTH-1:0] rreg [32:0];
    // Tristate pin handling
    // Bus Pirate IO pins
    wire [BP_PINS-1:0] bpio_toe, bpio_tdo, bpio_tdi;
    // BP IO pin control wires
    wire [BP_PINS-1:0] bpio_oe,bpio_di,bpio_do;

    iobuf BPIO_BUF[BP_PINS-1:0] (
      //interface
      .oe(`reg_bpio_oe), //bp_oe//output enable 1=true
      .od(`reg_bpio_od), //open drain 1=true
      .dir(`reg_bpio_dir),//direction 1=input
      .din(bpio_do),//data in (value when buffer is output)
      .dout(bpio_di),//data out (value when buffer is input)
      //hardware driver
      .bufdir(bpio_dir), //74LVC1T45 DIR pin LOW for Hi-Z
      .bufod(bpio_od), //74LVC1G07 OD pin HIGH for Hi-Z
      .bufdat_tristate_oe(bpio_toe), //tristate data pin output enable
      .bufdat_tristate_dout(bpio_tdo), //tristate data pin data out
      .bufdat_tristate_din(bpio_tdi)  //tristate data pin data in
      );

    // PWM
    //TODO: move to pin module????
    //TODO: N:1 mux freq measure and PWM on IO pins?
    wire pwm_out;
    reg pwm_reset;
    pwm PWM_OUT(pwm_reset, count[0],pwm_out, `reg_pwm_on,`reg_pwm_off);
    //assign bpio_do[0]=pwm_out;
    //assign bpio_do[1]=pwm_out;
    //assign bpio_do[2]=pwm_out;
    //assign bpio_do[3]=pwm_out;
    assign bpio_do[4]=pwm_out;

  	// Memory controller interface
  	wire [MC_DATA_WIDTH-1:0] mc_din;
    reg [MC_DATA_WIDTH-1:0] mc_dout_d;
    wire [MC_DATA_WIDTH-1:0] mc_dout;
    assign mc_dout=mc_dout_d;

    //LATCH OE
    assign lat_oe=1'b0; //open latch, eleminated on next revision
    localparam N = 3;
    reg [N:0] count;
    reg [2:0] reset_count;
    //1111 0100 0010 0100 0000 0000
    reg [7:0] la_sample_prescaler;

    wire [LA_WIDTH-1:0] sram_sio_tdi;
    wire [LA_WIDTH-1:0] sram_sio_tdo;

    reg la_active;
    wire sram_clock_source;
    reg sram_auto_clock, sram_auto_clock_delay;
    assign sram_clock_source=(`reg_la_start&&`reg_la_active)?clock:(`reg_la_io_quad)?sram_auto_clock:(`reg_la_io_spi)?mcu_clock:1'b0;
    assign sram_clock={sram_clock_source,sram_clock_source};
    assign sram_cs={`reg_la_io_cs1,`reg_la_io_cs0}; //TODO: hold CS low during active?
    assign sram_sio_tdo[0]=(`reg_la_start&&`reg_la_active)?lat[0]:`reg_la_io_quad?`reg_la_write[0]:mcu_mosi;
    assign sram_sio_tdo[4]=(`reg_la_start&&`reg_la_active)?lat[4]:`reg_la_io_quad?`reg_la_write[4]:mcu_mosi;
    assign {sram_sio_tdo[7:5],sram_sio_tdo[3:1]}=(`reg_la_start&&`reg_la_active)?{lat[7:5],lat[3:1]}:{`reg_la_write[7:5],`reg_la_write[3:1]};
    assign mcu_miso=!`reg_la_io_cs0?sram_sio_tdi[1]:sram_sio_tdi[5]; //very hack dont like

    //for simulation debugging...
    `ifdef SIMULATION
      wire [15:0] FPGA_REG_03;
      assign FPGA_REG_03=wreg[6'h03];
      wire [15:0] FPGA_REG_04;
      assign FPGA_REG_04=rreg[6'h04];
    `endif

    //FIFO

    wire out_fifo_in_nempty, out_fifo_in_full, out_fifo_out_nempty;
    wire [MC_DATA_WIDTH-1:0] out_fifo_out_data, out_fifo_in_data;
    wire out_fifo_in_shift, in_fifo_out_clock, out_fifo_in_clock;

    //in use
    wire [MC_DATA_WIDTH-1:0] in_fifo_out_data;
    wire in_fifo_in_nempty, in_fifo_in_full, in_fifo_out_nempty;//, in_fifo_out_pop;
    reg in_fifo_in_shift, peripheral_busy;//,in_fifo_out_pop;
    //assign in_fifo_out_pop=in_fifo_out_nempty && (peripheral_busy===1'b0);
    reg in_fifo_out_pop;

    reg in_fifo_out_pop_last, state_last, reset, peripheral_start;
    wire in_fifo_out_data_ready, out_fifo_in_data_ready, state;

    fifo #(
    .WIDTH(FIFO_WIDTH),
    .DEPTH(FIFO_DEPTH)
    ) FIFO_IN (
    .in_clock(clock),
    .in_shift(in_fifo_in_shift),
    .in_data(mc_din),
    .in_full(in_fifo_in_full),
    .in_nempty(in_fifo_in_nempty),

    .out_clock(clock),
    .out_pop(out_fifo_in_data_ready),
    .out_data(in_fifo_out_data),
    .out_nempty(in_fifo_out_nempty)
    );


    // SPI master
    //TODO: move CS, delay, start logic analyzer commands to FIFO and process in top level
     wire[7:0] spi_in;
     //assign in_fifo_out_data_ready=((in_fifo_out_pop_last===1'b1)&&(in_fifo_out_pop===1'b0));
     assign out_fifo_in_data_ready=((state_last===1'b1)&&(state===1'b0));
     spimaster SPI_MASTER(
     // general control
       .rst(reset),				// resets module to known state
       .clkin(clock),				// clock that makes everyhting tick
     // spi configuration
       .cpol(1'b1), //cpol,				// clock polarity
       .cpha(1'b0), //cpha,				// clock phase
       .cspol(1'b1), //cspol,				// CS polarity
       .autocs(1'b0), //autocs,				// assert CS automatically
     // sync signals
       .go(peripheral_start),					// starts a SPI transmission
       .state(state),				// state of module (0=idle, 1=busy/transmitting)
     // data in/out
       .data_i(in_fifo_out_data), 			// data in (will get transmitted)
       .data_o(spi_in),				// data out (will get received)
     // spi signals
       .mosi(bpio_do[0]),          // master out slave in
       .sclk(bpio_do[1]),          // SPI clock (= clkin/2)
       .miso(bpio_di[2]),          // master in slave out
       .cs(bpio_do[3])				     // chip select
       );



    always @(posedge clock)
      begin

        if(reset_count<2) begin
          reset_count<=reset_count+1;
          reset<=1'b1;
        end
        else
        begin
          reset<=1'b0;
        end

        count <= count + 1;
        pwm_reset<=1'b0;
        state_last<=state;
        in_fifo_out_pop_last<=in_fifo_out_pop;

        in_fifo_in_shift<=1'b0;
        peripheral_start<=1'b0;
        in_fifo_out_pop<=1'b0;
        `reg_fifo_status_full<=state;

        `reg_fifo_out_test<=in_fifo_out_data;
        `reg_fifo_status_full<=in_fifo_in_full;
        `reg_fifo_status_nempty<=in_fifo_in_nempty;
        `reg_fifo_status_out_nempty<=in_fifo_out_nempty;

        if(out_fifo_in_data_ready)
        begin //TODO: put in out fifo...
          `reg_fifo_out[7:0]<=spi_in;
        end

        if(!peripheral_busy && in_fifo_out_nempty && !state)
        begin
          peripheral_start<=1'b1;
          peripheral_busy<=1'b1;
        end
        else if(!peripheral_start && !state)
        begin
          peripheral_busy<=1'b0;
        end

        if(sram_auto_clock_delay)
        begin
          sram_auto_clock_delay<=1'b0;
          sram_auto_clock<=1'b1;
        end
        else if(sram_auto_clock)
        begin
          sram_auto_clock<=1'b0;
        end

        //TODO:
        //prescale to 0xff and capture to nearest 0xff
        if(`reg_la_clear_sample_counter)
        begin
          `reg_la_sample_count<=16'h0000;
          `reg_la_clear_sample_counter<=16'h0000;
          `reg_la_max_samples_reached<=1'b0;
        end
        else if(`reg_la_start)
        begin
          if(`reg_la_sample_count<16'hF424) //F424
          begin
            `reg_la_sample_count<=`reg_la_sample_count+1;
            `reg_la_active<=1'b1;
          end
          else
            begin
              `reg_la_active<=1'b0;
              `reg_la_max_samples_reached<=1'b1;
            end
        end



        if(!mc_ce)
        begin

          if (mc_we_sync)			// write

          begin
            wreg[mc_add] <= mc_din;
            case(mc_add)
              6'h02:sram_auto_clock_delay<=1'b1;
              6'h06:pwm_reset<=1'b1;
              6'h07:in_fifo_in_shift<=1'b1;
              default: begin end
            endcase
          end

          else if (mc_oe_sync)		// read

          begin
            //mc_dout_d <= rreg[mc_add];
            case(mc_add)
              6'h02:begin
                sram_auto_clock_delay<=1'b1; //is this really stable? don't we need delay betwee the clock and the reading of the results?
                mc_dout_d <= {8'h00,sram_sio_tdi}; //should move to if clock=1 in a clock delay?
              end
              6'h03: mc_dout_d<=`reg_la_config;
              default:mc_dout_d <= rreg[mc_add];
            endcase
          end
          /*else if (sram_auto_clock)begin
            sram_auto_clock<=1'b0;
            mc_dout_d <= {8'h00,sram_sio_tdi};
          end*/
        end// if we or oe
      end //if ce


    //define the tristate data pin explicitly in the top module
    //register MOSI
    SB_IO #(
      .PIN_TYPE(6'b1010_01), //tristate
      .PULLUP(1'b0)          //no pullup
    ) sram_mosi_sbio[LA_CHIPS-1:0] (
      .PACKAGE_PIN({sram_sio[4],sram_sio[0]}),//which pin
      .OUTPUT_ENABLE(`reg_la_io_quad?`reg_la_io_quad_direction:1'b1),   //output enable wire mcu_quadmode?mcu_direction:1'b1
      .D_OUT_0({sram_sio_tdo[4],sram_sio_tdo[0]}),        //data out wire
      .D_IN_0({sram_sio_tdi[4],sram_sio_tdi[0]})           //data in wire
    );
    //register MISO
    SB_IO #(
			.PIN_TYPE(6'b1010_01), //tristate
			.PULLUP(1'b0)          //no pullup
		) sram_miso_sbio[LA_CHIPS-1:0](
			.PACKAGE_PIN({sram_sio[5],sram_sio[1]}),//which pin
			.OUTPUT_ENABLE(`reg_la_io_quad?`reg_la_io_quad_direction:1'b0),   //output enable wire mcu_quadmode?mcu_direction:1'b0
			.D_OUT_0({sram_sio_tdo[5],sram_sio_tdo[1]}),        //data out wire
			.D_IN_0({sram_sio_tdi[5],sram_sio_tdi[1]})           //data in wire
		);
    //register SIO2/3
    SB_IO #(
			.PIN_TYPE(6'b1010_01), //tristate
			.PULLUP(1'b0)          //no pullup
		) sram_sio_sbio[3:0] (
			.PACKAGE_PIN({sram_sio[7:6],sram_sio[3:2]}),//which pin
			.OUTPUT_ENABLE(`reg_la_io_quad&&`reg_la_io_quad_direction),   //quadmode = 1 and direction = 1
			.D_OUT_0({sram_sio_tdo[7:6],sram_sio_tdo[3:2]}),        //data out wire
			.D_IN_0({sram_sio_tdi[7:6],sram_sio_tdi[3:2]})           //data in wire
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
    // Bus Pirate IO pins
    SB_IO #(
      .PIN_TYPE(6'b1010_01),
      .PULLUP(1'b0)
    ) bpio_tio [BP_PINS-1:0] (
      .PACKAGE_PIN(bpio_io),
      .OUTPUT_ENABLE(bpio_toe),
      .D_OUT_0(bpio_tdo),
      .D_IN_0(bpio_tdi)
    );

`ifdef SIMULATION
    initial begin
      peripheral_busy<=1'b0;
      reset<=1'b0;
      count<=3'b000;
      la_active<=0;
      reset_count<=0;
      rreg[6'b00000] <= 16'b0000000000000000;				// test values
      rreg[6'b00001] <= 16'b0000000000000000;
      rreg[6'b00010] <= 16'b0000000000000000;
      rreg[6'b00011] <= 16'b0000000000000000;
      rreg[6'b00100] <= 16'b0000000000000000;
      rreg[6'b00101] <= 16'b0000000000000000;
      rreg[6'b00110] <= 16'b0000000000000000;
      rreg[6'b00111] <= 16'b0000000000000000;
      rreg[6'b01000] <= 16'b0000000000000000;
      rreg[6'b01001] <= 16'b0000000000000000;
      rreg[6'b01010] <= 16'b0000000000000000;
      rreg[6'b01011] <= 16'b0000000000000000;
      rreg[6'b01100] <= 16'b0000000000000000;
      rreg[6'b01101] <= 16'b0000000000000000;
      rreg[6'b01110] <= 16'b0000000000000000;
      rreg[6'b01111] <= 16'b0000000000000000;
      rreg[6'b10000] <= 16'b0000000000000000;
      rreg[6'b10001] <= 16'b0000000000000000;
      rreg[6'b10010] <= 16'b0000000000000000;
      rreg[6'b10011] <= 16'b0000000000000000;
      rreg[6'b10100] <= 16'b0000000000000000;
      rreg[6'b10101] <= 16'b0000000000000000;
      rreg[6'b10110] <= 16'b0000000000000000;
      rreg[6'b10111] <= 16'b0000000000000000;
      rreg[6'b11000] <= 16'b0000000000000000;
      rreg[6'b11001] <= 16'b0000000000000000;
      rreg[6'b11010] <= 16'b0000000000000000;
      rreg[6'b11011] <= 16'b0000000000000000;
      rreg[6'b11100] <= 16'b0000000000000000;
      rreg[6'b11101] <= 16'b0000000000000000;
      rreg[6'b11110] <= 16'b0000000000000000;
      rreg[6'b11111] <= 16'b0000000000000000;

      wreg[6'b00000] <= 16'b0000000000000000;				// test values
      wreg[6'b00001] <= 16'b0000000000000000;
      wreg[6'b00010] <= 16'b0000000000000000;
      wreg[6'b00011] <= 16'b0000000000000000;
      wreg[6'b00100] <= 16'b0000000000000000;
      wreg[6'b00101] <= 16'b0000000000000000;
      wreg[6'b00110] <= 16'b0000000000000000;
      wreg[6'b00111] <= 16'b0000000000000000;
      wreg[6'b01000] <= 16'b0000000000000000;
      wreg[6'b01001] <= 16'b0000000000000000;
      wreg[6'b01010] <= 16'b0000000000000000;
      wreg[6'b01011] <= 16'b0000000000000000;
      wreg[6'b01100] <= 16'b0000000000000000;
      wreg[6'b01101] <= 16'b0000000000000000;
      wreg[6'b01110] <= 16'b0000000000000000;
      wreg[6'b01111] <= 16'b0000000000000000;
      wreg[6'b10000] <= 16'b0000000000000000;
      wreg[6'b10001] <= 16'b0000000000000000;
      wreg[6'b10010] <= 16'b0000000000000000;
      wreg[6'b10011] <= 16'b0000000000000000;
      wreg[6'b10100] <= 16'b0000000000000000;
      wreg[6'b10101] <= 16'b0000000000000000;
      wreg[6'b10110] <= 16'b0000000000000000;
      wreg[6'b10111] <= 16'b0000000000000000;
      wreg[6'b11000] <= 16'b0000000000000000;
      wreg[6'b11001] <= 16'b0000000000000000;
      wreg[6'b11010] <= 16'b0000000000000000;
      wreg[6'b11011] <= 16'b0000000000000000;
      wreg[6'b11100] <= 16'b0000000000000000;
      wreg[6'b11101] <= 16'b0000000000000000;
      wreg[6'b11110] <= 16'b0000000000000000;
      wreg[6'b11111] <= 16'b0000000000000000;


    end
  `endif

endmodule
