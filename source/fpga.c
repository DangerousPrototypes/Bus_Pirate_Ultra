
#include <stdint.h>
#include "cdcacm.h"
#include "delay.h"
#include "fpga.h"
#include "buspirate.h"
#include "fs.h"
#include "UI.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/fsmc.h>
#include <libopencm3/stm32/dma.h>

int bitstreaminfo(uint32_t addr, uint32_t size)
{
	uint32_t i, offset;
	char header[532];
	char description[256];
	char capabilities[256];
	char date[10], version[10];

	if(size < 8192)		// TODO: find the bitstream size for 384 LUT devices
	{
		cdcprintf("bitstream size is too less\r\n");
		return 0;
	}

	// retrieve info from bitstream 
	// info is embedded between 0xff, 0x00 <description>, 0x00, <capabilities>, 0x00,<datecodes>, 0x00,<version>, 0x00, 0xFF <bitstream>
	readflash(addr, (uint8_t *)header, 532);

	i=0;
	offset=2;
	while((header[offset+i]!=0x00)&&(i<255))			// name/description
	{
		description[i]=header[offset+i];
		i++;
	}
	description[i]=0x00;						// null terminate string
	offset+=i+1;
	i=0;
	while((header[offset+i]!=0x00)&&(i<255))			// capabilities
	{
		capabilities[i]=header[offset+i];
		i++;
	}
	capabilities[i]=0x00;
	offset+=i+1;
	i=0;
	while((header[offset+i]!=0x00)&&(i<9))				// datecode
	{
		date[i]=header[offset+i];
		i++;
	}
	date[i]=0x00;
	offset+=i+1;
	i=0;
	while((header[offset+i]!=0x00)&&(i<9))				// version
	{
		version[i]=header[offset+i];
		i++;
	}
	version[i]=0x00;
	
	cdcprintf("FPGA bitstream info:\r\n");
	cdcprintf(" name: %s\r\n", description);
	cdcprintf(" capabilities: %s\r\n", capabilities);
	cdcprintf(" date: %s\r\n", date);
	cdcprintf(" version: %s\r\n", version);

	return 1;	// seems valid
}


int uploadfpga(uint32_t addr, uint32_t size)
{
	uint32_t i;
	int returnval;

	// reset FPGA+program FPGA
	cdcprintf("Resetting FPGA..\r\n");
	gpio_clear(BP_FPGA_CRESET_PORT, BP_FPGA_CRESET_PIN);		// go into reset
	gpio_clear(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);			// if CS is low during reset, fpga becomes spi slave
	delayms(10);							// wait at least 200ns 
	gpio_set(BP_FPGA_CRESET_PORT, BP_FPGA_CRESET_PIN);		// out of reset
	delayms(100);							// wait at lease 1200us or wait for miso is 1
	gpio_set(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);			// release cs


	// start transfer
	spi_xfer(BP_FPGA_SPI, (uint16_t)0x0000);			// 8 dummy clock cycles
	gpio_clear(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);			// assert cs

	// setup flash for retrieving bitstream
	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);			// cs flash low
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_READ);
	spi_xfer(BP_FS_SPI, (uint16_t) ((addr>>16)&0x000000FF));	// address
	spi_xfer(BP_FS_SPI, (uint16_t) ((addr>>8)&0x000000FF));		// 
	spi_xfer(BP_FS_SPI, (uint16_t) (addr&0x000000FF));		// 

	// send image (msb first)
	cdcprintf("Uploading FPGA bitstream..\r\n");
	for(i=0; i<size; i++)
	{
		if((i&0x3FF)==0) progressbar(i, size);
		spi_xfer(BP_FPGA_SPI, (uint16_t)spi_xfer(BP_FS_SPI, (uint16_t)0x00FF));	
	}
	progressbar(i, size);

	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);				// cs flash high

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

	// reset fpga  bitstream
	// TODO: fpga needs a reset!


	return returnval;
}

void fpgainit(void)
{
	// enable peripherals needed 
	rcc_periph_clock_enable(BP_FPGA_SPI_CLK);
	rcc_periph_clock_enable(BP_FPGA_DMACTRL_CLK);

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
*/


	// fsmc setup (bank3 is used) 0x6c000000
	//           WREN     SRAM     16b     MBKEN   EXTMOD
	FSMC_BCR3=((1<<12) | (0<<2) | (1<<4) | (1<<0) | (1<<14));
	FSMC_BTR3=(FSMC_DATAST << 8) | FSMC_ADDSET;
	FSMC_BWTR3=(FSMC_DATAST << 8) | FSMC_ADDSET;
	
}

static int received=0;
static int transfered=0;

