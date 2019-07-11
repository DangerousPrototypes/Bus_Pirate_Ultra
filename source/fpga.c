
#include <stdint.h>
#include "cdcacm.h"
#include "delay.h"
#include "fpga.h"
#include "buspirate.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/fsmc.h>

void progressbar(uint32_t count, uint32_t maxcount)
{
	uint32_t i;
	char bar[21];

	for(i=0; i<=((count*20)/maxcount); i++) bar[i]='#';
	for(i=((count*20)/maxcount)+1; i<21; i++) bar[i]='-';

	cdcprintf("[%s] %d/%d\r", bar, count, maxcount);


}

void upload(void)
{
	int i;

	cdcprintf("\r\nuploading\r\n");


	for(i=0; i<=1000; i++)
	{
		progressbar(i, 1000);
		delayms(10);
	}
}

#include "sram.h"

int uploadfpga(void)
{
	uint32_t i, returnval;

	// reset FPGA+program FPGA
	cdcprintf("Resetting FPGA..\r\n");
	gpio_clear(BP_FPGA_CRESET_PORT, BP_FPGA_CRESET_PIN);		// go into reset
	gpio_clear(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);			// if CS is low during reset, fpga becomes spi slave
	delayms(10);							// wait at least 200ns 
	gpio_set(BP_FPGA_CRESET_PORT, BP_FPGA_CRESET_PIN);		// out of reset
	delayms(100);							// wait at lease 1200us or wait for miso is 1
	gpio_set(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);			// release cs

	cdcprintf("CDONE=%02X\r\n", gpio_get(BP_FPGA_CDONE_PORT, BP_FPGA_CDONE_PIN));

	// start transfer
	spi_xfer(BP_FPGA_SPI, (uint16_t)0x0000);			// 8 dummy clock cycles
	gpio_clear(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);			// assert cs
	
	// send image (msb first)
	cdcprintf("Uploading FPGA bitstream..\r\n");
//	for(i=0; i<=135183; i++)
	for(i=0; i<135100; i++)
	{
		if((i&0x3FF)==0) progressbar(i, 135100);
		spi_xfer(BP_FPGA_SPI, (uint16_t)bitstream[i]);	
	}
	progressbar(i, 135100);

	gpio_set(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);			// release cs
	for(i=0; i<12; i++) spi_xfer(BP_FPGA_SPI, (uint16_t)0x0000);	// wait 100 clockcycles to cdone=1 (104 cycles)

	cdcprintf("\r\n");

	if(gpio_get(BP_FPGA_CDONE_PORT, BP_FPGA_CDONE_PIN))
	{
		cdcprintf("CDONE=1 SUCCESS!!\r\n");
		returnval=1;
		rcc_set_mco(RCC_CFGR_MCO_PLL_DIV2);		// PLL/2
//		rcc_set_mco(RCC_CFGR_MCO_HSI);			// internal oscillator
//		rcc_set_mco(RCC_CFGR_MCO_HSE);			// external oscillator
//		rcc_set_mco(RCC_CFGR_MCO_SYSCLK);		// sysclk

	}
	else
	{
		cdcprintf("CDONE=0 ERROR uploading!!\r\n");
		returnval=0;
	}

	for(i=0; i<7; i++) spi_xfer(BP_FPGA_SPI, (uint16_t)0x0000);	// wait 49 clockcycles to enable pinio's (56 cycles)

	return returnval;
}

