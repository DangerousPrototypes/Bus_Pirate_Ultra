
#include <stdint.h>
#include "buspirate.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "HWSPI.h"
#include "cdcacm.h"
#include "UI.h"
#include "FPGA.h"


static uint32_t cpol, cpha, br, dff, lsbfirst, csidle, od;


void HWSPI_start(void)
{
	//cdcprintf("set CS=%d", !csidle);

	HWSPI_setcs(0);

	modeConfig.wwr=0;
}

void HWSPI_startr(void)
{
	//cdcprintf("set CS=%d", !csidle);

	HWSPI_setcs(0);

	modeConfig.wwr=1;
}

void HWSPI_stop(void)
{
	//cdcprintf("set CS=%d", csidle);

	HWSPI_setcs(1);

	modeConfig.wwr=0;

}

void HWSPI_stopr(void)
{
	//cdcprintf("set CS=%d", csidle);

	HWSPI_setcs(1);

	modeConfig.wwr=0;
}

void HWSPI_send_test(uint32_t d){
    cdcprintf("SPI write: %04X\r\n",d);
}

uint32_t HWSPI_send(uint32_t d,uint32_t r,uint8_t b)
{
	uint16_t returnval, bpsm_command;
//FPGA_REG_07=0x0804;
	//TODO: lsb ??
	if((modeConfig.numbits>=1)&&(modeConfig.numbits<=8))
	{
	    bpsm_command=(0x0000|(((uint16_t)b<<8)|((uint16_t)d&0x00FF)));
        FPGA_REG_07=bpsm_command;//write b bits of d
        //cdcprintf("BPSM-pwrite: %04X\r\n",d);

	}
	else
	{
		cdcprintf("Only 1 to 8 bits are implemented, but N bits are possible! Abuse me!");
		modeConfig.error=1;
		returnval=0;
	}

	return (uint16_t) returnval;
}

uint32_t HWSPI_read(void)
{
	uint16_t returnval;
/*
	//TODO: check lsb??
	if((modeConfig.numbits==8)||(modeConfig.numbits==16))
	{
		if(modeConfig.numbits==8) spi_set_dff_8bit(BP_SPI);			// is there a less overhead way of doing this?
		if(modeConfig.numbits==16) spi_set_dff_16bit(BP_SPI);

		returnval = HWSPI_xfer(0xFF);					// is 0xFF ok?
//		returnval = spi_xfer(BP_SPI, 0xFF);					// is 0xFF ok?
	}
	else
	{
		cdcprintf("Only 8 or 16 bits are allowed, use SW3W instead");
		modeConfig.error=1;
		returnval=0;
	}
*/
	return (uint16_t) returnval;
}

void HWSPI_macro(uint32_t macro)
{
}

/****************************************************/

void HWSPI_start_post(void)
{
	//cdcprintf("set CS=%d", !csidle);

	HWSPI_setcs(0);

	modeConfig.wwr=0;
}

void HWSPI_startr_post(void)
{
	//cdcprintf("set CS=%d", !csidle);

	HWSPI_setcs(0);

	modeConfig.wwr=1;
}

void HWSPI_stop_post(void)
{
	//cdcprintf("set CS=%d", csidle);

	HWSPI_setcs(1);

	modeConfig.wwr=0;

}

void HWSPI_stopr_post(void)
{
	//cdcprintf("set CS=%d", csidle);

	HWSPI_setcs(1);

	modeConfig.wwr=0;
}

uint32_t HWSPI_send_post(uint32_t d,uint32_t r,uint8_t b)
{
	uint16_t returnval;

	returnval=FPGA_REG_07;

    if(returnval!=(0x0000|(((uint16_t)b<<8)|((uint16_t)d&0x00FF)))){
        cdcprintf("FPGA out of sync: %04X!=%04X\r\n", returnval, (0x0000|(((uint16_t)b<<8)|((uint16_t)d&0x00FF))));
        //todo: return and raise error flag
    }
    cdcprintf("TX: ");
    printnum((uint8_t)returnval);
    cdcprintf("RX: ");
	while(!gpio_get(BP_FPGA_FIFO_OUT_NEMPTY_PORT,BP_FPGA_FIFO_OUT_NEMPTY_PIN))//todo: THIS SHOULD NOT BLOCK! COULD GET STUCK!!
	    cdcprintf("Delay!");
	returnval=FPGA_REG_07;
    printnum((uint8_t)returnval); //TODO: use statemachine, can't depend on this word always being there!
    cdcprintf(" %04X (%02dbits)\r\n",returnval,b);
    return returnval;
}

uint32_t HWSPI_read_post(void)
{
	uint16_t returnval;
/*
	//TODO: check lsb??
	if((modeConfig.numbits==8)||(modeConfig.numbits==16))
	{
		if(modeConfig.numbits==8) spi_set_dff_8bit(BP_SPI);			// is there a less overhead way of doing this?
		if(modeConfig.numbits==16) spi_set_dff_16bit(BP_SPI);

		returnval = HWSPI_xfer(0xFF);					// is 0xFF ok?
//		returnval = spi_xfer(BP_SPI, 0xFF);					// is 0xFF ok?
	}
	else
	{
		cdcprintf("Only 8 or 16 bits are allowed, use SW3W instead");
		modeConfig.error=1;
		returnval=0;
	}
*/
	return (uint16_t) returnval;
}

void HWSPI_macro_post(uint32_t macro)
{
}

/********************************************************************/










