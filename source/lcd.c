#include <stdint.h>
#include "delay.h"
#include "buspirate.h"
#include "lcd.h"
#include "font.h"
#include "fpga.h"
#include "UI.h"
#include "ADC.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

void modeLabelsSetupLCD(const char *labels){
    uint8_t i,cnt=0;
	disableLCD();
    writeFileToLCD();

    for(i=0;i<10;i++){
        /*
        X1 = width of screen (239) - pixels from edge of first label (6) + font height-1 (13) - position (i) * pixels between label corners (24)
        X2 = width of screen (239) - pixels from edge of first label (6) - position (i) * pixels between label corners (24)
        Y1 = distance from top of screen (155)
        Y2 = font width (10) * maximum characters allowed (5)
        */
        setBoundingBox( (239-(6+13))-(i*24), (239-6)-(i*24), 155, 155+((10*5)-1));
        gpio_set(BP_LCD_DP_PORT,BP_LCD_DP_PIN);
        gpio_clear(BP_LCD_CS_PORT, BP_LCD_CS_PIN);
        switch(i){
        case 8:
            break;
        case 9:
            writeCharacterToLCD('G');
            writeCharacterToLCD('N');
            writeCharacterToLCD('D');
            break;
        default:
            while(labels[cnt]!=0x00 && cnt<(6*8)){ //5 characters per label maximum (plus 0x00) * 8 labels as a safety
                writeCharacterToLCD(labels[cnt]);
                cnt++;
            }
            cnt++;
            break;
        }
        gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);
        gpio_set(BP_LCD_CS_PORT, BP_LCD_CS_PIN);
    }
    writePSUstate();
    updateLCD(1);
	enableLCD();

}

//first pin label top is at 240px-6px, 240-6-10, next one starts -10px, left at 234

void updateLCD(uint8_t initial){
    uint8_t i;
    uint32_t temp;

    //DIO1-8, Vout
    for(i=0;i<9;i++){

        switch(i){
            case 8:
                temp = voltage(BP_VOUT_CHAN, 1);
                break;
            default:
                 FPGA_REG_0A=(0b0|(i<<1));
                 delayus(10);
                 temp = voltage(BP_ADC_CHAN, 1);
                 break;
        }

        /*
        X1 = width of screen (239) - pixels from edge of first label (6) + font height-1 (13) - position (i) * pixels between label corners (24)
        X2 = width of screen (239) - pixels from edge of first label (6) - position (i) * pixels between label corners (24)
        Y1 = distance from top of screen (270)
        Y2 = font width (10) * maximum characters allowed (5) + distance from top of screen (270)
        */
        setBoundingBox( (239-(6+13))-(i*24), (239-6)-(i*24), 270, 270+((10*4)-1));
        gpio_set(BP_LCD_DP_PORT,BP_LCD_DP_PIN);
        gpio_clear(BP_LCD_CS_PORT, BP_LCD_CS_PIN);
        writeByteNumberToLCD(temp/1000);
        writeCharacterToLCD('.'); //.
        writeByteNumberToLCD((temp%1000)/100);
        if(initial) writeCharacterToLCD('V'); //V
        gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);
        gpio_set(BP_LCD_CS_PORT, BP_LCD_CS_PIN);
    }

}

void writePSUstate(void){
    uint8_t i=8;
    setBoundingBox( (239-(6+13))-(i*24), (239-6)-(i*24), 155, 155+(10*5));
    gpio_set(BP_LCD_DP_PORT,BP_LCD_DP_PIN);
    gpio_clear(BP_LCD_CS_PORT, BP_LCD_CS_PIN);
    if(modeConfig.psu==0){
        writeCharacterToLCD('V');
        writeCharacterToLCD('r');
        writeCharacterToLCD('e');
        writeCharacterToLCD('f');
    }else{
        writeCharacterToLCD('V');
        writeCharacterToLCD('o');
        writeCharacterToLCD('u');
        writeCharacterToLCD('t');
    }
    gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);
    gpio_set(BP_LCD_CS_PORT, BP_LCD_CS_PIN);

}


//const uint32_t font_lut24[]={0x5a5a5b,0x999999,0xb5b5b5,0xd6d6d6};
const uint32_t font_lut24[]={0x5a5a5b,0x999999,0xd6d6d6,0xb5b5b5}; //QVector(8123, 1239, 2989, 1089)
uint16_t font_lut16RGB[]={0,0,0,0};

