//------------------------------------------------------------------
//-- Bus Pirate peripheral tests
//--
//------------------------------------------------------------------
`include "iobuf.v"
`include "iobufphy.v"
module top #(
  parameter MC_DATA_WIDTH = 16,
  parameter MC_ADD_WIDTH = 6
) (
  input clock,
  inout bufio,
  output wire bufdir,
  output wire bufod,
  output wire sram0_clock,
  output wire sram0_cs,
  inout[3:0] sram0_sio,
  input wire mcu_quadmode, mcu_direction,
  input wire mcu_clock,
  input wire mcu_cs,
  input wire mcu_mosi, //sio0
  output wire mcu_miso, //sio1
  output wire lat_oe,
  input wire [3:0] lat,

  input wire mc_oe, mc_ce, mc_we,
  input wire [MC_ADD_WIDTH-1:0] mc_add,
  inout [MC_DATA_WIDTH-1:0] mc_data
  );
    // memory regs
    reg [MC_DATA_WIDTH-1:0] SRAM [32:0];
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

    wire [3:0] sram0_sio_tdi;
    wire [3:0] sram0_sio_tdo;

    //                  oe      od  dir din dout       bufdir bufod      the pins from the SB_IO block below
    iobuf MOSI_BUF(1'b1, 1'b0, 1'b0, count[N],dout,    bufdir,  bufod,  buf_data_oe,buf_data_dout,buf_data_din);

    //SRAM test:
    //straight through SPI
    reg [3:0] sram0_sio_tdo_d;
    assign sram0_clock=mcu_clock;
    assign sram0_cs=mcu_cs;
    assign sram0_sio_tdo[0]=mcu_quadmode?SRAM[1][0]:mcu_mosi;
    assign sram0_sio_tdo[3:1]=SRAM[1][3:1];
    //assign sram0_sio_tdo=4'b0000;
    assign mcu_miso=sram0_sio_tdi[1];
    //assign sram0_sio_tdo=SRAM[1][3:0];

    //LATCH OE
    assign lat_oe=1'b0;
    //assign mcu_sio_tdo=(mcu_arm) ? lat : sram0_sio_tdi;
    //assign sram0_sio_tdo=(mcu_arm) ? lat : mcu_sio_tdi;


    always @(posedge clock)
      begin
        count <= count + 1;
        //SRAM[1][3:0] <= sram0_sio_tdi;
        if(!mc_ce)
        begin
          if ((!mc_we))			// write (proper)
          begin
            SRAM [mc_add] <= mc_din; //16'hFFFF;
          end
          else if ((mc_we))		// read
          begin
            mc_dout_d <= SRAM [mc_add];
          end
        end
        else if(mcu_quadmode)
        begin
          if(!mcu_direction)
          begin
            SRAM[1][3:0]<=sram0_sio_tdi;
          end
        end
      end



    //define the tristate data pin explicitly in the top module
    //SRAM MOSI
    SB_IO #(
      .PIN_TYPE(6'b1010_01), //tristate
      .PULLUP(1'b0)          //no pullup
    ) sram0mosi (
      .PACKAGE_PIN(sram0_sio[0]),//which pin
      .OUTPUT_ENABLE(mcu_quadmode?mcu_direction:1'b1),   //output enable wire mcu_quadmode?mcu_direction:1'b1
      .D_OUT_0(sram0_sio_tdo[0]),        //data out wire
      .D_IN_0(sram0_sio_tdi[0])           //data in wire
    );
    //SRAM MISO
    SB_IO #(
			.PIN_TYPE(6'b1010_01), //tristate
			.PULLUP(1'b0)          //no pullup
		) sram0miso(
			.PACKAGE_PIN(sram0_sio[1]),//which pin
			.OUTPUT_ENABLE(mcu_quadmode?mcu_direction:1'b0),   //output enable wire mcu_quadmode?mcu_direction:1'b0
			.D_OUT_0(sram0_sio_tdo[1]),        //data out wire
			.D_IN_0(sram0_sio_tdi[1])           //data in wire
		);
    //SRAM SIO2/3
    SB_IO #(
			.PIN_TYPE(6'b1010_01), //tristate
			.PULLUP(1'b0)          //no pullup
		) sram0sio23[1:0] (
			.PACKAGE_PIN(sram0_sio[3:2]),//which pin
			.OUTPUT_ENABLE(mcu_quadmode&&mcu_direction ),   //quadmode = 1 and direction = 1
			.D_OUT_0(sram0_sio_tdo[3:2]),        //data out wire
			.D_IN_0(sram0_sio_tdi[3:2])           //data in wire
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
      SRAM[6'b00000] <= 16'b0101101001011010;				// test values
      SRAM[6'b00001] <= 16'b1010010110100101;
      SRAM[6'b00010] <= 16'b0101101001011010;
      SRAM[6'b00011] <= 16'b1010010110100101;
      SRAM[6'b00100] <= 16'b0101101001011010;
      SRAM[6'b00101] <= 16'b1010010110100101;
      SRAM[6'b00110] <= 16'b0101101001011010;
      SRAM[6'b00111] <= 16'b1010010110100101;
      SRAM[6'b01000] <= 16'b0101101001011010;
      SRAM[6'b01001] <= 16'b1010010110100101;
      SRAM[6'b01010] <= 16'b0101101001011010;
      SRAM[6'b01011] <= 16'b1010010110100101;
      SRAM[6'b01100] <= 16'b0101101001011010;
      SRAM[6'b01101] <= 16'b1010010110100101;
      SRAM[6'b01110] <= 16'b0101101001011010;
      SRAM[6'b01111] <= 16'b1010010110100101;
      SRAM[6'b10000] <= 16'b0101101001011010;
      SRAM[6'b10001] <= 16'b1010010110100101;
      SRAM[6'b10010] <= 16'b0101101001011010;
      SRAM[6'b10011] <= 16'b1010010110100101;
      SRAM[6'b10100] <= 16'b0101101001011010;
      SRAM[6'b10101] <= 16'b1010010110100101;
      SRAM[6'b10110] <= 16'b0101101001011010;
      SRAM[6'b10111] <= 16'b1010010110100101;
      SRAM[6'b11000] <= 16'b0101101001011010;
      SRAM[6'b11001] <= 16'b1010010110100101;
      SRAM[6'b11010] <= 16'b0101101001011010;
      SRAM[6'b11011] <= 16'b1010010110100101;
      SRAM[6'b11100] <= 16'b0101101001011010;
      SRAM[6'b11101] <= 16'b1010010110100101;
      SRAM[6'b11110] <= 16'b0101101001011010;
      SRAM[6'b11111] <= 16'b1010010110100101;
    end

endmodule
