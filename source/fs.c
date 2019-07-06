
#include <stdint.h>
#include "buspirate.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "cdcacm.h"
#include "fs.h" 

#define FLCMD_REMS	0x90		// Read Electronic Manufacturer ID & Device ID (REMS) 
#define FLCMD_READ	0x0B		// Read Data Bytes (READ) 


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
	spi_init_master(BP_FS_SPI, SPI_CR1_BAUDRATE_FPCLK_DIV_64, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	spi_set_full_duplex_mode(BP_FS_SPI);
	spi_enable(BP_FS_SPI);

}

void showID(void)
{
	uint16_t manid, devid;

	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);	// cs low
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_REMS);
	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);		// dummy
	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);		// dummy
	spi_xfer(BP_FS_SPI, (uint16_t) 0x00);		// 00= manid, devid 01= devid, manid
	manid=spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
	devid=spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

	cdcprintf("Flash manufacturer=%02X device=%02X\r\n", (uint8_t) manid, (uint8_t)devid);
	
}

void readflash (uint32_t addr, uint8_t *buff, uint16_t buffsize)
{
	int i;

	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);	// cs low
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_READ);
	spi_xfer(BP_FS_SPI, (uint16_t) ((addr>>16)&0x000000FF));	// address
	spi_xfer(BP_FS_SPI, (uint16_t) ((addr>>8)&0x000000FF));		// 
	spi_xfer(BP_FS_SPI, (uint16_t) (addr&0x000000FF));		// 

	for(i=0; i<buffsize; i++)
	{
		*(buff+i)=spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
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




