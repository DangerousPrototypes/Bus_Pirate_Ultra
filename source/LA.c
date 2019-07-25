

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/f1/bkp.h>
#include <libopencm3/cm3/scb.h>
#include "buspirate.h"
#include "LA.h"
#include "cdcacm.h"
#include "delay.h"
#include "fpga.h"

//static void setup_spix1rw(void);

static void setup_spix4w(void);
static void spiWx4(uint8_t d);

static void setup_spix4r(void);
static uint8_t spiRx4(void);

/*static enum _stop
        {
         NORMAL=0,
         OVERFLOW
        } stop=NORMAL;*/


static volatile uint32_t counts;

void logicAnalyzerSetup(void)
{
    uint8_t temp;
    uint16_t i;
    char buff[256];

    //la_sram_mode_setup();
    //la_sram_quad_setup();

    sram_deselect();
    //delayus(100);
	//send mode reset command just in case

	setup_spix4w(); //write
	sram_select();
	spiWx4(CMDRESETSPI);
	sram_deselect();

    //read from SRAM test
    setup_spix1rw();
    sram_select();	// cs low
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0x9F);
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);//24 dummy address bits...
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);
    for(i=0;i<8;i++){
        temp=buff[i]=spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF); //0d=MFID, 5d=KGD, + 6 bytes of density info
    }
    //release FPGA into program mode
    sram_deselect();			// release cs


    FPGA_REG_02|=1<<2;
    sram_select();	// cs low
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0x9F);
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);//24 dummy address bits...
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);
    for(i=0;i<8;i++){
        temp=buff[i]=spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF); //0d=MFID, 5d=KGD, + 6 bytes of density info
    }
    //release FPGA into program mode
    sram_deselect();			// release cs






	//quad mode
	sram_select();
	//delayms(1);
	spi_xfer(BP_FPGA_SPI, (uint16_t)CMDQUADMODE);
	sram_deselect();

	//clear the sram for testing purposes
	setup_spix4w(); //write
	sram_select();
	//delayms(1);
	spiWx4(CMDWRITE); //write command
	spiWx4(0);
	spiWx4(0);
	spiWx4(0); //3 byte address
	for(i=0;i<4;i++)
		spiWx4(i);
   /* la_sram_arm_start(); //attach SRAM to LATCH
    for(i=0;i<500;i++){ //clock in some data to sram
        delayms(4);
        gpio_set(GPIOB,GPIO13); //BP_LA_SRAM_CLOCK_HIGH(); //CLOCK HIGH
		delayms(4);
		gpio_clear(GPIOB,GPIO13); //BP_LA_SRAM_CLOCK_LOW(); //CLOCK LOW
        delayms(4);
        gpio_set(GPIOB,GPIO13); //BP_LA_SRAM_CLOCK_HIGH(); //CLOCK HIGH
		delayms(4);
		gpio_clear(GPIOB,GPIO13); //BP_LA_SRAM_CLOCK_LOW(); //CLOCK LOW
    }*/

    sram_deselect();
    //la_sram_arm_stop();

    setup_spix4w();
	sram_select();
	spiWx4(CMDREADQUAD); //read command
	spiWx4(0);
	spiWx4(0);
	spiWx4(0); //3 byte address
	setup_spix4r(); //read
	spiRx4(); //dummy byte * 3 for fast quad read command
	spiRx4(); //dummy byte
    spiRx4(); //dummy byte
    for(i=0;i<4;i++){
        temp=spiRx4();
        //cdcputc2(temp);

    }
    sram_deselect();
    temp=0xff;

}

void setup_spix1rw(void)
{
    	// enable peripheral
	rcc_periph_clock_enable(BP_FPGA_SPI_CLK);

    //la_sram_mode_spi();
    FPGA_REG_02=0x00;

	// SPI pins of FPGA
	gpio_set_mode(BP_FPGA_MOSI_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FPGA_MOSI_PIN);
	gpio_set_mode(BP_FPGA_CS_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_FPGA_CS_PIN);
	gpio_set_mode(BP_FPGA_CLK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FPGA_CLK_PIN);
	gpio_set_mode(BP_FPGA_MISO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_FPGA_MISO_PIN);

	// setup SPI (cpol=1, cpha=1) +- 1MHz
	spi_reset(BP_FPGA_SPI);
	spi_init_master(BP_FPGA_SPI, SPI_CR1_BAUDRATE_FPCLK_DIV_64, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	spi_set_full_duplex_mode(BP_FPGA_SPI);
	spi_enable(BP_FPGA_SPI);

}

// FIX ME! NEED PROPER SIO0...3 config pins!
void setup_spix4w(void)
{
    spi_reset(BP_FPGA_SPI);
    //la_sram_mode_quad();
    //la_sram_quad_output();
    FPGA_REG_02=0x3;

    //put clock under manual control and low
    gpio_set_mode(BP_FPGA_CLK_PORT,GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,BP_FPGA_CLK_PIN);
    gpio_clear(BP_FPGA_CLK_PORT,BP_FPGA_CLK_PIN);
}

void setup_spix4r(void)
{
    spi_reset(BP_FPGA_SPI);

    //mcu_quadmode direction pin of FPGA
    //la_sram_mode_quad();
    //la_sram_quad_input();
    FPGA_REG_02=0x1;

    //put clock under manual control and low
    gpio_set_mode(BP_FPGA_CLK_PORT,GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,BP_FPGA_CLK_PIN);
    gpio_clear(BP_FPGA_CLK_PORT,BP_FPGA_CLK_PIN);
}

static uint8_t spiRx4(void)
{
	uint8_t received;
	int i;

	//received=0;
	//sram_clock_high();
    received=((FPGA_REG_01&0x000f)<<4);
    //sram_clock_low();
    //sram_clock_high();
    //received|=(FPGA_REG_01&0x000f);
    //sram_clock_low();

	return received;
}

static void spiWx4(uint8_t d)
{

    FPGA_REG_01=(uint16_t) ( ((d)&0x00F0) | ((d>>4)&0x000F) );
    //sram_clock_high();
    //sram_clock_low();
    FPGA_REG_01=(uint16_t) ( ((d<<4)&0x00F0) | ((d)&0x000f)  );
    //sram_clock_high();
    //sram_clock_low();

}
