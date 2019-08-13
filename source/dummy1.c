

#include <stdint.h>
#include "dummy1.h"
#include "cdcacm.h"
#include "fpga.h"
#include "fs.h"
#include <libopencm3/stm32/exti.h>


static uint32_t returnval, pending, irq0, irq1;

void dummy1_start(void)
{
	cdcprintf("-DUMMY1- start()");
}
void dummy1_startr(void)
{
	cdcprintf("-DUMMY1- startr()");
}
void dummy1_stop(void)
{
	cdcprintf("-DUMMY1- stop()");
}
void dummy1_stopr(void)
{
	cdcprintf("-DUMMY1- stopr()");
}
uint32_t dummy1_send(uint32_t d)
{
	cdcprintf("--DUMMY1- send(%08X)=%08X", d, returnval);

	returnval=d;

	return d;
}
uint32_t dummy1_read(void)
{
	cdcprintf("-DUMMY1- read()=%08X", returnval);
	return returnval;
}
void dummy1_clkh(void)
{
	cdcprintf("-DUMMY1- clkh()");
}
void dummy1_clkl(void)
{
	cdcprintf("-DUMMY1- clkl()");
}
void dummy1_dath(void)
{
	cdcprintf("-DUMMY1- dath()");
}
void dummy1_datl(void)
{
	cdcprintf("-DUMMY1- datl()");
}
uint32_t dummy1_dats(void)
{
	cdcprintf("-DUMMY1- dats()=%08X", returnval);
	return returnval;
}
void dummy1_clk(void)
{
	cdcprintf("-DUMMY1- clk()");
}
uint32_t dummy1_bitr(void)
{
	cdcprintf("-DUMMY1- bitr()=%08X", returnval);
	return returnval;
}
uint32_t dummy1_period(void)
{
	if(pending)
	{
		cdcprintf("\r\n\x07");
		cdcprintf("IRQ0=%d, IRQ1=%d", irq0, irq1);
		pending=0;
		irq0=0;
		irq1=0;
		return 1;
	}
	return 0;
}
void dummy1_macro(uint32_t macro)
{
	cdcprintf("-DUMMY1- macro(%08X)", macro);
}
void dummy1_setup(void)
{
	file_struct *file;

	cdcprintf("-DUMMY1- setup()");
	pending=0;

	// upload bitstream
	file=findfileindex(3);
	uploadfpga(file->addr, file->size);

	// enable irq
	exti_enable_request(EXTI2);
	exti_enable_request(EXTI3);

}
void dummy1_setup_exc(void)
{
	cdcprintf("-DUMMY1- setup_exc()");

	FPGA_REG(0x3f)=0xFFFF; // triggers the interrupt

}
void dummy1_cleanup(void)
{
	cdcprintf("-DUMMY1- cleanup()");
}
void dummy1_pins(void)
{
	cdcprintf("pin1\tpin2\tpin3\tpin4");
}
void dummy1_settings(void)
{
	cdcprintf("DUMMY (arg1 arg2)=(%d, %d)", 1, 2);
}


void dummy1_isr(uint8_t num)
{
	pending=1;

	if (num==0) irq0=1;
	if (num==1) irq1=1;
}