void writeByteNumberToLCD(uint8_t c){
    writeCharacterToLCD(c+0x30);
}

void writeCharacterToLCD(uint8_t c){
    uint8_t i;
    uint16_t color;

    c=c-0x21;

	for(i=0; i<35; i++)
	{
        color=font_lut16RGB[(font[c][i]>>6)&0b11];
        spi_xfer(BP_LCD_SPI, (uint16_t)color>>8);
        spi_xfer(BP_LCD_SPI, (uint16_t)color>>0);
        color=font_lut16RGB[(font[c][i]>>4)&0b11];
        spi_xfer(BP_LCD_SPI, (uint16_t)color>>8);
        spi_xfer(BP_LCD_SPI, (uint16_t)color>>0);
        color=font_lut16RGB[(font[c][i]>>2)&0b11];
        spi_xfer(BP_LCD_SPI, (uint16_t)color>>8);
        spi_xfer(BP_LCD_SPI, (uint16_t)color>>0);
        color=font_lut16RGB[(font[c][i]>>0)&0b11];
        spi_xfer(BP_LCD_SPI, (uint16_t)color>>8);
        spi_xfer(BP_LCD_SPI, (uint16_t)color>>0);
	}
}

void writeFileToLCD(void){
    uint32_t bigcount;

	setBoundingBox(0, 240, 0, 320);
	//setup flash
	#define FLCMD_FREAD	0x0B		// Read Data Bytes (READ)
    gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);	// cs low
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_FREAD);
	spi_xfer(BP_FS_SPI, (uint16_t) ((0x30000>>16)&0x000000FF));	// address
	spi_xfer(BP_FS_SPI, (uint16_t) ((0x30000>>8)&0x000000FF));		//
	spi_xfer(BP_FS_SPI, (uint16_t) (0x30000&0x000000FF));		//

	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF); //read dummy byte

    gpio_set(BP_LCD_DP_PORT,BP_LCD_DP_PIN);
    gpio_clear(BP_LCD_CS_PORT, BP_LCD_CS_PIN);
	for(bigcount=0; bigcount<=153600; bigcount++)
	{
        spi_xfer(BP_LCD_SPI, (uint16_t)spi_xfer(BP_FS_SPI, (uint16_t) 0xFF));
	}
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);
    gpio_set(BP_LCD_CS_PORT, BP_LCD_CS_PIN);
}


void clearLCD(void){
    uint16_t color, blue, red, green, black, white;
    uint16_t x,y;
    blue=0b0000000000011111;
    red =0b1111100000000000;
    green=0b0000011111100000;
    black=0x0000;
    white=0xffff;

    setBoundingBox(0, 240, 0, 320);

    color=red;
    gpio_set(BP_LCD_DP_PORT,BP_LCD_DP_PIN);
    gpio_clear(BP_LCD_CS_PORT, BP_LCD_CS_PIN);
    for(x=0;x<240;x++){
        for(y=0;y<320;y++){
            spi_xfer(BP_LCD_SPI, (uint16_t) color>>8);
            spi_xfer(BP_LCD_SPI, (uint16_t) color&0xff);
        }
    }
    gpio_set(BP_LCD_CS_PORT, BP_LCD_CS_PIN);

}


void setBoundingBox(uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye){
    //setup write area
    //start must always be =< end
    writeLCDcommand(0x2A); //column start and end set
    writeLCDdata(xs>>8);
    writeLCDdata(xs&0xff); //0
    writeLCDdata(xe>>8);
    writeLCDdata(xe&0xff); //LCD_W (240)
    writeLCDcommand(0x2B); //row start and end set
    writeLCDdata(ys>>8);
    writeLCDdata(ys&0xff); //0
    writeLCDdata(ye>>8);
    writeLCDdata(ye&0xff); //320
    writeLCDcommand(0x2C);//Memory Write
}