void writefpga(uint16_t *data, int size)
{
	uint8_t timeout=DMA_TIMEOUT;
	transfered=0;

	dma_channel_reset(BP_FPGA_DMACTRL, BP_MEM2FPGA_DMACHAN);				// clear DMA registers

	dma_set_peripheral_address(BP_FPGA_DMACTRL, BP_MEM2FPGA_DMACHAN, FPGA_BASE);		// assume fpga-base/reg0 as data register
	dma_set_memory_address(BP_FPGA_DMACTRL, BP_MEM2FPGA_DMACHAN, (uint32_t)data);
	dma_set_number_of_data(BP_FPGA_DMACTRL, BP_MEM2FPGA_DMACHAN, size);			// num of bytes
	dma_set_read_from_memory(BP_FPGA_DMACTRL, BP_MEM2FPGA_DMACHAN);				// memory --> fpga
	dma_enable_memory_increment_mode(BP_FPGA_DMACTRL, BP_MEM2FPGA_DMACHAN);			// increment memory
	dma_disable_peripheral_increment_mode(BP_FPGA_DMACTRL, BP_MEM2FPGA_DMACHAN);		// don't increment peripheral
	dma_set_peripheral_size(BP_FPGA_DMACTRL, BP_MEM2FPGA_DMACHAN, DMA_CCR_PSIZE_16BIT);	// 16 bits peripheral access
	dma_set_memory_size(BP_FPGA_DMACTRL, BP_MEM2FPGA_DMACHAN, DMA_CCR_MSIZE_16BIT);		// 16 bits memory access
	dma_set_priority(BP_FPGA_DMACTRL, BP_MEM2FPGA_DMACHAN, DMA_CCR_PL_VERY_HIGH);		// move as fast as possible

	dma_enable_transfer_complete_interrupt(BP_FPGA_DMACTRL, BP_MEM2FPGA_DMACHAN);

	dma_enable_mem2mem_mode(BP_FPGA_DMACTRL, BP_MEM2FPGA_DMACHAN);				// do we need the enable too?
	dma_enable_channel(BP_FPGA_DMACTRL, BP_MEM2FPGA_DMACHAN);				// go!

	while((transfered==0)&&(timeout--)) delayus(1);

}


void readfpga(uint16_t *data, int size)
{
	uint8_t timeout=DMA_TIMEOUT;
	received=0;

	dma_channel_reset(BP_FPGA_DMACTRL, BP_FPGA2MEM_DMACHAN);				// clear DMA registers

	dma_set_peripheral_address(BP_FPGA_DMACTRL, BP_FPGA2MEM_DMACHAN, FPGA_BASE);		// assume fpga-base/reg0 as data register
	dma_set_memory_address(BP_FPGA_DMACTRL, BP_FPGA2MEM_DMACHAN, (uint32_t)data);
	dma_set_number_of_data(BP_FPGA_DMACTRL, BP_FPGA2MEM_DMACHAN, size);			// num of bytes
	dma_set_read_from_peripheral(BP_FPGA_DMACTRL, BP_FPGA2MEM_DMACHAN);			// fpga --> memory
	dma_enable_memory_increment_mode(BP_FPGA_DMACTRL, BP_FPGA2MEM_DMACHAN);			// increment memory
	dma_disable_peripheral_increment_mode(BP_FPGA_DMACTRL, BP_FPGA2MEM_DMACHAN);		// don't increment peripheral
	dma_set_peripheral_size(BP_FPGA_DMACTRL, BP_FPGA2MEM_DMACHAN, DMA_CCR_PSIZE_16BIT);	// 16 bits peripheral access
	dma_set_memory_size(BP_FPGA_DMACTRL, BP_FPGA2MEM_DMACHAN, DMA_CCR_MSIZE_16BIT);		// 16 bits memory access
	dma_set_priority(BP_FPGA_DMACTRL, BP_FPGA2MEM_DMACHAN, DMA_CCR_PL_VERY_HIGH);		// move as fast as possible

	dma_enable_transfer_complete_interrupt(BP_FPGA_DMACTRL, BP_FPGA2MEM_DMACHAN);

	dma_enable_mem2mem_mode(BP_FPGA_DMACTRL, BP_FPGA2MEM_DMACHAN);				// do we need the enable too?
	dma_enable_channel(BP_FPGA_DMACTRL, BP_FPGA2MEM_DMACHAN);				// go!

	while((received==0)&&(timeout--)) delayus(1);
}

void dma1_channel6_isr(void)				// receive isr
{
	if ((DMA1_ISR &DMA_ISR_TCIF6) != 0) {
		DMA1_IFCR |= DMA_IFCR_CTCIF6;

		received = 1;
	}

	dma_disable_transfer_complete_interrupt(BP_FPGA_DMACTRL, BP_FPGA2MEM_DMACHAN);

	dma_disable_channel(BP_FPGA_DMACTRL, BP_FPGA2MEM_DMACHAN);
}

void dma1_channel7_isr(void)				// send isr
{
	if ((DMA1_ISR &DMA_ISR_TCIF7) != 0) {
		DMA1_IFCR |= DMA_IFCR_CTCIF7;

		transfered = 1;
	}

	dma_disable_transfer_complete_interrupt(BP_FPGA_DMACTRL, BP_MEM2FPGA_DMACHAN);

	dma_disable_channel(BP_FPGA_DMACTRL, BP_MEM2FPGA_DMACHAN);
}


