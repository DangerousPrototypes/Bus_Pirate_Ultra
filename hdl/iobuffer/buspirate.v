//------------------------------------------------------------------
//-- Bus Pirate peripheral tests
//--
//------------------------------------------------------------------
`include "iobuf.v"
`include "iobufphy.v"
`include "synchronizer.v"
`include "pwm.v"

module top #(
  parameter MC_DATA_WIDTH = 16,
  parameter MC_ADD_WIDTH = 6,
  parameter LA_WIDTH = 8,
  parameter LA_CHIPS = 2,
  parameter BP_PINS = 5,
  parameter FIFO_WIDTH = 16,
  parameter FIFO_DEPTH = 4
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
    // memory regs
    reg [MC_DATA_WIDTH-1:0] register [32:0];
    // Tristate pin handling
    // Bus Pirate IO pins
    wire [BP_PINS-1:0] bpio_toe, bpio_tdo, bpio_tdi;

    // BP IO pin control wires
    wire [BP_PINS-1:0] bpio_oe,bpio_di,bpio_do;

    `define reg_bpio_od register[3][BP_PINS-1+8:8]
    `define reg_bpio_dir register[3][BP_PINS-1:0]

    iobuf BPIO_BUF[BP_PINS-1:0] (
      //interface
      //TODO: oe master control!
      .oe(1'b1), //bp_oe//output enable 1=true
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
    `define reg_pwm_on register[25]
    `define reg_pwm_off register [26]
    pwm AUX_PWM(pwm_reset, clock,pwm_out, `reg_pwm_on,`reg_pwm_off);

    assign bpio_do=pwm_out;

  	// Memory controller interface
  	wire [MC_DATA_WIDTH-1:0] mc_din;
    reg [MC_DATA_WIDTH-1:0] mc_dout_d;
    wire [MC_DATA_WIDTH-1:0] mc_dout;
    assign mc_dout=mc_dout_d;

    localparam N = 23;
    reg [N:0] count;
    wire bpio_data_oe;
    wire bpio_data_dout;
    wire bpio_data_din;
    wire dout;

    reg [9:0] sample_counter;

    wire [LA_WIDTH-1:0] sram_sio_tdi;
    wire [LA_WIDTH-1:0] sram_sio_tdo;

    `define reg_la_io_quad register[2][0]
    `define reg_la_io_quad_direction register[2][1]
    `define reg_la_io_cs0 register[2][8] //reserve upper bits for more SRAMs
    `define reg_la_io_cs1 register[2][9]

    //register test:
    //straight through SPI
    //reg [LA_WIDTH-1:0] sram_sio_tdo_d;
    wire sram_clock_source;
    //assign sram_clock_source=`reg_la_io_quad?(`reg_la_io_quad_direction?mc_we_sync:mc_oe_sync):mcu_clock;
    assign sram_clock_source=mcu_clock;
    assign sram_clock={sram_clock_source,sram_clock_source};
    assign sram_cs={`reg_la_io_cs0,`reg_la_io_cs1};
    assign sram_sio_tdo[0]=`reg_la_io_quad?register[1][0]:mcu_mosi;
    assign sram_sio_tdo[4]=`reg_la_io_quad?register[1][4]:mcu_mosi;
    assign {sram_sio_tdo[7:5],sram_sio_tdo[3:1]}={register[1][7:5],register[1][3:1]};
    assign mcu_miso=`reg_la_io_cs1?sram_sio_tdi[5]:sram_sio_tdi[1]; //very hack dont like

    //LATCH OE
    assign lat_oe=1'b0;

    //wires tied to the memory controller WE and OE signals
    wire mc_we_sync,mc_oe_sync,mc_ce_sync;
    sync MC_WE_SYNC(clock, mc_we, mc_we_sync);
    sync MC_OE_SYNC(clock, mc_oe, mc_oe_sync);
    sync MC_CE_SYNC(clock, mc_ce, mc_ce_sync);

    always @(posedge clock)
      begin
        count <= count + 1;
        pwm_reset<=1'b0;
        if(!mc_ce)
        begin
          if ((!mc_we))			// write (proper)
          begin
            case(mc_add)
              16'h0019,16'h001a:begin
                pwm_reset<=1'b1;
              end
            endcase
            register [mc_add] <= mc_din;
          end
          else if ((mc_we))		// read
          begin
            case(mc_add)
              16'h0001: register[1][7:0]<=sram_sio_tdi; //this doesnt work... when does register get put on the outputs?>???
              default: mc_dout_d <= register [mc_add];
            endcase
          end
        end
      end



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
/*
    SB_IO #(
      .PIN_TYPE(6'b1010_01), //tristate
      .PULLUP(1'b0)          //no pullup
    ) bpio_datapin (
      .PACKAGE_PIN(bpio_io),//which pin
      .OUTPUT_ENABLE(bpio__data_oe),   //output enable wire
      .D_OUT_0(bpio__data_dout),        //data out wire
      .D_IN_0(bpio__data_din)           //data in wire
    );
*/
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




    initial begin
      register[6'b00000] <= 16'b0000000000000000;				// test values
      register[6'b00001] <= 16'b0000000000000000;
      register[6'b00010] <= 16'b0000000000000000;
      register[6'b00011] <= 16'b0000000000000000;
      register[6'b00100] <= 16'b0000000000000000;
      register[6'b00101] <= 16'b0000000000000000;
      register[6'b00110] <= 16'b0000000000000000;
      register[6'b00111] <= 16'b0000000000000000;
      register[6'b01000] <= 16'b0000000000000000;
      register[6'b01001] <= 16'b0000000000000000;
      register[6'b01010] <= 16'b0000000000000000;
      register[6'b01011] <= 16'b0000000000000000;
      register[6'b01100] <= 16'b0000000000000000;
      register[6'b01101] <= 16'b0000000000000000;
      register[6'b01110] <= 16'b0000000000000000;
      register[6'b01111] <= 16'b0000000000000000;
      register[6'b10000] <= 16'b0000000000000000;
      register[6'b10001] <= 16'b0000000000000000;
      register[6'b10010] <= 16'b0000000000000000;
      register[6'b10011] <= 16'b0000000000000000;
      register[6'b10100] <= 16'b0000000000000000;
      register[6'b10101] <= 16'b0000000000000000;
      register[6'b10110] <= 16'b0000000000000000;
      register[6'b10111] <= 16'b0000000000000000;
      register[6'b11000] <= 16'b0000000000000000;
      register[6'b11001] <= 16'b0000000000000000;
      register[6'b11010] <= 16'b0000000000000000;
      register[6'b11011] <= 16'b0000000000000000;
      register[6'b11100] <= 16'b0000000000000000;
      register[6'b11101] <= 16'b0000000000000000;
      register[6'b11110] <= 16'b0000000000000000;
      register[6'b11111] <= 16'b0000000000000000;
    end

endmodule
