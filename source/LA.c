

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

static void setup_spix1rw(void);

static void setup_spix4w(void);
static void spiWx4(uint8_t d);
static void spiWx8(uint8_t d);
static void setup_spix4r(void);
static uint8_t spiRx4(void);
static uint8_t spiRx8(void);
static void getSramId(void);

static volatile uint32_t counts;

static volatile uint16_t config_register;

static volatile uint8_t eid[8];
static void getSramId(void){ //should return whole struct
    uint8_t i;

    //read from SRAM test
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0x9F);
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);//24 dummy address bits...
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);
    for(i=0;i<8;i++){
        eid[i]=spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF); //0d=MFID, 5d=KGD, + 6 bytes of density info
    }
    //return temp;
}

void logicAnalyzerSetup(void)
{
    FPGA_REG_03=config_register=0;

    sram_disable();
    sram_deselect();

	setup_spix4w();     //quad write mode

	sram_enable();
	sram_select();
	spiWx4(CMDRESETSPI);//reset to SPI mode
	sram_deselect();
	sram_disable();

    //read SRAM manufacturer, size, unique ID
    setup_spix1rw();
    sram_enable_0();
    sram_select();
    getSramId();
    sram_deselect();
    sram_disable();

    sram_enable_1();
    sram_select();
    getSramId();
    sram_deselect();
    sram_disable();

	sram_enable();
	sram_select();
	spi_xfer(BP_FPGA_SPI, (uint16_t)CMDQUADMODE); //place SRAM into quad mode
	sram_deselect();
	sram_disable();

	logicAnalyzerTest();
}

void logicAnalyzerTest(void){
    uint16_t r;
    uint32_t samples;
    setup_spix4w();     //quad write mode
    sram_enable();
    sram_select();
    spiWx4(CMDWRITE);   //write command
    spiWx4(0);          //3 byte address
    spiWx4(0);
    spiWx4(0);
    FPGA_REG_02=0x00AA;
    FPGA_REG_02=0x0055;
    FPGA_REG_02=0x00AA;
    FPGA_REG_02=0x0055;
    sram_deselect();

    samples=(FPGA_REG_03<<16)|FPGA_REG_04;

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
    r=spiRx8();
    r=spiRx8();
    r=spiRx8();
    r=spiRx8();
    sram_deselect();
}

void logicAnalyzerCaptureStart(void){
    //setup the sram for capture
    setup_spix4w();     //quad write mode
    sram_enable();
    sram_select();
    spiWx4(CMDWRITE);   //write command
    spiWx4(0);          //3 byte address
    spiWx4(0);
    spiWx4(0);
    //FPGA_REG_03|=(0b00001000);//clear sample counter

    //cdcprintf("Samples cleared: %04X\r\n",FPGA_REG_04);
    //cdcprintf("PIN STATES| active: %01b in_full: %01b out_nempty: %01b\r\n",(gpio_get(GPIOC,GPIO10)!=0),(gpio_get(GPIOC,GPIO11)!=0),(gpio_get(GPIOC,GPIO12)!=0) );

	//Bus pirate state machine is in reset, fill FIFO with commands
	//FPGA_REG_07=0xFE00; //LA start command
}


void logicAnalyzerCaptureStop(void){
    uint16_t bpsm_active,i;
    uint32_t samples;
    //FPGA_REG_07=0xFF00; //LA stop
    //FPGA_REG_03&=~(0b1<<7); //release statemachine from reset
    //delayms(2);
    bpsm_active=0;
    sram_deselect();
    //cdcprintf("OP took: %08X cycles\r\n",bpsm_active);


    samples=(FPGA_REG_03<<16)|FPGA_REG_04;
    //cdcprintf("Samples captured: %04X\r\n",samples);
    cdcputc2((uint8_t)(samples>>8));
    cdcputc2((uint8_t)(samples));

    //cdcprintf("Dumping samples\r\n");
    setup_spix4w();
	sram_select();
	spiWx4(CMDREADQUAD); //read command
	spiWx4(0);
	spiWx4(0);
	spiWx4(1); //3 byte address
	setup_spix4r(); //read
	spiRx4(); //dummy byte * 3 for fast quad read command
	spiRx4(); //dummy byte
    spiRx4(); //dummy byte
    for(i=0;i<samples;i++){
        cdcputc2(spiRx8());
    }
    sram_deselect();
}

void setup_spix1rw(void)
{
    int i;
    	// enable peripheral
	rcc_periph_clock_enable(BP_FPGA_SPI_CLK);

    FPGA_REG_03=config_register=0;

	// SPI pins of FPGA
	gpio_set_mode(BP_FPGA_MOSI_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FPGA_MOSI_PIN);
	gpio_set_mode(BP_FPGA_CS_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_FPGA_CS_PIN);
	gpio_set_mode(BP_FPGA_CLK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FPGA_CLK_PIN);
	gpio_set_mode(BP_FPGA_MISO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_FPGA_MISO_PIN);

	// setup SPI (cpol=1, cpha=1) +- 1MHz
	spi_reset(BP_FPGA_SPI);
	spi_init_master(BP_FPGA_SPI, SPI_CR1_BAUDRATE_FPCLK_DIV_2, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	spi_set_full_duplex_mode(BP_FPGA_SPI);
	spi_enable(BP_FPGA_SPI);

	sram_deselect();

    //la_sram_mode_spi();
    config_register&=~(0b110); //clear quad mode
    config_register|=0b001;// spi=true
    FPGA_REG_03=config_register;

}

// FIX ME! NEED PROPER SIO0...3 config pins!
void setup_spix4w(void)
{
    spi_reset(BP_FPGA_SPI);
    config_register&=~(0b1); //clear spi mode
    config_register|=0b110;// dir=output, quad=true
    FPGA_REG_03=config_register;

    //put clock under manual control and low
    //gpio_set_mode(BP_FPGA_CLK_PORT,GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,BP_FPGA_CLK_PIN);
    //gpio_clear(BP_FPGA_CLK_PORT,BP_FPGA_CLK_PIN);
}

void setup_spix4r(void)
{
    spi_reset(BP_FPGA_SPI);
    config_register&=~(0b101); //clear dir=output and spi mode
    config_register|=0b010;// dir=input, quad=true
    FPGA_REG_03=config_register;

    //put clock under manual control and low
    //gpio_set_mode(BP_FPGA_CLK_PORT,GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,BP_FPGA_CLK_PIN);
    //gpio_clear(BP_FPGA_CLK_PORT,BP_FPGA_CLK_PIN);
}

static uint8_t spiRx4(void)
{
	uint8_t received;
	received=0;
    received=((FPGA_REG_02&0x00ff)<<8);
    received|=(FPGA_REG_02&0x00ff);
	return received;
}

static uint8_t spiRx8(void)
{
	uint8_t received;
    received=((FPGA_REG_02));
	return received;
}

static void spiWx4(uint8_t d)
{
    FPGA_REG_02=(uint16_t) ( ((d)&0x00F0) | ((d>>4)&0x000F) );
    FPGA_REG_02=(uint16_t) ( ((d<<4)&0x00F0) | ((d)&0x000f)  );
}

static void spiWx8(uint8_t d)
{
    FPGA_REG_02=(uint16_t)d&0x00FF;
}

