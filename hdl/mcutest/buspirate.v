//------------------------------------------------------------------
//-- Bus Pirate peripheral tests
//--
//-- sram interface test
//------------------------------------------------------------------



module top #(
  parameter MC_DATA_WIDTH = 16,
  parameter MC_ADD_WIDTH = 6
) (
  input wire clock,
  input wire reset,
  input wire mc_oe, mc_ce, mc_we,
  input wire [MC_ADD_WIDTH-1:0] mc_add,
  inout wire [MC_DATA_WIDTH-1:0] mc_data,
  output reg irq0, irq1
);

	// Tristate pin handling
	// Memory controller interface
	wire [MC_DATA_WIDTH-1:0] mc_din;
	reg [MC_DATA_WIDTH-1:0] mc_dout;
	reg mc_data_oe;

	reg go;
	reg [31:0] count;

	// memory regs
	reg [MC_DATA_WIDTH-1:0] SRAM [32:0];
	initial begin
		mc_data_oe <= 1'b0;						// start in HiZ
		SRAM[5'b00000] <= 16'b0101101001011010;				// test values
		SRAM[5'b00001] <= 16'b1010010110100101;		
		SRAM[5'b00010] <= 16'b0101101001011010;
		SRAM[5'b00011] <= 16'b1010010110100101;		
		SRAM[5'b00100] <= 16'b0101101001011010;
		SRAM[5'b00101] <= 16'b1010010110100101;		
		SRAM[5'b00110] <= 16'b0101101001011010;
		SRAM[5'b00111] <= 16'b1010010110100101;
		SRAM[5'b01000] <= 16'b0101101001011010;
		SRAM[5'b01001] <= 16'b1010010110100101;		
		SRAM[5'b01010] <= 16'b0101101001011010;
		SRAM[5'b01011] <= 16'b1010010110100101;		
		SRAM[5'b01100] <= 16'b0101101001011010;
		SRAM[5'b01101] <= 16'b1010010110100101;		
		SRAM[5'b01110] <= 16'b0101101001011010;
		SRAM[5'b01111] <= 16'b1010010110100101;	
		SRAM[5'b10000] <= 16'b0101101001011010;
		SRAM[5'b10001] <= 16'b1010010110100101;		
		SRAM[5'b10010] <= 16'b0101101001011010;
		SRAM[5'b10011] <= 16'b1010010110100101;		
		SRAM[5'b10100] <= 16'b0101101001011010;
		SRAM[5'b10101] <= 16'b1010010110100101;		
		SRAM[5'b10110] <= 16'b0101101001011010;
		SRAM[5'b10111] <= 16'b1010010110100101;	
		SRAM[5'b11000] <= 16'b0101101001011010;
		SRAM[5'b11001] <= 16'b1010010110100101;		
		SRAM[5'b11010] <= 16'b0101101001011010;
		SRAM[5'b11011] <= 16'b1010010110100101;		
		SRAM[5'b11100] <= 16'b0101101001011010;	
		SRAM[5'b11101] <= 16'b1010010110100101;		
		SRAM[5'b11110] <= 16'b0101101001011010;
		SRAM[5'b11111] <= 16'b1010010110100101;	

		go <= 1'b0;
		count <= 32'b00000000000000000000000000000000;
		irq0 <= 1'b1;
		irq1 <= 1'b1;
	end


	always @(negedge clock)
	begin
		if(mc_ce == 1'b0)						// cs = low
		begin
			if ((mc_we == 1'b0)&&(mc_oe == 1'b1))			// write
			begin
				mc_data_oe <= 0;
				SRAM [mc_add] <= mc_din;
				if(mc_add==6'b111111) go=1;
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
			
		if(go == 1'b1)
		begin
			count<=count+1;
			if(count==32'h40000000) irq0 <= 1'b0;
				else irq0 <= 1'b1;
			if(count==32'h80000000) irq1 <= 1'b0;
				else irq1 <= 1'b1;
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
