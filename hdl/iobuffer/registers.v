`ifndef __REGISTERS__
`define __REGISTERS__

//write registers
`define reg_bpio_oe wreg[6'h00][BP_PINS-1:0]
`define reg_bpio_od wreg[6'h00][BP_PINS-1+8:8]
`define reg_bpio_hl wreg[6'h01][BP_PINS-1:0]
`define reg_bpio_dir wreg[6'h01][BP_PINS-1+8:8]

`define reg_la_write wreg[6'h02]
`define reg_la_config wreg[6'h03]
`define reg_la_io_quad wreg[6'h03][0]
`define reg_la_io_quad_direction wreg[6'h03][1]
`define reg_la_io_spi wreg[6'h03][2]
`define reg_la_start wreg[6'h03][3]
`define reg_la_io_cs0 wreg[6'h03][8] //reserve upper bits for more SRAMs
`define reg_la_io_cs1 wreg[6'h03][9]
`define reg_la_samples_post wreg[6'h04]

`define reg_pwm_on wreg[6'h05]
`define reg_pwm_off wreg[6'h06]

//read registers
`define reg_la_read rreg[6'h02]
//`define reg_la_config wreg[6'h03]
`define reg_la_sample_count rreg[6'h04]


`endif
