

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include "buspirate.h"
#include "LA.h"
#include "cdcacm.h"
#include "delay.h"

static void setup_spix1rw(void);

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

	//send mode reset command just in case
	setup_spix4w(); //write
	gpio_clear(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);
	delayms(1);
	spiWx4(CMDRESETSPI);
	gpio_set(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);

    //read from SRAM test
    setup_spix1rw();
    gpio_clear(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);	// cs low
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0x9F);
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);//24 dummy address bits...
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);
    for(i=0;i<8;i++){
        temp=buff[i]=spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF); //0d=MFID, 5d=KGD, + 6 bytes of density info
    }
    //release FPGA into program mode
    gpio_set(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);			// release cs

	//force to sequencial mode just in case
	//NOT USED ON CURRENT CHIP
	/*setup_spix1rw();
	gpio_clear(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);
	delayms(1);
	spi_xfer(BP_FPGA_SPI, (uint16_t)CMDWRITERREG); //spiWx1(CMDWRITERREG);//write register
	spi_xfer(BP_FPGA_SPI, (uint16_t)CMDSEQLMODE); //spiWx1(CMDSEQLMODE);
	gpio_set(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);
	delayms(1);*/

	//quad mode
	gpio_clear(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);
	delayms(1);
	spi_xfer(BP_FPGA_SPI, (uint16_t)CMDQUADMODE); //spiWx1(CMDQUADMODE);
	gpio_set(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);

	//clear the sram for testing purposes
	setup_spix4w(); //write
	gpio_clear(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);
	delayms(1);
	spiWx4(CMDWRITE); //write command
	spiWx4(0);
	spiWx4(0);
	spiWx4(0); //3 byte address
	//for(i=0;i<4;i++)
		//spiWx4(i);
    la_sram_arm_start(); //attach SRAM to LATCH
    for(i=0;i<500;i++){ //clock in some data to sram
        delayms(4);
        gpio_set(GPIOB,GPIO13); //BP_LA_SRAM_CLOCK_HIGH(); //CLOCK HIGH
		delayms(4);
		gpio_clear(GPIOB,GPIO13); //BP_LA_SRAM_CLOCK_LOW(); //CLOCK LOW
        delayms(4);
        gpio_set(GPIOB,GPIO13); //BP_LA_SRAM_CLOCK_HIGH(); //CLOCK HIGH
		delayms(4);
		gpio_clear(GPIOB,GPIO13); //BP_LA_SRAM_CLOCK_LOW(); //CLOCK LOW
    }

    gpio_set(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);
    la_sram_arm_stop();

    setup_spix4w();
	gpio_clear(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);
	spiWx4(CMDREADQUAD); //read command
	spiWx4(0);
	spiWx4(0);
	spiWx4(0); //3 byte address
	setup_spix4r(); //read
	spiRx4(); //dummy byte * 3 for fast quad read command
	spiRx4(); //dummy byte
    spiRx4(); //dummy byte
    for(i=0;i<500;i++){
        temp=spiRx4();
        cdcputc2(temp);

    }
    gpio_set(BP_FPGA_CS_PORT, BP_FPGA_CS_PIN);
    temp=0xff;

}

