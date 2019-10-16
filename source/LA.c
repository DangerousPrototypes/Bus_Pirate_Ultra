

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


static void getSramId(void){ //should return whole struct
    uint8_t temp[8],i;

    //read from SRAM test
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0x9F);
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);//24 dummy address bits...
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);
    spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF);
    for(i=0;i<8;i++){
        temp[i]=spi_xfer(BP_FPGA_SPI, (uint16_t) 0xFF); //0d=MFID, 5d=KGD, + 6 bytes of density info
    }
    //return temp;
}

void logicAnalyzerSetup(void)
{
    sram_deselect();

	setup_spix4w();     //quad write mode
	sram_select();
	spiWx4(CMDRESETSPI);//reset to SPI mode
	sram_deselect();

    //read SRAM manufacturer, size, unique ID
    setup_spix1rw();
    sram_select_0();	// cs low
    getSramId();
    sram_deselect();	// release cs

    sram_select_1();	// cs low
    getSramId();
    sram_deselect();	// release cs

	sram_select();
	spi_xfer(BP_FPGA_SPI, (uint16_t)CMDQUADMODE); //place SRAM into quad mode
	sram_deselect();
}

void logicAnalyzerTest(void){
    FPGA_REG_00=0x00FF; //normal output|output enable all pins
    FPGA_REG_01=0x0000; //direction output(only non-spi pins)|low(unimplemented)
    logicAnalyzerCaptureStart();
    FPGA_REG_07=0x81FF;//all IO high command
	FPGA_REG_07=0x8100;//all IO low command
	FPGA_REG_07=0x0855;//write 8 bits of 0x55
	FPGA_REG_07=0x04AA;//write 4 bits of 0xAA
    FPGA_REG_07=0x08AA;//0xaa
    FPGA_REG_07=0x0855;//0x55
    FPGA_REG_07=0x08FF;//0xff
    FPGA_REG_07=0x0800;//0x00
    FPGA_REG_07=0x84FF;//delay for 0xFF cycles

    FPGA_REG_07=0x84FF;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x84FF;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x84FF;
    FPGA_REG_07=0x84FF;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x84FF;
    FPGA_REG_07=0x84FF;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x84FF;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x84FF;
    FPGA_REG_07=0x84FF;

    FPGA_REG_07=0x81FF;//all IO high
	//while(FPGA_REG_08&&0b1);
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;

    FPGA_REG_07=0x84FF;

    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    logicAnalyzerCaptureStop();
}

void logicAnalyzerCaptureStart(void){
    //setup the sram for capture
    setup_spix4w();     //quad write mode
    sram_select();
    spiWx4(CMDWRITE);   //write command
    spiWx4(0);          //3 byte address
    spiWx4(0);
    spiWx4(0);
    FPGA_REG_03|=(0b00001000);//clear sample counter

    //cdcprintf("Samples cleared: %04X\r\n",FPGA_REG_04);
    //cdcprintf("PIN STATES| active: %01b in_full: %01b out_nempty: %01b\r\n",(gpio_get(GPIOC,GPIO10)!=0),(gpio_get(GPIOC,GPIO11)!=0),(gpio_get(GPIOC,GPIO12)!=0) );

	//Bus pirate state machine is in reset, fill FIFO with commands
	//FPGA_REG_07=0xFE00; //LA start command
}


void logicAnalyzerCaptureStop(void){
    uint16_t bpsm_active,i,samples;
    //FPGA_REG_07=0xFF00; //LA stop
    //FPGA_REG_03&=~(0b1<<7); //release statemachine from reset
    //delayms(2);
    bpsm_active=0;
    while(true){
        i=GPIOC_IDR;
        i=i&(0b1<<10);
        if(i==0){
                break;
        }else if(bpsm_active>0xfff){
            cdcprintf("BP state machine timeout!\r\n");
            cdcprintf("PIN STATES| active: %01b in_full: %01b out_nempty: %01b\r\n",(gpio_get(GPIOC,GPIO10)!=0),(gpio_get(GPIOC,GPIO11)!=0),(gpio_get(GPIOC,GPIO12)!=0) );
            cdcprintf("BPSM state: %04X\r\n",FPGA_REG_09);
           // cdcprintf("BPSM command: %04X\r\n",FPGA_REG_10);
            return;
        }else{
            bpsm_active=bpsm_active+1;
        }
    }
    sram_deselect();
    //cdcprintf("OP took: %08X cycles\r\n",bpsm_active);


    samples=FPGA_REG_04;
    //cdcprintf("Samples captured: %04X\r\n",samples);
    cdcputc2((uint8_t)(samples>>8));
    cdcputc2((uint8_t)(samples));

    //cdcprintf("Dumping samples\r\n");
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
    FPGA_REG_03&=~(0b1<<2); //clear spi mode
    FPGA_REG_03|=0b11;

    //put clock under manual control and low
    gpio_set_mode(BP_FPGA_CLK_PORT,GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,BP_FPGA_CLK_PIN);
    gpio_clear(BP_FPGA_CLK_PORT,BP_FPGA_CLK_PIN);
}