void fpgainit(void)
{
	// enable peripheral
	rcc_periph_clock_enable(BP_FPGA_SPI_CLK);

	// SPI pins of FPGA
	gpio_set_mode(BP_FPGA_MOSI_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FPGA_MOSI_PIN);
	gpio_set_mode(BP_FPGA_CS_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_FPGA_CS_PIN);
	gpio_set_mode(BP_FPGA_CLK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FPGA_CLK_PIN);
	gpio_set_mode(BP_FPGA_MISO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_FPGA_MISO_PIN);

	// control pins
	gpio_set_mode(BP_FPGA_CDONE_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_FPGA_CDONE_PIN);
	gpio_set_mode(BP_FPGA_CRESET_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BP_FPGA_CRESET_PIN);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO8);

	// setup SPI (cpol=1, cpha=1) +- 1MHz
	spi_reset(BP_FPGA_SPI);
	spi_init_master(BP_FPGA_SPI, SPI_CR1_BAUDRATE_FPCLK_DIV_32, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	spi_set_full_duplex_mode(BP_FPGA_SPI);
	spi_enable(BP_FPGA_SPI);

	// put FPGA into reset and slavemode
	gpio_clear(BP_FPGA_CRESET_PORT, BP_FPGA_CRESET_PIN);
	gpio_clear(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);

	// disable clock
	rcc_set_mco(RCC_CFGR_MCO_NOCLK);		// disable clock

	// memory interface
	gpio_set_mode(BP_FSMC_A0_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_A0_PIN);
	gpio_set_mode(BP_FSMC_A1_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_A1_PIN);
	gpio_set_mode(BP_FSMC_A2_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_A2_PIN);
	gpio_set_mode(BP_FSMC_A3_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_A3_PIN);
	gpio_set_mode(BP_FSMC_A4_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_A4_PIN);
	gpio_set_mode(BP_FSMC_A5_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_A5_PIN);
	
	gpio_set_mode(BP_FSMC_D0_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_D0_PIN);
	gpio_set_mode(BP_FSMC_D1_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_D1_PIN);
	gpio_set_mode(BP_FSMC_D2_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_D2_PIN);
	gpio_set_mode(BP_FSMC_D3_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_D3_PIN);
	gpio_set_mode(BP_FSMC_D4_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_D4_PIN);
	gpio_set_mode(BP_FSMC_D5_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_D5_PIN);
	gpio_set_mode(BP_FSMC_D6_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_D6_PIN);
	gpio_set_mode(BP_FSMC_D7_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_D7_PIN);
	gpio_set_mode(BP_FSMC_D8_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_D8_PIN);
	gpio_set_mode(BP_FSMC_D9_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_D9_PIN);
	gpio_set_mode(BP_FSMC_D10_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_D10_PIN);
	gpio_set_mode(BP_FSMC_D11_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_D11_PIN);
	gpio_set_mode(BP_FSMC_D12_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_D12_PIN);
	gpio_set_mode(BP_FSMC_D13_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_D13_PIN);
	gpio_set_mode(BP_FSMC_D14_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_D14_PIN);
	gpio_set_mode(BP_FSMC_D15_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_D15_PIN);

	gpio_set_mode(BP_FSMC_NOE_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_NOE_PIN);
	gpio_set_mode(BP_FSMC_NWE_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_NWE_PIN);	// bank 3
	gpio_set_mode(BP_FSMC_NCE_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FSMC_NCE_PIN);


	// enable fsmc
	rcc_periph_clock_enable(RCC_FSMC);

/*
from https://www.stm32duino.com/viewtopic.php?t=1637

timing for 55ns sram:

#define DATAST   0x4
#define ADDSET   0x1


fsmc registers set to:

        *bcrs[i] = (FSMC_BCR_WREN |
                    FSMC_BCR_MTYP_SRAM |
                    FSMC_BCR_MWID_16BITS |
                    FSMC_BCR_MBKEN);
        *btrs[i] = (DATAST << 8) | ADDSET;

https://github.com/leaflabs/libmaple/blob/master/libmaple/include/libmaple/fsmc.h defines:

#define FSMC_BCR_MWID_8BITS             (0x0 << 4)
#define FSMC_BCR_MWID_16BITS            (0x1 << 4)
#define FSMC_BCR_WREN_BIT               12
#define FSMC_BCR_WREN                   (1U << FSMC_BCR_WREN_BIT)
#define FSMC_BCR_MTYP_SRAM              (0x0 << 2)
#define FSMC_BCR_MBKEN_BIT              0
#define FSMC_BCR_MBKEN                  (1U << FSMC_BCR_MBKEN_BIT)

#define FSMC_BTR_DATAST                 (0xFF << 8)
#define FSMC_BTR_ADDSET                 0xF
*/

// 55ns SRAM timings?
#define DATAST   0x4
#define ADDSET   0x1

	// fsmc setup (bank3 is used) 0x6c000000
	//           WREN     SRAM     16b     MBKEN   EXTMOD
	FSMC_BCR3=((1<<12) | (0<<2) | (1<<4) | (1<<0) | (1<<14));
	FSMC_BTR3=(DATAST << 8) | ADDSET;
	
}