void setup_spix1rw(void)
{
    	// enable peripheral
	rcc_periph_clock_enable(BP_FPGA_SPI_CLK);

    la_sram_mode_spi();

	// SPI pins of FPGA
	gpio_set_mode(BP_FPGA_MOSI_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FPGA_MOSI_PIN);
	gpio_set_mode(BP_FPGA_CS_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_FPGA_CS_PIN);
	gpio_set_mode(BP_FPGA_CLK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FPGA_CLK_PIN);
	gpio_set_mode(BP_FPGA_MISO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_FPGA_MISO_PIN);

	// setup SPI (cpol=1, cpha=1) +- 1MHz
	spi_reset(BP_FPGA_SPI);
	spi_init_master(BP_FPGA_SPI, SPI_CR1_BAUDRATE_FPCLK_DIV_8, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	spi_set_full_duplex_mode(BP_FPGA_SPI);
	spi_enable(BP_FPGA_SPI);

}

// FIX ME! NEED PROPER SIO0...3 config pins!
void setup_spix4w(void)
{
    spi_reset(BP_FPGA_SPI);

    la_sram_mode_quad();
    la_sram_quad_output();

    //put clock under manual control and low
    gpio_set_mode(GPIOB,GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,GPIO13);
    gpio_clear(GPIOB,GPIO13);

	// set SIO pins to output to enable quadspi
    gpio_set_mode(BP_FPGA_SIO0_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_FPGA_SIO0_PIN); //SIO0
    gpio_set_mode(BP_FPGA_SIO1_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_FPGA_SIO1_PIN); //SIO0
    gpio_set_mode(BP_FPGA_SIO2_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_FPGA_SIO2_PIN); //SIO0
    gpio_set_mode(BP_FPGA_SIO3_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_FPGA_SIO3_PIN); //SIO0

}

void setup_spix4r(void)
{
    spi_reset(BP_FPGA_SPI);

    //mcu_quadmode direction pin of FPGA
    la_sram_mode_quad();
    la_sram_quad_input();

    //put clock under manual control and low
    gpio_set_mode(GPIOB,GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,GPIO13);
    gpio_clear(GPIOB,GPIO13);

	// set SIO pins to input
    gpio_set_mode(BP_FPGA_SIO0_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_FPGA_SIO0_PIN); //SIO0
    gpio_set_mode(BP_FPGA_SIO1_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_FPGA_SIO1_PIN); //SIO0
    gpio_set_mode(BP_FPGA_SIO2_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_FPGA_SIO2_PIN); //SIO0
    gpio_set_mode(BP_FPGA_SIO3_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_FPGA_SIO3_PIN); //SIO0
}

static uint8_t spiRx4(void)
{
	uint8_t received;
	int i;

	received=0;

	//READ
    for(i=0; i<2; i++)
	{
        gpio_set(GPIOB,GPIO13); //BP_LA_SRAM_CLOCK_HIGH(); //CLOCK HIGH
        received<<=1;
        received|=(gpio_get(BP_FPGA_SIO3_PORT, BP_FPGA_SIO3_PIN)?1:0);
        received<<=1;
        received|=(gpio_get(BP_FPGA_SIO2_PORT, BP_FPGA_SIO2_PIN)?1:0);
        received<<=1;
        received|=(gpio_get(BP_FPGA_SIO1_PORT, BP_FPGA_SIO1_PIN)?1:0);
        received<<=1;
        received|=(gpio_get(BP_FPGA_SIO0_PORT, BP_FPGA_SIO0_PIN)?1:0);
        gpio_clear(GPIOB,GPIO13); //BP_LA_SRAM_CLOCK_LOW(); //CLOCK LOW
	}

	return received;
}

static void spiWx4(uint8_t d)
{
	int i;
	uint8_t mask;

	mask=0x80;

	for(i=0; i<2; i++)
	{
		if(d&mask) gpio_set(BP_FPGA_SIO3_PORT, BP_FPGA_SIO3_PIN);
			else gpio_clear(BP_FPGA_SIO3_PORT, BP_FPGA_SIO3_PIN);
		mask>>=1;
		if(d&mask) gpio_set(BP_FPGA_SIO2_PORT, BP_FPGA_SIO2_PIN);
			else gpio_clear(BP_FPGA_SIO2_PORT, BP_FPGA_SIO2_PIN);
		mask>>=1;
		if(d&mask) gpio_set(BP_FPGA_SIO1_PORT, BP_FPGA_SIO1_PIN);
			else gpio_clear(BP_FPGA_SIO1_PORT, BP_FPGA_SIO1_PIN);
		mask>>=1;
		if(d&mask) gpio_set(BP_FPGA_SIO0_PORT, BP_FPGA_SIO0_PIN);
			else gpio_clear(BP_FPGA_SIO0_PORT, BP_FPGA_SIO0_PIN);
		mask>>=1;

		gpio_set(GPIOB,GPIO13); //BP_LA_SRAM_CLOCK_HIGH(); //CLOCK HIGH

		gpio_clear(GPIOB,GPIO13); //BP_LA_SRAM_CLOCK_LOW(); //CLOCK LOW
	}

	return;
}