void setup_spix4r(void)
{
    spi_reset(BP_FPGA_SPI);
    FPGA_REG_03|=0b1;
    FPGA_REG_03&=~(0b110);

    //put clock under manual control and low
    gpio_set_mode(BP_FPGA_CLK_PORT,GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,BP_FPGA_CLK_PIN);
    gpio_clear(BP_FPGA_CLK_PORT,BP_FPGA_CLK_PIN);
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


/*void logicAnalyzerSetup(void)
{
    uint8_t temp;
    uint16_t i,samples;
    uint32_t bpsm_active=0;
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

    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,GPIO10|GPIO11|GPIO12);

    FPGA_REG_03|=(0b10001000);//clear sample counter | start capture

    cdcprintf("Samples cleared: %04X\r\n",FPGA_REG_04);
    cdcprintf("PIN STATES| active: %01b in_full: %01b out_nempty: %01b\r\n",(gpio_get(GPIOC,GPIO10)!=0),(gpio_get(GPIOC,GPIO11)!=0),(gpio_get(GPIOC,GPIO12)!=0) );
//set_io bp_active 137 #C10
//set_io bp_fifo_in_full 136 #C11
//set_io bp_fifo_out_nempty 135 #C12

	//Bus pirate state machine is in reset, fill FIFO with commands
	FPGA_REG_07=0xFE00; //LA start command
    FPGA_REG_07=0x81FF;//all IO high command
	FPGA_REG_07=0x8100;//all IO low command
	FPGA_REG_07=0x0855;//write 8 bits of 0x55
	FPGA_REG_07=0x04AA;//write 4 bits of 0xAA
    FPGA_REG_07=0x08AA;//0xaa
    FPGA_REG_07=0x0855;//0x55
    FPGA_REG_07=0x08FF;//0xff
    FPGA_REG_07=0x0800;//0x00
    FPGA_REG_07=0x84FF;//delay for 0xFF cycles

    FPGA_REG_07=0x84FF;
        FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x84FF;
        FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x84FF;
    FPGA_REG_07=0x84FF;
        FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x84FF;
    FPGA_REG_07=0x84FF;
        FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x84FF;
        FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x84FF;
    FPGA_REG_07=0x84FF;

    FPGA_REG_07=0x81FF;//all IO high
	//while(FPGA_REG_08&&0b1);
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;

    FPGA_REG_07=0x84FF;

    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0x08AA;
    FPGA_REG_07=0x0855;
    FPGA_REG_07=0xFF00; //LA stop

	//TODO: some complete operations signal!

    //while();
    //while(FPGA_REG_03&0b100000); //wait for complete
    FPGA_REG_03&=~(0b1<<7);
    delayus(2);
    //samples=0;
    while(true){
        i=GPIOC_IDR;
        i=i&(0b1<<10);
        if(i==0){
                break;
        }else if(bpsm_active>0xfff){
            cdcprintf("PIN STATES| active: %01b in_full: %01b out_nempty: %01b\r\n",(gpio_get(GPIOC,GPIO10)!=0),(gpio_get(GPIOC,GPIO11)!=0),(gpio_get(GPIOC,GPIO12)!=0) );
            cdcprintf("BPSM state: %04X\r\n",FPGA_REG_09);
            cdcprintf("BPSM command: %04X\r\n",FPGA_REG_10);
            return;
        }else{
            bpsm_active=bpsm_active+1;
        }
    }

    cdcprintf("OP took: %08X cycles\r\n",bpsm_active);

    //cdcprintf("FPGA_REG_3: %08b\r\n",FPGA_REG_03);

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
    temp=0xff;

}*/


