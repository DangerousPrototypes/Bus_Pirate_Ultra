
#include <stdint.h>
#include "buspirate.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "cdcacm.h"
#include "fs.h" 
#include "delay.h"

#define FLCMD_REMS	0x90		// Read Electronic Manufacturer ID & Device ID (REMS) 
#define FLCMD_RDID	0x9F		// Read Identification (RDID)  
#define FLCMD_READ	0x03		// Read Data Bytes (READ) 
#define FLCMD_FREAD	0x0B		// Fast Read Data Bytes (FREAD) 
#define FLCMD_RUID	0x4B		// Read Unique ID (RUID)
#define FLCMD_PE	0x81		// Page Erase (PE) 
#define FLCMD_WREN	0x06		// Write Enable (WREN) 
#define FLCMD_RDSR	0x05		// Read Status Register (RDSR) 
#define FLCMD_PP	0x02		// Page Program (PP) 

void eraseSector(uint32_t addr)
{
	uint8_t busy, status;

	busy=1;
	addr&=0xFFFFFF00;		// page align

	// write enable
	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_WREN);
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

	// erase page
	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_PE);
	spi_xfer(BP_FS_SPI, (uint16_t) ((addr>>16)&0x000000FF));	// address
	spi_xfer(BP_FS_SPI, (uint16_t) ((addr>>8)&0x000000FF));		// 
	spi_xfer(BP_FS_SPI, (uint16_t) (addr&0x000000FF));		// 
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

	while(busy)
	{
		delayus(10);

		// check WIP bit 
		gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);
		spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_RDSR);
		status=(uint8_t) spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
		gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

		busy=(status&0x01);
	}
}


void writeFlash(uint32_t addr, uint8_t *buff, uint8_t size) 
{
	uint8_t busy, status;
	int i;

	// write enable
	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_WREN);
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

	// page write
	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_PP);
	spi_xfer(BP_FS_SPI, (uint16_t) ((addr>>16)&0x000000FF));	// address
	spi_xfer(BP_FS_SPI, (uint16_t) ((addr>>8)&0x000000FF));		// 
	spi_xfer(BP_FS_SPI, (uint16_t) (addr&0x000000FF));		// 

	for(i=0; i<(size); i++)
	{
		spi_xfer(BP_FS_SPI, (uint16_t) buff[i]);
	}

	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

	busy=1;

	while(busy)
	{
		delayus(10);

		// check WIP bit 
		gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);
		spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_RDSR);
		status=(uint8_t) spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
		gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

		busy=(status&0x01);
	}
}

void flashinit(void)
{
	// enable peripheral
	rcc_periph_clock_enable(BP_FS_SPI_CLK);

	// SPI pins of FPGA
	gpio_set_mode(BP_FS_MOSI_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FS_MOSI_PIN);
	gpio_set_mode(BP_FS_CS_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_FS_CS_PIN);
	gpio_set_mode(BP_FS_CLK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FS_CLK_PIN);
	gpio_set_mode(BP_FS_MISO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_FS_MISO_PIN);

	// setup SPI (cpol=1, cpha=1) 4Mhz
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);
	spi_reset(BP_FS_SPI);
	spi_init_master(BP_FS_SPI, SPI_CR1_BAUDRATE_FPCLK_DIV_128, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	spi_set_full_duplex_mode(BP_FS_SPI);
	spi_enable(BP_FS_SPI);

}

void showFlashID(void)
{
	uint16_t manid, devid1, devid2;

	cdcprintf("Flash info:\r\n");

	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);	// cs low
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_REMS);
	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);		// dummy
	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);		// dummy
	spi_xfer(BP_FS_SPI, (uint16_t) 0x00);		// 00= manid, devid 01= devid, manid
	manid=spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
	devid1=spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

	cdcprintf("REMS=%02X %02X\r\n", (uint8_t) manid, (uint8_t)devid1);

	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);	// cs low
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_RDID);
	manid=spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
	devid1=spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
	devid2=spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

	cdcprintf("RDID=%02X %02X %02X\r\n", (uint8_t) manid, (uint8_t)devid1, (uint8_t)devid2);

	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);	// cs low
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_RUID);
	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);		// dummy
	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);		// dummy
	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);		// dummy
	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);		// dummy
	manid=spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
	devid1=spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

	
}

void readFlash (uint32_t addr, uint8_t *buff, uint16_t buffsize)
{
	int i;

	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);	// cs low
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_READ);
	spi_xfer(BP_FS_SPI, (uint16_t) ((addr>>16)&0x000000FF));	// address
	spi_xfer(BP_FS_SPI, (uint16_t) ((addr>>8)&0x000000FF));		// 
	spi_xfer(BP_FS_SPI, (uint16_t) (addr&0x000000FF));		// 
//	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);		// dummy

	for(i=0; i<buffsize; i++)
	{
		*(buff+i)=(uint8_t)spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
	}

	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);
}

void printbuff(uint8_t *buff, uint16_t buffsize)
{
	uint32_t i, j;
	
	for(i=0; i<buffsize; i+=8)
	{
		cdcprintf("%08X: ", i);
		for(j=0; j<8; j++)
		{
			cdcprintf("%02X ", buff[i+j]);
		}
		for(j=0; j<8; j++)
		{
			cdcprintf("%c", buff[i+j]);
		}
		cdcprintf("\r\n");
	}
}