void writeLCDcommand(uint16_t command){
    //D/C low for command
    gpio_clear(BP_LCD_DP_PORT,BP_LCD_DP_PIN);
    gpio_clear(BP_LCD_CS_PORT, BP_LCD_CS_PIN);
    spi_xfer(BP_LCD_SPI, (uint16_t) command);
    gpio_set(BP_LCD_CS_PORT, BP_LCD_CS_PIN);
}
void writeLCDdata(uint16_t data){
    //D/C high for data
    gpio_set(BP_LCD_DP_PORT,BP_LCD_DP_PIN);
    gpio_clear(BP_LCD_CS_PORT, BP_LCD_CS_PIN);
    spi_xfer(BP_LCD_SPI, (uint16_t) data);
    gpio_set(BP_LCD_CS_PORT, BP_LCD_CS_PIN);
}
void disableLCD(void){
    writeLCDcommand(0x28);
}

void enableLCD(void){
    writeLCDcommand(0x29);
}

void setupLCD(void){
    uint8_t i,c;
    uint16_t temp;
  	// enable peripheral
	rcc_periph_clock_enable(BP_LCD_SPI_CLK);

	// SPI pins of LCD
	gpio_set_mode(BP_LCD_MOSI_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_LCD_MOSI_PIN);
	gpio_set_mode(BP_LCD_CLK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_LCD_CLK_PIN);
	gpio_set_mode(BP_LCD_MISO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_LCD_MISO_PIN);

	// control pins
	gpio_set_mode(BP_LCD_CS_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LCD_CS_PIN);
	gpio_set_mode(BP_LCD_DP_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,BP_LCD_DP_PIN);
	gpio_set_mode(BP_LCD_RESET_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LCD_RESET_PIN);
	gpio_set_mode(BP_LCD_AUX2_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BP_LCD_AUX2_PIN);

	//PWM

	// setup SPI (cpol=1, cpha=1) +- 1MHz
	spi_reset(BP_LCD_SPI);
	spi_init_master(BP_LCD_SPI, SPI_CR1_BAUDRATE_FPCLK_DIV_2, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	spi_set_full_duplex_mode(BP_LCD_SPI);
	spi_enable(BP_LCD_SPI);

	//RESET high for active
	gpio_set(BP_LCD_RESET_PORT,BP_LCD_RESET_PIN);
	//CS high for hold
	gpio_set(BP_LCD_CS_PORT, BP_LCD_CS_PIN);

	//precalculate the 24bit color values into 16bit RGB for our display
    for(int i=0; i<4;i++){

       c=((font_lut24[i]&0xff0000)>>16);
       temp=(uint16_t)((c&0b11111000)<<8);
       c=((font_lut24[i]&0xff00)>>8);
       temp|=((c&0b11111100)<<3);
       c=((font_lut24[i]&0xff));
       temp|=((c&0b11111000)>>3);
       font_lut16RGB[i]=temp;
    }
}

void initializeILI9341(void){

gpio_clear(BP_LCD_RESET_PORT,BP_LCD_RESET_PIN);
delayms(120);
gpio_set(BP_LCD_RESET_PORT,BP_LCD_RESET_PIN);
delayms(120);

writeLCDcommand(0x11);//Sleep out, DC/DC converter, internal oscillator, panel scanning "enable"
delayms(120);

writeLCDcommand(0x01);//soft reset
delayms(1000);
//power control A
writeLCDcommand(0xCB);
writeLCDdata(0x39);
writeLCDdata(0x2C);
writeLCDdata(0x00);
writeLCDdata(0x34);
writeLCDdata(0x02);

//power control B
writeLCDcommand(0xCF);
writeLCDdata(0x00);
writeLCDdata(0xC1);
writeLCDdata(0x30);

//driver timing control A
writeLCDcommand(0xE8);
writeLCDdata(0x85);
writeLCDdata(0x00);
writeLCDdata(0x78);

//driver timing control B
writeLCDcommand(0xEA);
writeLCDdata(0x00);
writeLCDdata(0x00);

//power on sequence control
writeLCDcommand(0xED);
writeLCDdata(0x64);
writeLCDdata(0x03);
writeLCDdata(0x12);
writeLCDdata(0x81);

//pump ratio control
writeLCDcommand(0xF7);
writeLCDdata(0x20);

//power control,VRH[5:0]
writeLCDcommand(0xC0);
writeLCDdata(0x23);

//Power control,SAP[2:0];BT[3:0]
writeLCDcommand(0xC1);
writeLCDdata(0x10);

//vcm control
writeLCDcommand(0xC5);
writeLCDdata(0x3E);
writeLCDdata(0x28);

//vcm control 2
writeLCDcommand(0xC7);
writeLCDdata(0x86);

//memory access control
writeLCDcommand(0x36);
writeLCDdata(0x48);

//pixel format
writeLCDcommand(0x3A);
writeLCDdata(0x55);

//frameration control,normal mode full colours
writeLCDcommand(0xB1);
writeLCDdata(0x00);
writeLCDdata(0x18);

//display function control
writeLCDcommand(0xB6);
writeLCDdata(0x08);
writeLCDdata(0x82);
writeLCDdata(0x27);

//3gamma function disable
writeLCDcommand(0xF2);
writeLCDdata(0x00);

//gamma curve selected
writeLCDcommand(0x26);
writeLCDdata(0x01);

//set positive gamma correction
writeLCDcommand(0xE0);
writeLCDdata(0x0F);
writeLCDdata(0x31);
writeLCDdata(0x2B);
writeLCDdata(0x0C);
writeLCDdata(0x0E);
writeLCDdata(0x08);
writeLCDdata(0x4E);
writeLCDdata(0xF1);
writeLCDdata(0x37);
writeLCDdata(0x07);
writeLCDdata(0x10);
writeLCDdata(0x03);
writeLCDdata(0x0E);
writeLCDdata(0x09);
writeLCDdata(0x00);

//set negative gamma correction
writeLCDcommand(0xE1);
writeLCDdata(0x00);
writeLCDdata(0x0E);
writeLCDdata(0x14);
writeLCDdata(0x03);
writeLCDdata(0x11);
writeLCDdata(0x07);
writeLCDdata(0x31);
writeLCDdata(0xC1);
writeLCDdata(0x48);
writeLCDdata(0x08);
writeLCDdata(0x0F);
writeLCDdata(0x0C);
writeLCDdata(0x31);
writeLCDdata(0x36);
writeLCDdata(0x0F);

//exit sleep
writeLCDcommand(0x11);
delayms(120);
//display on
writeLCDcommand(0x29);


/*
	writeLCDcommand(ILI9341_CMD_POWER_ON_SEQ_CONTROL);
	writeLCDdata(ILI9341_CMD_IDLE_MODE_ON);
	writeLCDdata(ILI9341_CMD_MEMORY_WRITE);
	writeLCDdata(ILI9341_CMD_NOP);
	writeLCDdata(ILI9341_CMD_TEARING_EFFECT_LINE_OFF);
	writeLCDdata(0x02); 	// XXX

	writeLCDcommand(ILI9341_CMD_POWER_CONTROL_B);
	writeLCDdata(ILI9341_CMD_NOP);
	writeLCDdata(ILI9341_CMD_POWER_CONTROL_2);
	writeLCDdata(ILI9341_CMD_PARTIAL_AREA);

	writeLCDcommand(ILI9341_CMD_DRIVER_TIMING_CONTROL_A);
	writeLCDdata(0x85); 	// XXX
	writeLCDdata(ILI9341_CMD_NOP);
	writeLCDdata(0x78); 	// XXX

	writeLCDcommand(ILI9341_CMD_DRIVER_TIMING_CONTROL_B);
	writeLCDdata(ILI9341_CMD_NOP);
	writeLCDdata(ILI9341_CMD_NOP);

	writeLCDcommand(0xED);	// XXX
	writeLCDdata(0x64); 	// XXX
	writeLCDdata(0x03);	// XXX
	writeLCDdata(ILI9341_CMD_PARTIAL_MODE_ON);
	writeLCDdata(0X81); 	// XXX

	writeLCDcommand(ILI9341_CMD_PUMP_RATIO_CONTROL);
	writeLCDdata(ILI9341_CMD_DISP_INVERSION_OFF);

	writeLCDcommand(ILI9341_CMD_POWER_CONTROL_1);
	writeLCDdata(0x23);	//VRH[5:0] 	// XXX

	writeLCDcommand(ILI9341_CMD_POWER_CONTROL_2);
	writeLCDdata(ILI9341_CMD_ENTER_SLEEP_MODE);

	writeLCDcommand(ILI9341_CMD_VCOM_CONTROL_1);
	writeLCDdata(ILI9341_CMD_READ_MEMORY_CONTINUE);
	writeLCDdata(ILI9341_CMD_DISPLAY_OFF);

	writeLCDcommand(ILI9341_CMD_VCOM_CONTROL_2);
	writeLCDdata(0x86);	//--	// XXX

	writeLCDcommand(ILI9341_CMD_MEMORY_ACCESS_CONTROL);
	writeLCDdata(0x48);	//C8	//48 68gal.gal.gal.//28 E8 gal.gal.gal.	// XXX

	writeLCDcommand(ILI9341_CMD_COLMOD_PIXEL_FORMAT_SET);
	writeLCDdata(ILI9341_CMD_WRITE_CONTENT_ADAPT_BRIGHTNESS);

	writeLCDcommand(ILI9341_CMD_FRAME_RATE_CONTROL_NORMAL);
	writeLCDdata(ILI9341_CMD_NOP);
	writeLCDdata(0x18); 	// XXX

	writeLCDcommand(ILI9341_CMD_DISPLAY_FUNCTION_CONTROL);
	writeLCDdata(0x08); 	// XXX
	writeLCDdata(0x82);	// XXX
	writeLCDdata(0x27);	// XXX

	writeLCDcommand(ILI9341_CMD_ENABLE_3_GAMMA_CONTROL);
	writeLCDdata(ILI9341_CMD_NOP);

	writeLCDcommand(0x26);	//Gamma curve selected 	// XXX
	writeLCDdata(ILI9341_CMD_SOFTWARE_RESET);

	writeLCDcommand(ILI9341_CMD_POSITIVE_GAMMA_CORRECTION);
	writeLCDdata(0x0F); 	// XXX
	writeLCDdata(0x31);	// XXX
	writeLCDdata(ILI9341_CMD_PAGE_ADDRESS_SET);
	writeLCDdata(ILI9341_CMD_READ_DISP_PIXEL_FORMAT);
	writeLCDdata(ILI9341_CMD_READ_DISP_SIGNAL_MODE);
	writeLCDdata(0x08); 	// XXX
	writeLCDdata(0x4E); 	// XXX
	writeLCDdata(0xF1); 	// XXX
	writeLCDdata(ILI9341_CMD_VERT_SCROLL_START_ADDRESS);
	writeLCDdata(0x07); 	// XXX
	writeLCDdata(ILI9341_CMD_ENTER_SLEEP_MODE);
	writeLCDdata(0x03);	// XXX
	writeLCDdata(ILI9341_CMD_READ_DISP_SIGNAL_MODE);
	writeLCDdata(ILI9341_CMD_READ_DISP_STATUS);
	writeLCDdata(ILI9341_CMD_NOP);

	writeLCDcommand(ILI9341_CMD_NEGATIVE_GAMMA_CORRECTION);
	writeLCDdata(ILI9341_CMD_NOP);
	writeLCDdata(ILI9341_CMD_READ_DISP_SIGNAL_MODE);
	writeLCDdata(0x14); 	// XXX
	writeLCDdata(0x03);	// XXX
	writeLCDdata(ILI9341_CMD_SLEEP_OUT);
	writeLCDdata(0x07); 	// XXX
	writeLCDdata(0x31); 	// XXX
	writeLCDdata(ILI9341_CMD_POWER_CONTROL_2);
	writeLCDdata(0x48); 	// XXX
	writeLCDdata(0x08); 	// XXX
	writeLCDdata(0x0F); 	// XXX
	writeLCDdata(ILI9341_CMD_READ_DISP_PIXEL_FORMAT);
	writeLCDdata(0x31); 	// XXX
	writeLCDdata(ILI9341_CMD_MEMORY_ACCESS_CONTROL);
	writeLCDdata(ILI9341_CMD_READ_DISP_SELF_DIAGNOSTIC);

	writeLCDcommand(ILI9341_CMD_SLEEP_OUT);
	delayms(120);

	writeLCDcommand(ILI9341_CMD_DISPLAY_ON);
	writeLCDcommand(ILI9341_CMD_MEMORY_WRITE);
	//glcd_bg(bgcolor);

*/

}

void initializeLCD(void){
//#define QT020HLCG00 //nicer build quality
#define HT020SQV003NS //large metal body frame

//gpio_clear(BP_LCD_RESET_PORT,BP_LCD_RESET_PIN);
delayms(120);
//gpio_set(BP_LCD_RESET_PORT,BP_LCD_RESET_PIN);
delayms(120);

writeLCDcommand(0x11);//Sleep out, DC/DC converter, internal oscillator, panel scanning "enable"
delayms(120);

#ifdef QT020HLCG00
writeLCDcommand(0x35);//TEON (35h): Tearing Effect Line On
writeLCDdata(0x00);
#endif

#ifdef HT020SQV003NS
writeLCDcommand(0x36);  //MADCTL (36h): Memory Data Access Control
writeLCDdata(0b00000000); //0x00
#endif

writeLCDcommand(0x3A);//COLMOD,interface pixel format
#ifdef QT020HLCG00
writeLCDdata(0x05);
#elif defined(HT020SQV003NS)
writeLCDdata(0x55);
#endif

writeLCDcommand(0xB2); //porch setting,, default=0C/0C/00/33/33
#ifdef QT020HLCG00
writeLCDdata(0x5c); //back porch setting  0c
writeLCDdata(0x0c); //front porch setting 0c
writeLCDdata(0x00); //PSEN=0, disable seprate porch control
writeLCDdata(0x33); //back/front porch setting in idle mode
writeLCDdata(0x33); //back/front porch setting in partial mode
#elif defined(HT020SQV003NS)
writeLCDdata(0x05);
writeLCDdata(0x05);
writeLCDdata(0x00);
writeLCDdata(0x33);
writeLCDdata(0x33);
#endif

writeLCDcommand(0xB7); //Gate Control for VGH and VGL setting, default=35
writeLCDdata(0x75);  //0x62

#ifdef QT020HLCG00
writeLCDcommand(0xB3);//Frame Rate Control 1. mising from other, duplicated below
writeLCDdata(0x10);
writeLCDdata(0x05);
writeLCDdata(0x0E);
#endif

writeLCDcommand(0xBB); //VCOMS setting (0.1~1.675 V), default=20
#ifdef QT020HLCG00
writeLCDdata(0x12);
#elif defined(HT020SQV003NS)
writeLCDdata(0x22);
#endif

writeLCDcommand(0xC0); //LCM control, default=2C
#ifdef QT020HLCG00
writeLCDdata(0x2C); //XOR XBRG=36;XOR XMX=36 ;XOR inverse=21;
writeLCDdata(0xC2);
writeLCDdata(0x01);
#elif defined(HT020SQV003NS)
writeLCDdata(0x2C);
#endif

writeLCDcommand(0xC2); ///VDV and VRH command enable, default=01 or FF
writeLCDdata(0x01);

writeLCDcommand(0xC3); //VRH set (VRH=GVDD), default=0B
#ifdef QT020HLCG00
writeLCDdata(0x08);
#elif defined(HT020SQV003NS)
writeLCDdata(0x13);
#endif

writeLCDcommand(0xC4); //VDV set, default=20
writeLCDdata(0x20); //VDV=0v

writeLCDcommand(0xC6);//FRCTRL=Frame Rate Control in Normal Mode , default=0F
#ifdef QT020HLCG00
writeLCDdata(0x0F); //FR in normal mode=60Hz
#elif defined(HT020SQV003NS)
writeLCDdata(0x11);
#endif

writeLCDcommand(0xD0);//Power Control, default=A4/81
writeLCDdata(0xA4);//Constant
writeLCDdata(0xA1);//AVDD=6.8V;AVCL=-4.8V;VDDS=2.3V

#ifdef HT020SQV003NS
writeLCDcommand(0xD6);
writeLCDdata(0xA1);
#endif

writeLCDcommand(0xE0);//PVGAMCTRL:Positive Voltage Gamma Control
#ifdef QT020HLCG00
writeLCDdata(0xf0); //V63[7:4]=D(1101) & V0[3:0]=(0000),default=D/0
writeLCDdata(0x1B); //V1[5:0]=7(000111),default=0
writeLCDdata(0x1f); //V2[5:0]=E(001110),default=2
writeLCDdata(0x10); //V4[4:0]=10(10000),default=7
writeLCDdata(0x0a); //V6[4:0]=11(10001),default=B
writeLCDdata(0x15); //J0P[5:4]=2(10) ,default=1 & V13[3:0]=A(1010) ,default=A
writeLCDdata(0x3f); //V20[6:0]=36(0110110) ,default=31
writeLCDdata(0x44); //V36[6:4]=4(100) ,default=5 & V27[2:0]=4(100) ,default=9
writeLCDdata(0x51); //V43[6:0]=44(1000100) ,default=40
writeLCDdata(0x3a); //J1P[5:4]=2(10) ,default=2 & V50[3:0]=8(1000) ,default=9
writeLCDdata(0x15); //V57[4:0]=14(10100) ,default=12
writeLCDdata(0x14); //V59[4:0]=13(10011) ,default=12
writeLCDdata(0x2e); //V61[5:0]=14(010100),default=12
writeLCDdata(0x30); //V62[5:0]=17(010111) ,default=17
#elif defined(HT020SQV003NS)
writeLCDdata(0xD0);
writeLCDdata(0x05);
writeLCDdata(0x0A);
writeLCDdata(0x09);
writeLCDdata(0x08);
writeLCDdata(0x05);
writeLCDdata(0x2E);
writeLCDdata(0x44);
writeLCDdata(0x45);
writeLCDdata(0x0F);
writeLCDdata(0x17);
writeLCDdata(0x16);
writeLCDdata(0x2B);
writeLCDdata(0x33);
#endif

writeLCDcommand(0xE1);//NVGAMCTRL:Negative Voltage Gamma Control
#ifdef QT020HLCG00
writeLCDdata(0xf0); //V63[7:4]=D(1101) & V0[3:0]=(0000),default=D/0
writeLCDdata(0x1B); //V1[5:0]=1(000001),default=0
writeLCDdata(0x1f); //V2[5:0]=8(000100),default=2
writeLCDdata(0x10); //V4[4:0]=1(01010),default=7
writeLCDdata(0x0a); //V6[4:0]=1(01011),default=5
writeLCDdata(0x15); //J0N[5:4]=2(10),default=2 & V13[3:0]=(1000),default=5
writeLCDdata(0x3f); //V20[6:0]=32(0110010),default=2D
writeLCDdata(0x44); //V36[6:4]=4(100),default=4 & V27[2:0]=4(100),default=4
writeLCDdata(0x51); //V43[6:0]=46(1000110),default=44
writeLCDdata(0x3a); //J1N[5:4]=2(10),default=1 & V50[3:0]=C(1100),default=C
writeLCDdata(0x15); //V57[4:0]=18(11000),default=18
writeLCDdata(0x14); //V59[4:0]=17(10111),default=16
writeLCDdata(0x2e); //V61[5:0]=1A(011010),default=1C
writeLCDdata(0x30); //V62[5:0]=1D(011011),default=1D
#elif defined(HT020SQV003NS)
writeLCDdata(0xD0);
writeLCDdata(0x05);
writeLCDdata(0x0A);
writeLCDdata(0x09);
writeLCDdata(0x08);
writeLCDdata(0x05);
writeLCDdata(0x2E);
writeLCDdata(0x43);
writeLCDdata(0x45);
writeLCDdata(0x0F);
writeLCDdata(0x16);
writeLCDdata(0x16);
writeLCDdata(0x2B);
writeLCDdata(0x33);
#endif

#ifdef QT020HLCG00
writeLCDcommand(0xe4);//gate control for decrease consumption
writeLCDdata(0x1d);
#endif

writeLCDcommand(0x29);//Display ON ,default= Display OFF

writeLCDcommand(0x21);//Display inversion ON ,default=Display inversion OF

#ifdef QT020HLCG00 //already set above????
writeLCDcommand(0xB2); //porch setting,, default=0C/0C/00/33/33
writeLCDdata(0x7f); //back porch setting  0c
writeLCDdata(0x7f); //front porch setting 0c
writeLCDdata(0x01); //PSEN=0, disable separate porch control
writeLCDdata(0x33); //back/front porch setting in idle mode
writeLCDdata(0x33); //back/front porch setting in partial mode
#endif

#ifdef QT020HLCG00 //already set above????
writeLCDcommand(0xB3); //Frame Rate Control 1 (In partial mode/ idle colors)
writeLCDdata(0x11); //enable separate FR control
writeLCDdata(0x05);
writeLCDdata(0x0E);
#endif

#ifdef QT020HLCG00 //already set above????
writeLCDcommand(0xC6);//FRCTRL=Frame Rate Control in Normal Mode , default=0F
writeLCDdata(0x1f); //FR in normal mode=39Hz
#endif
}

