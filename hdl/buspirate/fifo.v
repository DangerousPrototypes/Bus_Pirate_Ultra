/*
 *  IcoProg -- Programmer and Debug Tool for the IcoBoard
 *
 *  Copyright (C) 2016  Clifford Wolf <clifford@clifford.at>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
 //https://github.com/cliffordwolf/icotools/blob/master/icosoc/common/icosoc_crossclkfifo.v
`ifndef __FIFO__
`define __FIFO__
module fifo #(
	parameter WIDTH = 8,
	parameter DEPTH = 16
) (
	input                  clock,
	input                  in_shift,
	input      [WIDTH-1:0] in_data,
	output reg             in_full,
	output reg             in_nempty,

	input                  out_pop,
	output     [WIDTH-1:0] out_data,
	output reg             out_nempty
);
	localparam integer ABITS = $clog2(DEPTH);

	initial begin
		in_full = 0;
		in_nempty = 0;
		out_nempty = 0;
	end

	reg [ABITS-1:0] in_ipos = 0, in_opos = 0;
	reg [ABITS-1:0] out_opos = 0, out_ipos = 0;

	reg [WIDTH-1:0] memory [0:DEPTH-1];

	wire [ABITS-1:0] next_ipos = in_ipos == DEPTH-1 ? 0 : in_ipos + 1;
	wire [ABITS-1:0] next_next_ipos = next_ipos == DEPTH-1 ? 0 : next_ipos + 1;

	wire [ABITS-1:0] next_opos = out_opos == DEPTH-1 ? 0 : out_opos + 1;
	reg [WIDTH-1:0] out_data_d;

	always @(posedge clock) begin

	out_ipos <=in_ipos;
	in_opos <= out_opos;

		if (in_shift && !in_full) begin
			memory[in_ipos] <= in_data;
			in_full <= next_next_ipos == in_opos;
			in_nempty <= 1;
			in_ipos <= next_ipos;
		end else begin
			in_full <= next_ipos == in_opos;
			in_nempty <= in_ipos != in_opos;
		end

		if (out_pop && out_nempty) begin
			out_data_d <= memory[next_opos];
			out_nempty <= next_opos != out_ipos;
			out_opos <= next_opos;
		end else begin
			out_data_d <= memory[out_opos];
			out_nempty <= out_opos != out_ipos;
		end

	end

	assign out_data = out_nempty ? out_data_d : 0;



endmodule
`endif
