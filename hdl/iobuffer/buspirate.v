//------------------------------------------------------------------
//-- Bus Pirate peripheral tests
//--
//------------------------------------------------------------------
`include "iobuf.v"
`include "iobufphy.v"
`include "synchronizer.v"
module top #(
  parameter MC_DATA_WIDTH = 16,
  parameter MC_ADD_WIDTH = 6,
  parameter LA_WIDTH = 8,
  parameter LA_CHIPS = 2

) (
  input clock,
  inout bufio,
  output wire bufdir,
  output wire bufod,
  output wire[LA_CHIPS-1:0] sram_clock,
  output wire[LA_CHIPS-1:0] sram_cs,
  inout[LA_WIDTH-1:0] sram_sio,
  output wire lat_oe,
  input wire [LA_WIDTH-1:0] lat,
  //input wire mcu_quadmode, mcu_direction,
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
  	// Memory controller interface
  	wire [MC_DATA_WIDTH-1:0] mc_din;
    reg [MC_DATA_WIDTH-1:0] mc_dout_d;
    wire [MC_DATA_WIDTH-1:0] mc_dout;
    assign mc_dout=mc_dout_d;

    localparam N = 23;
    reg [N:0] count;
    wire buf_data_oe;
    wire buf_data_dout;
    wire buf_data_din;
    wire dout;

    reg [9:0] sample_counter;

    wire [LA_WIDTH-1:0] sram_sio_tdi;
    wire [LA_WIDTH-1:0] sram_sio_tdo;

    //                  oe      od  dir din dout       bufdir bufod      the pins from the SB_IO block below
    iobuf MOSI_BUF(1'b1, 1'b0, 1'b0, count[N],dout,    bufdir,  bufod,  buf_data_oe,buf_data_dout,buf_data_din);


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
        if(!mc_ce)
        begin
          if ((!mc_we))			// write (proper)
          begin
            case(mc_add)
              default:register [mc_add] <= mc_din; //16'hFFFF;
            endcase
          end
          else if ((mc_we))		// read
          begin
            case(mc_add)
              16'h0001: register[1][7:0]<=sram_sio_tdi;
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

    SB_IO #(
      .PIN_TYPE(6'b1010_01), //tristate
      .PULLUP(1'b0)          //no pullup
    ) bufdatapin (
      .PACKAGE_PIN(bufio),//which pin
      .OUTPUT_ENABLE(buf_data_oe),   //output enable wire
      .D_OUT_0(buf_data_dout),        //data out wire
      .D_IN_0(buf_data_din)           //data in wire
    );




    initial begin
      register[6'b00000] <= 16'b0101101001011010;				// test values
      register[6'b00001] <= 16'b1010010110100101;
      register[6'b00010] <= 16'b0101101001011010;
      register[6'b00011] <= 16'b1010010110100101;
      register[6'b00100] <= 16'b0101101001011010;
      register[6'b00101] <= 16'b1010010110100101;
      register[6'b00110] <= 16'b0101101001011010;
      register[6'b00111] <= 16'b1010010110100101;
      register[6'b01000] <= 16'b0101101001011010;
      register[6'b01001] <= 16'b1010010110100101;
      register[6'b01010] <= 16'b0101101001011010;
      register[6'b01011] <= 16'b1010010110100101;
      register[6'b01100] <= 16'b0101101001011010;
      register[6'b01101] <= 16'b1010010110100101;
      register[6'b01110] <= 16'b0101101001011010;
      register[6'b01111] <= 16'b1010010110100101;
      register[6'b10000] <= 16'b0101101001011010;
      register[6'b10001] <= 16'b1010010110100101;
      register[6'b10010] <= 16'b0101101001011010;
      register[6'b10011] <= 16'b1010010110100101;
      register[6'b10100] <= 16'b0101101001011010;
      register[6'b10101] <= 16'b1010010110100101;
      register[6'b10110] <= 16'b0101101001011010;
      register[6'b10111] <= 16'b1010010110100101;
      register[6'b11000] <= 16'b0101101001011010;
      register[6'b11001] <= 16'b1010010110100101;
      register[6'b11010] <= 16'b0101101001011010;
      register[6'b11011] <= 16'b1010010110100101;
      register[6'b11100] <= 16'b0101101001011010;
      register[6'b11101] <= 16'b1010010110100101;
      register[6'b11110] <= 16'b0101101001011010;
      register[6'b11111] <= 16'b1010010110100101;
    end

endmodule
