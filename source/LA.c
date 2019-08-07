

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

/*static enum _stop
        {
         NORMAL=0,
         OVERFLOW
        } stop=NORMAL;*/


static volatile uint32_t counts;

void logicAnalyzerSetup(void)
{
    uint8_t temp;
    uint16_t i,samples;
    char buff[256];

    //la_sram_mode_setup();
    //la_sram_quad_setup();
    FPGA_REG_03=0b1100000000;

    sram_deselect();
    //delayus(100);
	//send mode reset command just in case

	setup_spix4w(); //write
	sram_select();
	spiWx4(CMDRESETSPI);
	sram_deselect();

    //read from SRAM test
    setup_spix1rw();
    sram_select_0();	// cs low
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0x9F);
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);//24 dummy address bits...
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);
    //for(i=0;i<8;i++){
    //    temp=buff[i]=spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF); //0d=MFID, 5d=KGD, + 6 bytes of density info
    //}

    temp=spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);
    cdcprintf("SRAM 0 test: %02X ",temp);
    if(temp!=0x0D){
        cdcprintf("FAIL!\r\n");
    }else{
        cdcprintf("PASS!\r\n");
    }
    //release FPGA into program mode
    sram_deselect();			// release cs

    sram_select_1();	// cs low
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0x9F);
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);//24 dummy address bits...
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);
    //for(i=0;i<8;i++){
    //    temp=buff[i]=spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF); //0d=MFID, 5d=KGD, + 6 bytes of density info
    //}
    temp=spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);
    cdcprintf("SRAM 1 test: %02X ",temp);
    if(temp!=0x0D){
        cdcprintf("FAIL!\r\n");
    }else{
        cdcprintf("PASS!\r\n");
    }
    //release FPGA into program mode
    sram_deselect();			// release cs

	//quad mode
	//sram_select_0();
	sram_select();
	spi_xfer(BP_FPGA_SPI, (uint16_t)CMDQUADMODE);
	sram_deselect();

    //sram_select_1();
	//spi_xfer(BP_FPGA_SPI, (uint16_t)CMDQUADMODE);
	//sram_deselect();

//setup io
    FPGA_REG_00=0x00FF;
    FPGA_REG_01=0x0000;


    //setup the sram for capture
    setup_spix4w(); //write
    sram_select();
    spiWx4(CMDWRITE); //write command
    spiWx4(0);
    spiWx4(0);
    spiWx4(0); //3 byte address
    //for(i=0;i<8;i++){
    //    spiWx8(0xff);
    //}
    //sram_deselect();

    FPGA_REG_03|=(0b11000);//clear sample counter | start capture
    //cdcprintf("FPGA_REG_3: %04X\r\n",FPGA_REG_03);

    //now do some stuff
    //setup the PWM
	FPGA_REG_05=0x1;
	FPGA_REG_06=0x1;
    //delay
    //delayms(1);
    //change the PWM
	//FPGA_REG_05=0x10;
	//FPGA_REG_06=0x10;
	delayms(1);

    //cdcprintf("FPGA_REG_3: %04X\r\n",FPGA_REG_03);*/

	FPGA_REG_03&=~((0b1<<4));//stop capture

    cdcprintf("FPGA_REG_3: %08b\r\n",FPGA_REG_03);

    sram_deselect();
    samples=FPGA_REG_04;
    cdcprintf("Samples captured: %04X\r\n",samples);
    //cdcprintf("FPGA_REG_3: %04X\r\n",FPGA_REG_03);

    //TODO: read samples back to cdc2
    cdcputc2((uint8_t)(samples>>8));
    cdcputc2((uint8_t)(samples));

    cdcprintf("Dumping samples\r\n");
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
    for(i=0;i<samples;i++){
        cdcputc2(spiRx8());
    }
    sram_deselect();


/*	setup_spix4w(); //write
	sram_select();
	spiWx4(CMDWRITE); //write command
	spiWx4(0);
	spiWx4(0);
	spiWx4(0); //3 byte address
    for(i=0;i<4;i++){
        spiWx8(0xaa);
        spiWx8(0x55);
    }
    sram_deselect();

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
    temp=spiRx8();
    cdcprintf("SRAM quad test: %02X ",temp);
    if(temp!=0xaa){
        cdcprintf("FAIL!\r\n");
    }else{
        cdcprintf("PASS!\r\n");
    }
    sram_deselect();


    setup_spix4w(); //write
    sram_select();
    spiWx4(CMDWRITE); //write command
    spiWx4(0);
    spiWx4(0);
    spiWx4(0); //3 byte address
    for(i=0;i<0xffff;i++){

        spiWx8((uint8_t)i);
    }
    spiWx8((uint8_t)i);//one extra, need even number of samples
    sram_deselect();

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
    for(i=0;i<0xffff;i++){
        temp=spiRx8();
        if(temp!=(uint8_t)i){
            cdcprintf("SRAM big test failed at: %04X!=%02X \r\n",i,temp);
            break;
        }

    }

    sram_deselect();

*/







    temp=0xff;

}

void setup_spix1rw(void)
{
    int i;
    	// enable peripheral
	rcc_periph_clock_enable(BP_FPGA_SPI_CLK);

    //la_sram_mode_spi();
    FPGA_REG_03&=~(0b11); //clear quad mode
    FPGA_REG_03|=(0b1<<2); //setup spi mode
    i=FPGA_REG_03;

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
    FPGA_REG_03&=~(0b1<<2); //clear spi mode
    FPGA_REG_03|=0b11;

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
    //FPGA_REG_02=0x1;
    FPGA_REG_03|=0b1;
    FPGA_REG_03&=~(0b110);

    //put clock under manual control and low
    gpio_set_mode(BP_FPGA_CLK_PORT,GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,BP_FPGA_CLK_PIN);
    gpio_clear(BP_FPGA_CLK_PORT,BP_FPGA_CLK_PIN);
}

static uint8_t spiRx4(void)
{
	uint8_t received;
	int i;

	received=0;
	//sram_clock_high();
    received=((FPGA_REG_02&0x00ff)<<8);
    //sram_clock_low();
    //sram_clock_high();
    received|=(FPGA_REG_02&0x00ff);
    //sram_clock_low();

	return received;
	//i=FPGA_REG_01;
	//return (i<<8) | (FPGA_REG_01&0x00ff);
}

static uint8_t spiRx8(void)
{
	uint8_t received;
	int i;

	//sram_clock_high();
    received=((FPGA_REG_02));
    //sram_clock_low();
	return received;
	//i=FPGA_REG_01;
	//return (i<<8) | (FPGA_REG_01&0x00ff);
}

static void spiWx4(uint8_t d)
{

    FPGA_REG_02=(uint16_t) ( ((d)&0x00F0) | ((d>>4)&0x000F) );
    //sram_clock_high();
    //sram_clock_low();
    FPGA_REG_02=(uint16_t) ( ((d<<4)&0x00F0) | ((d)&0x000f)  );
    //sram_clock_high();
    //sram_clock_low();

}

static void spiWx8(uint8_t d)
{

    FPGA_REG_02=(uint16_t)d&0x00FF;
    //sram_clock_high();
    //sram_clock_low();

}

