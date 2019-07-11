//------------------------------------------------------------------
//-- Bus Pirate peripheral tests
//--
//-- sram interface test
//------------------------------------------------------------------



module top #(
  parameter MC_DATA_WIDTH = 16,
  parameter MC_ADD_WIDTH = 6,
)(
  input wire clock,
  input wire reset,
  input wire mc_oe, mc_ce, mc_we,
  input wire [MC_ADD_WIDTH-1:0] mc_add,
  inout wire [MC_DATA_WIDTH-1:0] mc_data,
);

	// Tristate pin handling
	// Memory controller interface
	wire [MC_DATA_WIDTH-1:0] mc_din;
	reg [MC_DATA_WIDTH-1:0] mc_dout;
	reg mc_data_oe;

	// memory regs
	reg [MC_DATA_WIDTH-1:0] SRAM [MC_ADD_WIDTH-1:0];

	initial begin
		mc_data_oe <= 1'b0;									// HiZ
	end

	always @(posedge reset)
	begin
		SRAM[5'b00000] <= 16'b0101101001011010;
		SRAM[5'b00001] <= 16'b1010010110100101;		
	end


	always @(posedge clock)
	begin
		if(mc_ce == 1'b0)									// cs = low
		begin
			if ((mc_we == 1'b0)&&(mc_oe == 1'b1))			// write
			begin
				mc_data_oe <= 0;
				SRAM [mc_add] <= mc_din;
			end
			else if ((mc_we == 1'b1)&&(mc_oe == 1'b0))		// read
			begin
				mc_data_oe <= 1;
				mc_dout <= SRAM [mc_add];
			end
			else
			begin
				mc_data_oe <= 0;
			end
		end		
		else
		begin
			mc_data_oe <= 0;
		end
			
	end



    //define the tristate data pin explicitly in the top module

    // Memory controller data pins
    SB_IO #(
      .PIN_TYPE(6'b1010_01),
      .PULLUP(1'b0)
    ) mc_io [MC_DATA_WIDTH-1:0] (
      .PACKAGE_PIN(mc_data),
      .OUTPUT_ENABLE(mc_data_oe),
      .D_OUT_0(mc_dout),
      .D_IN_0(mc_din)
    );
    
endmodule