void HWSPI_setup(void)
{
    //Needs more care! FPGA needs to be reconfigured after this (logic analyzer setup, etc)
    /*cdcprintf("Loading FPGA...");
    if(uploadfpga()){
        cdcprintf("done!\r\n");
    }else{
        cdcprintf("error :(\r\n");
    } //TODO: return error status and go back to HiZ????
*/
    //setup io
    //`define reg_bpio_oe wreg[6'h00][BP_PINS-1:0]
    //`define reg_bpio_od wreg[6'h00][BP_PINS-1+8:8]
    //`define reg_bpio_hl wreg[6'h01][BP_PINS-1:0]
    //`define reg_bpio_dir wreg[6'h01][BP_PINS-1+8:8]
    FPGA_REG_00=0x00FF; //normal output|output enable all pins
    FPGA_REG_01=0b0000110000000000; //direction output(only non-spi pins)|low(unimplemented)
}

void HWSPI_setup_exc(void)
{


}

void HWSPI_cleanup(void)
{

}

void HWSPI_pins(void)
{
	cdcprintf("CS\tMISO\tCLK\tMOSI");
}

void HWSPI_settings(void)
{
	cdcprintf("HWSPI (br cpol cpha cs)=(%d %d %d %d)", (br>>3), (cpol>>1)+1, cpha+1, csidle+1);
}

void HWSPI_printSPIflags(void)
{
	uint32_t temp;

	/*temp=SPI_SR(BP_SPI);

	if(temp&SPI_SR_BSY) cdcprintf(" BSY");
	if(temp&SPI_SR_OVR) cdcprintf(" OVR");
	if(temp&SPI_SR_MODF) cdcprintf(" MODF");
	if(temp&SPI_SR_CRCERR) cdcprintf(" CRCERR");
	if(temp&SPI_SR_UDR) cdcprintf(" USR");
	if(temp&SPI_SR_CHSIDE) cdcprintf(" CHSIDE");
//	if(temp&SPI_SR_TXE) cdcprintf(" TXE");
//	if(temp&SPI_SR_RXNE) cdcprintf(" RXNE");
*/

}

void HWSPI_help(void)
{
	cdcprintf("Peer to peer 3 or 4 wire full duplex protocol. Very\r\n");
	cdcprintf("high clockrates upto 20MHz are possible.\r\n");
	cdcprintf("\r\n");
	cdcprintf("More info: https://en.wikipedia.org/wiki/Serial_Peripheral_Interface_Bus\r\n");
	cdcprintf("\r\n");


	cdcprintf("BPCMD\t {,] |                 DATA (1..32bit)               | },]\r\n");
	cdcprintf("CMD\tSTART| D7  | D6  | D5  | D4  | D3  | D2  | D1  | D0  | STOP\r\n");

	if(cpha)
	{
		cdcprintf("MISO\t-----|{###}|{###}|{###}|{###}|{###}|{###}|{###}|{###}|------\r\n");
        cdcprintf("MOSI\t-----|{###}|{###}|{###}|{###}|{###}|{###}|{###}|{###}|------\r\n");
	}
	else
	{
		cdcprintf("MISO\t---{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}--|------\r\n");
		cdcprintf("MOSI\t---{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}--|------\r\n");
	}

	if(cpol>>1)
		cdcprintf("CLK     \"\"\"\"\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"\"\"\"\"\r\n");
	else
		cdcprintf("CLK\t_____|__\"\"_|__\"\"_|__\"\"_|__\"\"_|__\"\"_|__\"\"_|__\"\"_|__\"\"_|______\r\n");

	if(csidle)
		cdcprintf("CS\t\"\"___|_____|_____|_____|_____|_____|_____|_____|_____|___\"\"\"\r\n");
	else
		cdcprintf("CS\t__\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"___\r\n");

	cdcprintf("\r\nCurrent mode is CPHA=%d and CPOL=%d\r\n",cpha, cpol>>1);
	cdcprintf("\r\n");
	cdcprintf("Connection:\r\n");
	cdcprintf("\tMOSI \t------------------ MOSI\r\n");
	cdcprintf("\tMISO \t------------------ MISO\r\n");
	cdcprintf("{BP}\tCLK\t------------------ CLK\t{DUT}\r\n");
	cdcprintf("\tCS\t------------------ CS\r\n");
	cdcprintf("\tGND\t------------------ GND\r\n");
}


// helpers for binmode and other protocols
void HWSPI_setcpol(uint32_t val)
{
	cpol=val;
}

void HWSPI_setcpha(uint32_t val)
{
	cpha=val;
}

void HWSPI_setbr(uint32_t val)
{
	br=val;
}

void HWSPI_setdff(uint32_t val)
{
	dff=val;
}

void HWSPI_setlsbfirst(uint32_t val)
{
	lsbfirst=val;
}

void HWSPI_setcsidle(uint32_t val)
{
	csidle=val;
}

void HWSPI_setcs(uint8_t cs)
{
    uint16_t bpsm_command;

	if(cs==0)		// 'start'
	{
		if(csidle) bpsm_command=(0x8100);
			else bpsm_command=(0x81FF);
	}
	else			// 'stop'
	{
		if(csidle) bpsm_command=(0x81FF);
			else bpsm_command=(0x8100);
	}

	FPGA_REG_07=bpsm_command;
	cdcprintf("BPSM-io: %04X\r\n",bpsm_command);

}

uint16_t HWSPI_xfer(uint16_t d)
{
	FPGA_REG_07=(0x0800|(d&0x00FF));//write 8 bits of d
	cdcprintf("BPSM-pwrite: %04X\r\n",(0x0800|(d&0x00FF)));
	return 0x0000;
}







