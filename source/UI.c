#include <stdlib.h>
#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/f1/bkp.h>
#include <libopencm3/cm3/scb.h>


#include "debug.h"
#include "cdcacm.h"
#include "buspirate.h"
#include "UI.h"
#include "protocols.h"
#include "AUXpin.h"
#include "ADC.h"
#include "delay.h"
#include "LA.h"
#include "fs.h"
#include "fpga.h"
#include "PSU.h"
#include "lcd.h"

#include <libopencm3/stm32/spi.h>

// globals
uint32_t cmdhead, cmdtail, cursor;		// TODO swap head an tail?
uint16_t bytecodePreprocessed, bytecodeProcessed, bytecodePostprocessed;
char cmdbuff[CMDBUFFSIZE];
struct _modeConfig modeConfig;

struct _bytecode bytecodes[256];

void serviceAsyncCommandFIFO();
void postProcess();
void processByteCode();

// global constants
const char vpumodes[][4] = {
"EXT\0",
"3V3\0",
"5V0\0"
};

const char bitorders[][4] ={
"MSB\0",
"LSB\0"
};

const char states[][4] ={
"OFF\0",
"ON\0"
};

const char displaymodes[][4] ={
"DEC\0",
"HEX\0",
"OCT\0",
"BIN\0"
};



// eats up the spaces and comma's from the cmdline
void consumewhitechars(void)
{
	while((cmdtail!=cmdhead)&&((cmdbuff[cmdtail]==' ')||(cmdbuff[cmdtail]==',')))
	{
		cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	}
}

// decodes value from the cmdline
// XXXXXX integer
// 0xXXXX hexadecimal
// 0bXXXX binair
uint32_t getint(void)
{
	int i;
	uint32_t number;

	i=0;
	number=0;

	if((cmdbuff[cmdtail]>=0x31)&&(cmdbuff[cmdtail]<=0x39))			// 1-9 decimal
	{
		number=cmdbuff[cmdtail]-0x30;
		i=1;

		while((cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]>=0x30)&&(cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]<=0x39))
		{
			number*=10;
			number+=cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]-0x30;
			i++;
		}
	}
	else if(cmdbuff[cmdtail]==0x30)
	{
		if((cmdbuff[((cmdtail+1)&(CMDBUFFSIZE-1))]>=0x30)&&(cmdbuff[((cmdtail+1)&(CMDBUFFSIZE-1))]<=0x39))		// 0-9 decimal
		{
			i=2;

			while((cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]>=0x30)&&(cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]<=0x39))
			{
				number*=10;
				number+=cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]-0x30;
				i++;
			}

		}
		else if((cmdbuff[((cmdtail+1)&(CMDBUFFSIZE-1))]=='x')||(cmdbuff[((cmdtail+1)&(CMDBUFFSIZE-1))]=='X'))		// 0x hexadecimal
		{
			i=2;

			while(((cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]>=0x30)&&(cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]<=0x39)) || \
				((cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]>='a')&&(cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]<='f')) || \
				((cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]>='A')&&(cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]<='F')))
			{
				number<<=4;
				if((cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]>=0x30)&&(cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]<=0x39))
				{
					number+=cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]-0x30;
				}
				else
				{
					cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]|=0x20;		// to lowercase
					number+=cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]-0x57;	// 0x61 ('a') -0xa
				}
				i++;
			}

		}
		else if((cmdbuff[((cmdtail+1)&(CMDBUFFSIZE-1))]=='b')||(cmdbuff[((cmdtail+1)&(CMDBUFFSIZE-1))]=='B'))		// 0b hexadecimal
		{
			i=2;

			while((cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]>=0x30)&&(cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]<=0x31))
			{
				number<<=1;
				number+=cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]-0x30;
				i++;
			}
		}
		else									// not perse wrong assume user entered 0
		{
			number=0;
			i=1;
			//	modeConfig.error=1;
		}
	}
	else
	{
		modeConfig.error=1;
	}

	// update cmdbuff pointers
	cmdtail=((cmdtail+(i-1))&(CMDBUFFSIZE-1));

	// tell the userinput (0 can be error!!)
	return number;
}

// initializes the variables
void initUI(void)
{
	int i;

	for(i=0; i<CMDBUFFSIZE; i++) cmdbuff[i]=0;
	cmdhead=0;
	cmdtail=0;

	modeConfig.wwr=0;
	modeConfig.numbits=8;
	modeConfig.oc=0;
	modeConfig.mode=0;
	modeConfig.pullups=0;
	modeConfig.vpumode=0;
	modeConfig.psu=0;
	modeConfig.bitorder=0;
	modeConfig.error=0;
	modeConfig.displaymode=0;
	modeConfig.pwm=0;
	modeConfig.init=0;

/*	modeConfig.mosiport=0;
	modeConfig.mosipin=0;
	modeConfig.misoport=0;
	modeConfig.misopin=0;
	modeConfig.csport=0;
	modeConfig.cspin=0;
	modeConfig.clkport=0;
	modeConfig.clkpin=0;
*/
	modeConfig.subprotocolname=0;
	setupLCD();
	initializeLCD();
	setBoundingBox(0, 240, 0, 320);


}

int isbuscmd(char c)
{
	switch(c)
	{
		case '[':	// start
		case ']':	// stop
		case '{':	// start
		case '}':	// stop
		case '/':	// clk h
		case '\\':	// clk l
		case '^':	// clk tick
		case '-':	// dat hi
		case '_':	// dat l
		case '.':	// dat s
		case '!':	// read bit
		case 'r':	// read
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':	// send value
		case '\"':	// send string
				return 1;
				break;
		default: 	return 0;
	}
}


void serviceAsyncCommandFIFO(){
    uint16_t fifo;

    if(gpio_get(BP_FPGA_FIFO_OUT_NEMPTY_PORT,BP_FPGA_FIFO_OUT_NEMPTY_PIN)){

        //read from the FIFO
        fifo=FPGA_REG_07;
        cdcprintf("FIFO: %04X\r\n",fifo);

        if(!(fifo&0x8000)){//first bit is 0, this is a peripheral read/write command
            cdcprintf("FPGA Peripheral write %d bits: %02X\r\n", (uint8_t)(fifo>>8),(uint8_t)fifo);
        }else{
            switch(fifo>>8){
            case 0xfe:
                cdcprintf("FPGA: Logic analyzer start\r\n");
                break;
            case 0xff:
                cdcprintf("FPGA: Logic analyzer stop\r\n");
                logicAnalyzerCaptureStop();
                break;
            case 0x81:
                cdcprintf("FPGA DIO write: %02X\r\n",(uint8_t)fifo);
                break;
            //case 0x82:
            //case 0x83:
            case 0x84:
                cdcprintf("FPGA delay: %02X\r\n", (uint8_t)fifo);
                break;
            default:
                cdcprintf("Unknown command %04X\r\n",fifo);
                break;

            }
        }
    }

}

// one big loop eating all user input. executed/interpretted it when the user presses enter
//
void doUI(void)
{
	int go;
	char c;

	uint32_t temp, temp2, temp3, repeat, received, bits;
	int i;
	uint8_t cancelPreprocessor=0;


	go=0;

	// wait for usb ready
//	while(!usbready());
	delayms(500);

	// show welcome
	//versioninfo();
	//cdcprintf("%s> ", protocols[modeConfig.mode].protocol_name);


	while(1)
	{
	    //modeConfig.mode=1;
		getuserinput();

		//flush the FIFO, need a better way to reset FPGA to known state...
		//while(gpio_get(BP_FPGA_FIFO_OUT_NEMPTY_PORT,BP_FPGA_FIFO_OUT_NEMPTY_PIN));
		    //temp=FPGA_REG_07;

		bytecodePreprocessed=0;
		bytecodeProcessed=0;
		bytecodePostprocessed=0;
		cancelPreprocessor=0;


		//cdcprintf2("cmd=%s\r\n", cmdbuff+cmdtail);
//		for(i=0; i<512; i++)
//		{
//			cdcputc2(((cmdbuff[i]>=0x20)&&(cmdbuff[i]<=0x7E))?cmdbuff[i]:'_');
//		}
//		cdcprintf2("\r\n");


		if(protocols[modeConfig.mode].protocol_periodic())
			go=2;
		else
			go=1;

		cdcprintf("\r\n");

		bytecodes[bytecodePreprocessed].command=0xFE; //start LA command...
		bytecodes[bytecodePreprocessed].blocking=0; //start LA command...
		bytecodePreprocessed=bytecodePreprocessed+1;

		while((go==1)&&(cmdtail!=cmdhead))
		{
			c=cmdbuff[cmdtail];

			// delayed init is handled here
			if((!modeConfig.init)&&(isbuscmd(c)))
			{
				if(modeConfig.mode!=0)
				{
					cdcprintf("running postphoned HWinit()\r\n");
					protocols[modeConfig.mode].protocol_setup_exc();
					modeConfig.init=1;
				}
			}


			switch (c)
			{
                case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
                    bytecodes[bytecodePreprocessed].command=0x00; //write to bus
                    bytecodes[bytecodePreprocessed].data=orderbits(getint());
                    bytecodes[bytecodePreprocessed].option1=getnumbits();
                    bytecodes[bytecodePreprocessed].repeat=getrepeat();
                    bytecodes[bytecodePreprocessed].blocking=0;
                    if(bytecodes[bytecodePreprocessed].option1<1) //custom user bits or use mode default
                        bytecodes[bytecodePreprocessed].option1=(uint16_t)modeConfig.numbits;
                    break;
                case '&':
				case '%':
                case 'r':
                    bytecodes[bytecodePreprocessed].command=c;
                    bytecodes[bytecodePreprocessed].option1=getnumbits();
                    bytecodes[bytecodePreprocessed].repeat=getrepeat();
                    bytecodes[bytecodePreprocessed].blocking=0;
                    break;
				case '[':
				case ']':
				case '{':
				case '}':
				case '/':
				case '\\':
				case '^':
				case '-':
				case '_':
				case '.':
				case '!':
                    bytecodes[bytecodePreprocessed].command=c;
                    bytecodes[bytecodePreprocessed].blocking=0;
				    break;
                case 'a':
                case '@':
                case 'A':
                    if(modeConfig.mode!=HIZ)
                    {
                        bytecodes[bytecodePreprocessed].command=c;
                        bytecodes[bytecodePreprocessed].blocking=0;
                    }
                    else
                    {
                        cdcprintf("Can't set AUX in HiZ mode!");
                        modeConfig.error=1;
                    }
				    break;
                case 'd':
                case 'D':
                case 'f':
                case 'g':
                case 'G':
                case 'h':
				case '?':
				case 'H':
				case 'i':
				case 'l':
				case 'L':
				//case 'm':
				//case 'o':
				case 'p':
				case 'P':
				case 'v':
				case 'w':
				case 'W':
                case '~':
				    bytecodes[bytecodePreprocessed].command=c;
				    bytecodes[bytecodePreprocessed].blocking=1;
				    break;
                /*case 'W':
                        temp=askint("value", 1, 0xFFFFFFFF, 1000);
						rcc_periph_clock_enable(RCC_DAC);
                        gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO4);
                        dac_disable(CHANNEL_1);
                        dac_buffer_disable(CHANNEL_1);
                        dac_disable_waveform_generation(CHANNEL_1);
                        dac_enable(CHANNEL_1);
                        dac_set_trigger_source(DAC_CR_TSEL1_SW);

                        dac_load_data_buffer_single(temp, RIGHT12, CHANNEL_1);
                        dac_software_trigger(CHANNEL_1);

                        //enable the VREG
                        gpio_set(BP_PSUEN_PORT, BP_PSUEN_PIN);
                        cancelPreprocessor=1;
                        break;
                case 'd':
                        temp = voltage(BP_USB_CHAN, 1);
                        //FPGA_REG_03&=~(0b1<<7);//release statemachine from reset
                        cdcprintf("USB: %d.%02dV\r\n", temp/1000, (temp%1000)/10);
                         temp = voltage(BP_VOUT_CHAN, 1);
                        //FPGA_REG_03&=~(0b1<<7);//release statemachine from reset
                        cdcprintf("VOUT: %d.%02dV\r\n", temp/1000, (temp%1000)/10);

                        FPGA_REG_0A=0b1110;
                        delayms(1);
                        temp = voltage(BP_ADC_CHAN, 1);
                        //FPGA_REG_03&=~(0b1<<7);//release statemachine from reset
                        cdcprintf("ADC: %d.%02dV\r\n", temp/1000, (temp%1000)/10);
                        FPGA_REG_0A=0b0000;
                        delayms(1);
                        temp = voltage(BP_ADC_CHAN, 1);
                        //FPGA_REG_03&=~(0b1<<7);//release statemachine from reset
                        cdcprintf("ADC: %d.%02dV\r\n", temp/1000, (temp%1000)/10);
                        FPGA_REG_0A=0b0010;
                        delayms(1);
                        temp = voltage(BP_ADC_CHAN, 1);
                        //FPGA_REG_03&=~(0b1<<7);//release statemachine from reset
                        cdcprintf("ADC: %d.%02dV\r\n", temp/1000, (temp%1000)/10);
						cancelPreprocessor=1;
                    break;*/
                case '=': //do bitwise and other math here.... =0x10|0x20, =0x10<<1, =0x10*8, etc
				case '|':
                    cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
                    bytecodes[bytecodePreprocessed].command=c;
                    bytecodes[bytecodePreprocessed].data=getint();
                    bytecodes[bytecodePreprocessed].blocking=1;
                    break;
				case '\"':
				    break; //HOW TO DEAL WITH STRING??? INDEX IN BIG ARRAY? NULL TERMINATED IN ARRAY?
                case '(':
                    cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);		// advance 1 position
                    temp=getint();
                    cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);		// advance 1
                    if(cmdbuff[cmdtail]==')')
                    {
                        bytecodes[bytecodePreprocessed].command=c;
                        bytecodes[bytecodePreprocessed].data=temp;
                        bytecodes[bytecodePreprocessed].blocking=1;
                    }
                    else
                    {
                        modeConfig.error=1;
                        cdcprintf("Error parsing macro");
                    }
                    break;
                case 'm':
                    changemode();
                    cancelPreprocessor=1;
                    break;
				case 'o':
                    changedisplaymode();
                    cancelPreprocessor=1;
                    break;
				case '$':
				    jumptobootloader();
				    cancelPreprocessor=1;
  					break;
				case '#':
				    reset();
				    cancelPreprocessor=1;
                    break;
				//case '~':
                        //logicAnalyzerCaptureStop();
                        //logicAnalyzerTest();
						//break;
				case 0x00:  break;
				case ' ':	break;
				case ',':	break;	// reuse this command??
				default:
				    cdcprintf("Unknown command: %c", c);
                    modeConfig.error=1;
                    //go=0;
                    //cmdtail=cmdhead-1;
                    break;
			}

			cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);	// advance to next char/command
			if((c!=' ')&&(c!=0x00)&&(c!=',')){
                //cdcprintf("\r\n");
                bytecodePreprocessed=bytecodePreprocessed+1;
			}

			if(modeConfig.error)			// something went wrong
			{
				cdcprintf("\x07");		// bell!
				go=0;
				cmdtail=cmdhead;
				modeConfig.error=0;
			}
		}

        if(!cancelPreprocessor&&bytecodePreprocessed>1){
            bytecodes[bytecodePreprocessed].command=0xFF; //stop LA command...
            bytecodes[bytecodePreprocessed].blocking=0;
            bytecodePreprocessed=bytecodePreprocessed+1;
            FPGA_REG_03|=(0b1<<7);//put statemachine in reset
            logicAnalyzerCaptureStart();
            while(bytecodePostprocessed<bytecodePreprocessed){
                processByteCode();
                postProcess();
            }
            logicAnalyzerCaptureStop();
        }
        cdcprintf("\r\n");


		if(modeConfig.subprotocolname)
			cdcprintf("%s-(%s)> ", protocols[modeConfig.mode].protocol_name, modeConfig.subprotocolname);
		else
			cdcprintf("%s> ", protocols[modeConfig.mode].protocol_name);
		if(go==2)
		{
			temp=0;
			while(((cmdtail+temp)&(CMDBUFFSIZE-1))!=cmdhead)
			{
				cdcputc(cmdbuff[((cmdtail+temp)&(CMDBUFFSIZE-1))]);
				temp++;
			}
		}
		go=0;
	}
}

void processByteCode(){

    uint16_t i;


    //process byte code until fgpa full or we hit a blocking command....
    while((bytecodeProcessed<bytecodePreprocessed) && !gpio_get(BP_FPGA_FIFO_IN_FULL_PORT,BP_FPGA_FIFO_IN_FULL_PIN))
    {
        switch (bytecodes[bytecodeProcessed].command)
        {
            case 0x00:
                    //bytecodes[bytecodePreprocessed].option1=bits;
                    //if (modeConfig.numbits<32) temp&=((1<<modeConfig.numbits)-1);
                    //while(bytecodes[bytecodeProcessed].repeat--)
                    //{//TODO: how to handle in lower layer, send custom bit numbers...
                        protocols[modeConfig.mode].protocol_send(bytecodes[bytecodeProcessed].data,bytecodes[bytecodeProcessed].repeat,bytecodes[bytecodeProcessed].option1);		// reshuffle bits if necessary
                    //}
                    break;
            case 'r':
                //while(bytecodes[bytecodeProcessed].repeat--)
                //{
                    protocols[modeConfig.mode].protocol_read();
                //}
                break;
            case '&':
                    FPGA_REG_07=(0x8400|(uint8_t)bytecodes[bytecodeProcessed].repeat);//delay for repeat cycles
                    //bytecodes[bytecodeProcessed].fpgaCommand=
                    break;
            case '%':
                    for(i=0;i<bytecodes[bytecodeProcessed].repeat;i++)
                        FPGA_REG_07=(0x8400|(uint8_t)0x0F);//delay for repeat cycles
                    break;
            case '\"': //NOT SURE HOW TO HANDLE THIS YET... ALSO THERE IS A BUG IF ONLY ONE CHAR "H" says unterminated
                   break;
            case '(':   protocols[modeConfig.mode].protocol_macro(bytecodes[bytecodeProcessed].data); //macro... should return if blocking or not??? hum
                    break;
            case '[':   modeConfig.wwr=0; //WWR should be handled on the backend processing!!!
                    protocols[modeConfig.mode].protocol_start();
                    break;
            case ']':   modeConfig.wwr=0;
                    protocols[modeConfig.mode].protocol_stop();
                    break;
            case '{':   modeConfig.wwr=1;
                    protocols[modeConfig.mode].protocol_startR();
                    break;
            case '}':   modeConfig.wwr=0;
                    protocols[modeConfig.mode].protocol_stopR();
                    break;
            case '/':   protocols[modeConfig.mode].protocol_clkh();
                    break;
            case '\\':  protocols[modeConfig.mode].protocol_clkl();
                    break;
            case '^':   protocols[modeConfig.mode].protocol_clk();
                    break;
            case '-':   protocols[modeConfig.mode].protocol_dath();
                    break;
            case '_':   protocols[modeConfig.mode].protocol_datl();
                    break;
            case '.':   protocols[modeConfig.mode].protocol_dats();
                    break;
            case '!':   protocols[modeConfig.mode].protocol_bitr();
                    break;
            case 'a':
                    //setAUX(0);
                    break;
            case 'A':
                    //setAUX(1);
                    break;
            case '@':
                    //cdcprintf("AUX=%d", getAUX());
                    break;
            case 'g':
                    //setPWM(0, 0);				// disable PWM
                    break;
            case 0xfe:
                    FPGA_REG_07=0xfe00;
                    break;
            case 0xff:
                    FPGA_REG_07=0xff00;
                    break;
                //below here are all blocking commands that wait for all previous commands to finish....
            default:
                    //ADD HALT STATEMENT TO COMMAND QUEUE!
                    if(bytecodes[bytecodeProcessed].blocking==1)
                        FPGA_REG_07=0xfd00;
                    break;
        }
        bytecodeProcessed=bytecodeProcessed+1;
    }

    FPGA_REG_03&=~(0b1<<7);//release statemachine from reset
}

void postProcess(){
    uint16_t temp, result;
    uint16_t i, temp2, temp3;

    if((bytecodeProcessed>bytecodePostprocessed) && gpio_get(BP_FPGA_FIFO_OUT_NEMPTY_PORT,BP_FPGA_FIFO_OUT_NEMPTY_PIN))
    {//process next byte

            /*result=FPGA_REG_07;

            if(bytecodes[bytecodePostprocessed].blocking && result!=0xfd00){
                cdcprintf("FPGA out of sync\r\n");
            }*/

            switch (bytecodes[bytecodePostprocessed].command)
			{
				case 0x00:
				    //todo: non-blocking wait for additional bytes?
                    protocols[modeConfig.mode].protocol_send_post(bytecodes[bytecodePostprocessed].data,bytecodes[bytecodePostprocessed].repeat,bytecodes[bytecodePostprocessed].option1);
                    break;
                case 'r':
                        if(result!=(0x0800|(((uint16_t)bytecodes[bytecodePostprocessed].option1<<8)|((uint16_t)bytecodes[bytecodePostprocessed].data&0x00FF)))){
                            cdcprintf("FPGA out of sync\r\n");
                        }
						//while(bytecodes[bytecodeIndex].repeat--)
						//{
							//protocols[modeConfig.mode].protocol_read();

                        cdcprintf("RX: ");
                        printnum((uint8_t)FPGA_REG_07);
                        cdcprintf("\r\n");
						//}
						break;
				case '\"': //NOT SURE HOW TO HANDLE THIS YET... ALSO THERE IS A BUG IF ONLY ONE CHAR "H" says unterminated
				       break;
				case '(':   //protocols[modeConfig.mode].protocol_macro(bytecodes[bytecodeIndex].data);
						break;
				case '[':   modeConfig.wwr=0; //WWR should be handled on the backend processing!!!
                        //protocols[modeConfig.mode].protocol_start_post(result);
						break;
				case ']':   modeConfig.wwr=0;
						//protocols[modeConfig.mode].protocol_stop_post(result);
						break;
				case '{':   modeConfig.wwr=1;
						//protocols[modeConfig.mode].protocol_startR_post(result);
						break;
				case '}':   modeConfig.wwr=0;
						//protocols[modeConfig.mode].protocol_stopR_post(result);
						break;
				case '/':   //protocols[modeConfig.mode].protocol_clkh_post(result);
						break;
				case '\\': // protocols[modeConfig.mode].protocol_clkl_post(result);
						break;
				case '^':  // protocols[modeConfig.mode].protocol_clk_post(result);
						break;
				case '-':  // protocols[modeConfig.mode].protocol_dath_post(result);
						break;
				case '_':  // protocols[modeConfig.mode].protocol_datl_post(result);
						break;
				case '.':  // protocols[modeConfig.mode].protocol_dats_post(result);
						break;
				case '!':  // protocols[modeConfig.mode].protocol_bitr_post(result);
						break;
				case '&':
                        //TODO: compare command to verify we are in the right place in the data....
                        if(FPGA_REG_07!=(0x8400|(uint8_t)bytecodes[bytecodeProcessed].repeat)){
                            cdcprintf("FPGA out of sync\r\n");
                        }
						cdcprintf("Delay: %dus", bytecodes[bytecodePostprocessed].repeat);
						break;
                case '%':
                        //TODO: compare command to verify we are in the right place in the data....
                        if(FPGA_REG_07!=(0x8400|0x0f)){
                            cdcprintf("FPGA out of sync\r\n");
                        }
                        cdcprintf("Delay: %dms", bytecodes[bytecodePostprocessed].repeat);
                        /*for(i=1;i<bytecodes[bytecodePostprocessed].repeat;i++)
                            temp=FPGA_REG_07;//delay for repeat cycled (CURRENTLY NOT SAFE!!!! NEED STATEMACHINE!!)*/
                    break;
				case 'a':
                        cdcprintf("Set AUX: 0");
						break;
				case 'A':
                        cdcprintf("Set AUX: 1");
						break;
				case '@':
                        cdcprintf("AUX=%d", getAUX());
						break;
                case 0xfe:
                        if(FPGA_REG_07!=0xfe00){
                            cdcprintf("FPGA out of sync\r\n");
                        }
                        cdcprintf("LA: start\r\n");
                        break;
                case 0xff:
                        if(FPGA_REG_07!=0xff00){
                            cdcprintf("FPGA out of sync\r\n");
                        }
                        cdcprintf("LA: stop\r\n");
                        break;
				case 'g':
                        setPWM(0, 0);				// disable PWM
						break;
                //below here are all blocking commands that wait for all previous commands to finish....
				case 'G':
                        if(modeConfig.mode!=HIZ)
						{
							temp=askint(PWMMENUPERIOD, 1, 0xFFFFFFFF, 1000);
							temp2=askint(PWMMENUOC, 1, 0xFFFFFFFF, 200);
							setPWM(temp, temp2);			// enable PWM
							if(modeConfig.pwm)
								cdcprintf("\r\nPWM on");
							else
								cdcprintf("\r\nPWM off");

						}
						else
						{
							cdcprintf("Can't use PWM in HiZ mode!");
						}
						break;
				case 'd':
                        //temp = voltage(BP_ADC_CHAN, 1);
                        //FPGA_REG_03&=~(0b1<<7);//release statemachine from reset
                        //cdcprintf("ADC: %d.%02dV\r\n", temp/1000, (temp%1000)/10);
                        temp = voltage(BP_VOUT_CHAN, 1);
                        cdcprintf("Vout: %d.%02dV\r\n", temp/1000, (temp%1000)/10);
						break;
				case 'D':
                        cdcprintf("Press any key to exit\r\n");
                        while(!cdcbyteready())
                        {
                            temp = voltage(BP_ADC_CHAN, 1);
                            cdcprintf("ADC: %d.%02dV\r", temp/1000, (temp%1000)/10);
                            delayms(250);
                        }
                        cdcgetc();
						break;
				case 'f':
                        cdcprintf("Freq: %ld Hz", getfreq());
						break;

				case 'h':
				case '?':
                        printhelp();
						break;
				case 'H':
                        protocols[modeConfig.mode].protocol_help();
						break;
				case 'i':
                        versioninfo();
						break;
				case 'l':
                        modeConfig.bitorder=0;
						cdcprintf("Bitorder: MSB");
						break;
				case 'L':
                        modeConfig.bitorder=1;
						cdcprintf("Bitorder: LSB");
						break;
				case 'p':
                        gpio_clear(BP_VPUEN_PORT, BP_VPUEN_PIN);	// always permitted
						cdcprintf("Pull-ups: disabled");
						modeConfig.pullups=0;
						break;
				case 'P':
                        if(modeConfig.mode!=0)		// reset vpu mode to externale??
						{

							gpio_set(BP_VPUEN_PORT, BP_VPUEN_PIN);
							cdcprintf("Pull-ups: enabled\r\n");
							delayms(10);
							temp = voltage(BP_VPU_CHAN, 1);
							cdcprintf("Vpu: %d.%02dV", temp/1000, (temp%1000)/10);
							modeConfig.pullups=1;
						}
						else
						{
							cdcprintf("Cannot enable pull-ups in HiZ");
							modeConfig.error=1;
						}
						break;
				case 'v':
                        showstates();
						break;
				case 'w':
                        psuDisable();
						cdcprintf("Vout: disabled");
						modeConfig.psu=0;
						break;
				case 'W':
                        if(modeConfig.mode!=0)
						{
                            psuSetOutput(0x0000);
							//delayms(10);
                            temp = voltage(BP_VOUT_CHAN, 1);
                            cdcprintf("Vout: %d.%02dV\r\n", temp/1000, (temp%1000)/10);
							/*if((voltage(BP_3V3_CHAN, 1)<3000))
							{
								cdcprintf("\r\nShort circuit!");
								gpio_clear(BP_PSUEN_PORT, BP_PSUEN_PIN);
							}
							else*/
								modeConfig.psu=1;
						}
						else
						{
							cdcprintf("Cannot enable PSU in HiZ!");
							modeConfig.error=1;
						}
						break;
				case '=': //TODO: add binary math
						temp2=modeConfig.displaymode;		// remember old displaymode
						temp3=modeConfig.numbits;		// remember old numbits
						modeConfig.numbits=32;
						for(i=0; i<4; i++)
						{
							cdcprintf("=");
							modeConfig.displaymode=i;
							printnum(bytecodes[bytecodePostprocessed].data);
						}
						modeConfig.numbits=temp3;
						break;
				case '|':
						temp2=modeConfig.displaymode;		// remember old displaymode
						modeConfig.bitorder^=1;
						temp3=modeConfig.numbits;		// remember old numbits
						modeConfig.numbits=32;
						for(i=0; i<4; i++)
						{
							cdcprintf("|");
							modeConfig.displaymode=i;
							printnum(orderbits(bytecodes[bytecodePostprocessed].data));
						}
						modeConfig.bitorder^=1;
						modeConfig.numbits=temp3;
						break;
				case '~':
                        //logicAnalyzerCaptureStop();
                        //logicAnalyzerTest();
						break;
				default:
                        cdcprintf("Unknown command: %c", bytecodes[bytecodePostprocessed].command);
						modeConfig.error=1;
						break;
			}
			if(bytecodes[bytecodePostprocessed].blocking) //TODO: there is a clock glitch on the logic analyzer clock when this happens...
            {
                result=FPGA_REG_07; //clear the blocking command...
    			FPGA_REG_03&=~(0b1<<7);//release statemachine from reset
			}
			bytecodePostprocessed=bytecodePostprocessed+1;


    }
}




// display teh versioninfo about the buspirate
// when not in HiZ mode it dumps info about the pins/voltags etc.
void versioninfo(void)
{
	int i;
	uint32_t *id = (uint32_t *)0x1FFFF7E8;
	uint16_t flashsize = *(uint16_t *) 0x1FFFF7E0;
	uint16_t ramsize=96;
//	uint32_t pwmperiod, pwmoc;

	// sram and flash size goed hand in hand
	if(flashsize<=16) ramsize=6;
	else if(flashsize<=32) ramsize=10;
	else if(flashsize<=128) ramsize=20;
	else if(flashsize<=512) ramsize=64;
	else ramsize=96;

	#ifndef FWVER
        #define FWVER 0
    #endif

	cdcprintf("Bus Pirate ULTRA %s\r\n", BP_PLATFORM);
	cdcprintf("Firmware %s (%s), bootloader N/A\r\n", FIRMWARE_VERSION, FWVER);
	cdcprintf("STM32 with %dK FLASH, %dK SRAM ", flashsize, ramsize);
	cdcprintf("s/n: %08X%08X%08X\r\n", id[0], id[1], id[2]);
	cdcprintf("https://dangerousprototypes.com/\r\n");

	cdcprintf("Available busprotocols:");

	for(i=0; i<MAXPROTO; i++)
	{
		cdcprintf(" %s", protocols[i].protocol_name);
	}
	cdcprintf("\r\n");

	protocols[modeConfig.mode].protocol_settings();
	cdcprintf("\r\n");

	if(modeConfig.mode!=HIZ)
	{
		cdcprintf("#bits: %d, ",modeConfig.numbits);
		cdcprintf("bitorder: %s, ", bitorders[modeConfig.bitorder]);
		cdcprintf("PU: %s, ", states[modeConfig.pullups]);
		cdcprintf("Vpu mode: %s, ", vpumodes[modeConfig.vpumode]);
		cdcprintf("Power: %s\r\n", states[modeConfig.psu]);
		cdcprintf("Displaymode: %s, ", displaymodes[modeConfig.displaymode]);
/*		cdcprintf("PWM: %s\r\n", states[modeConfig.pwm]);
		if(modeConfig.pwm)
		{
			pwmperiod=TIM_ARR(BP_PWM_TIMER);

// TODO is there a better way to do this?
#if(BP_PWM_CHANCHAN==1)
			pwmoc=TIM_CCR1(BP_PWM_TIMER);
#endif
#if(BP_PWM_CHANCHAN==2)
			pwmoc=TIM_CCR2(BP_PWM_TIMER);
#endif
#if(BP_PWM_CHANCHAN==3)
			pwmoc=TIM_CCR3(BP_PWM_TIMER);
#endif
#if(BP_PWM_CHANCHAN==4)
			pwmoc=TIM_CCR4(BP_PWM_TIMER);
#endif
			cdcprintf("PWM clock %d Hz, dutycycle %d.%02d\r\n", (36000000/pwmperiod), pwmoc == pwmperiod ? 1 : 0, ((pwmoc*100)/pwmperiod)%100);
		}
*/
		showstates();
	}
	if(modeConfig.init==0) cdcprintf("Pending HWinit()\r\n");
}

const char pinstates[][4] = {
"0\0",
"1\0",
"N/A\0"
};

const char pinmodes[][5] ={
"ANA.\0",		// analogue
"I-FL\0",		// input float
"I-UD\0",		// input pullup/down
"???\0",		// illegal
"O-PP\0",		// output pushpull
"O-OD\0",		// output opendrain
"O PP\0",		// output pushpull peripheral
"O OD\0",		// output opendrain peripheral
"----\0"		// pin is not used
};


uint8_t getpinmode(uint32_t port, uint16_t pin)
{
	uint32_t crl, crh;
	uint8_t pinmode, crpin, i;

	crl = GPIO_CRL(port);
	crh = GPIO_CRH(port);
	crpin=0;

	for(i=0; i<16; i++)
	{
		if((pin>>i)&0x0001)
		{
			crpin=(i<8?(crl>>(i*4)):(crh>>((i-8)*4)));
			crpin&=0x000f;
		}
	}

	pinmode=crpin>>2;

	if(crpin&0x03)		// >1 is output
	{
		pinmode+=4;
	}

	return pinmode;
}



// show voltages/pinstates
void showstates(void)
{
/*	uint8_t auxstate, csstate, misostate, clkstate, mosistate;
	uint8_t auxmode, csmode, misomode, clkmode, mosimode;
	uint16_t v50, v33, vpu, adc;

	cdcprintf("1.GND\t2.+5v\t3.+3V3\t4.Vpu\t5.ADC\t6.AUX\t7.CS\t8.MISO\t9.CLK\t10.MOSI\r\n");
	cdcprintf("GND\t+5v\t+3V3\tVpu\tADC\tAUX\t");
	protocols[modeConfig.mode].protocol_pins();
	cdcprintf("\r\n");

	// read pindirection
	auxmode=getpinmode(BP_AUX_PORT, BP_AUX_PIN);
	if(modeConfig.csport)
		csmode=getpinmode(modeConfig.csport, modeConfig.cspin);
	else
		csmode=8;

	if(modeConfig.misoport)
		misomode=getpinmode(modeConfig.misoport, modeConfig.misopin);
	else
		misomode=8;

	if(modeConfig.clkport)
		clkmode=getpinmode(modeConfig.clkport, modeConfig.clkpin);
	else
		clkmode=8;
	if(modeConfig.mosiport)
		mosimode=getpinmode(modeConfig.mosiport, modeConfig.mosipin);
	else
		mosimode=8;

	cdcprintf("PWR\tPWR\tPWR\tPWR\tAN\t%s\t%s\t%s\t%s\t%s\r\n", pinmodes[auxmode], pinmodes[csmode], pinmodes[misomode], pinmodes[clkmode], pinmodes[mosimode]);

	// pinstates
	auxstate=(gpio_get(BP_AUX_PORT, BP_AUX_PIN)?1:0);
	if(modeConfig.csport)
		csstate=(gpio_get(modeConfig.csport, modeConfig.cspin)?1:0);
	else
		csstate=2;

	if(modeConfig.misoport)
		misostate=(gpio_get(modeConfig.misoport, modeConfig.misopin)?1:0);
	else
		misostate=2;

	if(modeConfig.clkport)
		clkstate=(gpio_get(modeConfig.clkport, modeConfig.clkpin)?1:0);
	else
		clkstate=2;
	if(modeConfig.mosiport)
		mosistate=(gpio_get(modeConfig.mosiport, modeConfig.mosipin)?1:0);
	else
		mosistate=2;

	// adcs
	v33=voltage(BP_3V3_CHAN, 1);
	v50=voltage(BP_5V0_CHAN, 1);
	vpu=voltage(BP_VPU_CHAN, 1);  // TODO make difference between vextern (pin) and vpu (voltage used)??
	adc=voltage(BP_ADC_CHAN, 1);

	// show state of pin
	cdcprintf("GND\t%d.%02dV\t%d.%02dV\t%d.%02dV\t%d.%02dV\t%s\t%s\t%s\t%s\t%s\r\n", v50/1000, (v50%1000)/10, v33/1000, (v33%1000)/10, vpu/1000, (vpu%1000)/10, adc/1000, (adc%1000)/10, pinstates[auxstate], pinstates[csstate], pinstates[misostate], pinstates[clkstate], pinstates[mosistate]);

*/
}

// takes care of mode switchover
void changemode(void)
{
	uint32_t mode;
	int i;


	cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);	// pointer is set to 'm' we should advance 1
	consumewhitechars();			// eat whitechars
	mode=getint();

	if((mode>MAXPROTO))
	{
		cdcprintf("\r\nIllegal mode!\r\n");
		modeConfig.error=1;
	}

	while(modeConfig.error)			// no integer found
	{
		cdcprintf("\x07");
		modeConfig.error=0;

		for(i=0; i<MAXPROTO; i++)
			cdcprintf(" %d. %s\r\n", i+1, protocols[i].protocol_name);

		cdcprintf("Mode> ");
		cmdtail=cmdhead;	// flush all input
		getuserinput();
		consumewhitechars();
		mode=getint();

		if((mode>MAXPROTO)||(mode==0))
		{
			cdcprintf("\r\nIllegal mode!\r\n");
			modeConfig.error=1;
		}
	}

	protocols[modeConfig.mode].protocol_cleanup();		// switch to HiZ
	protocols[0].protocol_setup_exc();			// disables powersuppy etc.
	modeConfig.mode=mode-1;
	protocols[modeConfig.mode].protocol_setup();		// setup the new mode

	if(modeConfig.mode!=0)					// postphone the setup, this allows the user to setup the powersupply first
	{
		cdcprintf("Postphoning HWinit()\r\n");
	}

	if(modeConfig.mode==0) 	gpio_clear(BP_LED_MODE_PORT, BP_LED_MODE_PIN);
		else 	gpio_set(BP_LED_MODE_PORT, BP_LED_MODE_PIN);

}

// set display mode  (hex, bin, octa, dec)
void changedisplaymode(void)
{
	uint32_t displaymode;
	int i;


	cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);	// pointer is set to 'o' we should advance 1
	consumewhitechars();			// eat whitechars
	displaymode=getint();

	if((displaymode>4)||(displaymode==0))
	{
		cdcprintf("\r\nIllegal displaymode!\r\n");
		modeConfig.error=1;
	}

	while(modeConfig.error)			// no integer found
	{
		cdcprintf("\x07");
		modeConfig.error=0;

		for(i=0; i<4; i++)
			cdcprintf(" %d. %s\r\n", i+1, displaymodes[i]);

		cdcprintf("Displaymode>");
		cmdtail=cmdhead;	// flush all input
		getuserinput();
		consumewhitechars();
		displaymode=getint();

		if((displaymode>4)||(displaymode==0))
		{
			cdcprintf("\r\nIllegal displaymode!\r\n");
			modeConfig.error=1;
		}
	}

	modeConfig.displaymode=displaymode-1;
}

// displays the help
void printhelp(void)
{
	cdcprintf(" General\t\t\t\t\tProtocol interaction\r\n");
	cdcprintf(" ---------------------------------------------------------------------------\r\n");
	cdcprintf(" \t\t\t\t\t(0)\tList current macros\r\n");
	cdcprintf(" =X/|X\tConverts X/reverse X\t\t(x)\tMacro x\r\n");
	cdcprintf(" ~\tSelftest\t\t\t[\tStart\r\n");
	cdcprintf(" #\tReset the BP   \t\t\t]\tStop\r\n");
	cdcprintf(" $\tJump to bootloader\t\t{\tStart with read\r\n");
	cdcprintf(" &/%%\tDelay 1 us/ms\t\t\t}\tStop\r\n");
	cdcprintf(" a/A/@\tAUXPIN (low/HI/READ)\t\t\"abc\"\tSend string\r\n");
	cdcprintf(" b\tSet vpumode\t\t\t123\r\n");
	cdcprintf(" c/C\tAUX assignment (aux/CS)\t\t0x123\r\n");
	cdcprintf(" d/D\tMeasure ADC (once/CONT.)\t0b110\tSend value\r\n");
	cdcprintf(" f\tMeasure frequency\t\tr\tRead\r\n");
	cdcprintf(" g/S\tGenerate PWM/Servo\t\t/\tCLK hi\r\n");
	cdcprintf(" h/H/?\tHelp (general/PROTOCOL)\t\\\tCLK lo\r\n");
	cdcprintf(" i\tVersioninfo/statusinfo\t\t^\tCLK tick\r\n");
	cdcprintf(" l/L\tBitorder (msb/LSB)\t\t-\tDAT hi\r\n");
	cdcprintf(" m\tChange mode\t\t\t_\tDAT lo\r\n");
	cdcprintf(" o\tSet output type\t\t\t.\tDAT read\r\n");
	cdcprintf(" p/P\tPullup resistors (off/ON)\t!\tBit read\r\n");
	cdcprintf(" s\tScript engine\t\t\t:\tRepeat e.g. r:10\r\n");
	cdcprintf(" v\tShow volts/states\t\t.\tBits to read/write e.g. 0x55.2\r\n");
	cdcprintf(" w/W\tPSU (off/ON)\t\t<x>/<x= >/<0>\tUsermacro x/assign x/list all\r\n");
}

// copies an previous cmd to current position int cmdbuff
int cmdhistory(int ptr)
{
	int i;
	uint32_t temp;

	i=1;

	for (temp=(cmdtail-2)&(CMDBUFFSIZE-1); temp!=cmdhead; temp=(temp-1)&(CMDBUFFSIZE-1))
	{
		if(!cmdbuff[temp]) ptr--;

		if((ptr==0)&&(cmdbuff[(temp+1)&(CMDBUFFSIZE-1)]))		// do we want this one?
		{
			while(cursor!=((cmdhead)&(CMDBUFFSIZE-1)))			//clear line to end
			{
				cdcputc(' ');
				cursor=(cursor+1)&(CMDBUFFSIZE-1);
			}
			while(cursor!=((cmdtail)&(CMDBUFFSIZE-1)))			//move back to start;
			{
				cdcprintf("\x1B[D \x1B[D");
				cursor=(cursor-1)&(CMDBUFFSIZE-1);
			}


			while(cmdbuff[(temp+i)&(CMDBUFFSIZE-1)])
			{
				cmdbuff[(cmdtail+i-1)&(CMDBUFFSIZE-1)]=cmdbuff[(temp+i)&(CMDBUFFSIZE-1)];
				cdcputc(cmdbuff[(temp+i)&(CMDBUFFSIZE-1)]);
				i++;
			}
			cmdhead=(cmdtail+i-1)&(CMDBUFFSIZE-1);
			cursor=cmdhead;
			cmdbuff[cmdhead]=0x00;
			break;
		}
	}

	return (!ptr);
}


// handles the userinput
void getuserinput(void)
{
	int go, histptr;
	char c;
	uint32_t temp;

	go=0;
	cursor=cmdhead;
	histptr=0;

    //binmode stuff
    uint8_t page[256];

	while(!go)
	{


        if(cdcbyteready2())
		{

            int i=0;
            uint32_t addr=0x00000000;
            c=cdcgetc2();
            switch(c){
                case 0x20:
                    logicAnalyzerSetup();
                    break;
                case 0xc0:
                        cdcputc2(0x4B);
                        break;
                case 0x02: //write page

                    for(i=0;i<3;i++){
                        while(!cdcbyteready2());
                        addr=addr<<8;
                        addr|=cdcgetc2();
                    }
                    for(i=0;i<256;i++){
                        while(!cdcbyteready2());
                        page[i]=cdcgetc2();
                    }
                    writePage(addr,page);
                    //readFlash(addr,buff,256);
                    cdcputc2(0x4B);
                    break;
                case 0x03:
                    gpio_clear(BP_LED_MODE_PORT,BP_LED_MODE_PIN);
                    fpgainit();
                    cdcputs("FPGA update in progress...");
                    if(uploadfpga()==1){
                        gpio_set(BP_LED_MODE_PORT,BP_LED_MODE_PIN);
                       cdcputc2(0x4B);
                       //
                        logicAnalyzerSetup();
                        cdcputs("done.\n");
                    }else{
                        gpio_clear(BP_LED_MODE_PORT,BP_LED_MODE_PIN);
                        cdcputc2(0x00);
                        cdcputs("error.\n");
                    }

                    break;
            }//switch
		}//cdc





		//UI
		if(cdcbyteready())
		{
			c=cdcgetc();

			switch(c)
			{
				case 0x08:							// backspace
						if((cmdhead!=cmdtail)&&(cursor!=cmdtail))	// not empty or at beginning?
						{
							if(cursor==cmdhead)			// at end?
							{
								cmdhead=(cmdhead-1)&(CMDBUFFSIZE-1);
								cursor=cmdhead;
								cdcputs("\x08 \x08");
								cmdbuff[cmdhead]=0x00;
							}
							else
							{
								temp=cursor;
								cdcprintf("\x1B[D");
								while(temp!=cmdhead)
								{
									cmdbuff[((temp-1)&(CMDBUFFSIZE-1))]=cmdbuff[temp];
									cdcputc(cmdbuff[temp]);
									temp=(temp+1)&(CMDBUFFSIZE-1);
								}
								cdcputc(' ');
								cmdbuff[cmdhead]=0x00;
								cmdhead=(cmdhead-1)&(CMDBUFFSIZE-1);
								cursor=(cursor-1)&(CMDBUFFSIZE-1);
								cdcprintf("\x1B[%dD", ((cmdhead-cursor+1)&(CMDBUFFSIZE-1)));
							}
						}
						else cdcputc('\x07');
						break;
				case '\r':	cmdbuff[cmdhead]=0x00;
						cmdhead=(cmdhead+1)&(CMDBUFFSIZE-1);
						go=1;
						break;
				case '\x1B':	c=cdcgetc();
						if(c=='[')
						{
							c=cdcgetc();
							switch(c)
							{
								case 'D':	if(cursor!=cmdtail)	// left
										{
											cursor=(cursor-1)&(CMDBUFFSIZE-1);
											cdcprintf("\x1B[D");
										}
										else cdcputc('\x07');

										break;
								case 'C':	if(cursor!=cmdhead)	// right
										{
											cursor=(cursor+1)&(CMDBUFFSIZE-1);
											cdcprintf("\x1B[C");
										}
										else cdcputc('\x07');

										break;
								case 'A':	// up
										histptr++;
										if(!cmdhistory(histptr))	// on error restore ptr and ring a bell
										{
											histptr--;
											cdcputc('\x07');
										}
										break;
								case 'B':	// down
										histptr--;
										if((histptr<1)||(!cmdhistory(histptr)))
										{
											histptr=0;
											cdcputc('\x07');
										}
										break;
								default: 	break;
							}

						}
						break;
				default:	if((((cmdhead+1)&(CMDBUFFSIZE-1))!=cmdtail)&&(c>=0x20)&&(c<=0x7E))	// only accept printable characters if room available
						{
							if(cursor==cmdhead)		// at end
							{
								cdcputc(c);
								cmdbuff[cmdhead]=c;
								cmdhead=(cmdhead+1)&(CMDBUFFSIZE-1);
								cursor=cmdhead;
							}
							else
							{
								temp=cmdhead+1;
								while(temp!=cursor)
								{
									cmdbuff[temp]=cmdbuff[(temp-1)&(CMDBUFFSIZE-1)];
									temp=(temp-1)&(CMDBUFFSIZE-1);
								}
								cmdbuff[cursor]=c;
								temp=cursor;
								while(temp!=((cmdhead+1)&(CMDBUFFSIZE-1)))
								{
									cdcputc(cmdbuff[temp]);
									temp=(temp+1)&(CMDBUFFSIZE-1);
								}
								cursor=(cursor+1)&(CMDBUFFSIZE-1);
								cmdhead=(cmdhead+1)&(CMDBUFFSIZE-1);
								cdcprintf("\x1B[%dD", ((cmdhead-cursor)&(CMDBUFFSIZE-1)));
							}
						}
						else cdcputc('\x07');
						break;
			}
		}

//		if(cdcbyteready2())
//		{
//			SUMPlogicCommand(cdcgetc2());
//		}
		//SUMPlogicService();
	}
}


// keep the user asking the menu until it falls between minval and maxval, enter returns the default value
uint32_t askint(const char *menu, uint32_t minval, uint32_t maxval, uint32_t defval)
{
	uint32_t temp;
	uint8_t	done;

	done=0;

	while(!done)
	{
		cdcprintf(menu);

		cmdtail=cmdhead;	// flush all input
		getuserinput();
		consumewhitechars();
		temp=getint();

		if(temp==0)		// assume user pressed enter
		{
			temp=defval;
			done=1;
		}
		else if((temp>=minval)&&(temp<=maxval))
			done=1;
		else
		{
			cdcprintf("\x07");
			done=0;
		}
	}

	// clear errors
	modeConfig.error=0;

	return temp;
}

// get the repeat from the commandline (if any) XX:repeat
uint32_t getrepeat(void)
{
	uint32_t tail, repeat;

	repeat=1;				// do at least one round :D

	tail=(cmdtail+1)&(CMDBUFFSIZE-1);	// advance one
	if(tail!=cmdhead)			// did we reach the end?
	{
		if(cmdbuff[tail]==':')		// we have a repeat \o/
		{
			cmdtail=(cmdtail+2)&(CMDBUFFSIZE-1);
			repeat=getint();
		}
	}
	return repeat;
}

// get the number of bits from the commandline (if any) XXX.numbit
uint32_t getnumbits(void)
{
	uint32_t tail, numbits;

	numbits=0;

	tail=(cmdtail+1)&(CMDBUFFSIZE-1);	// advance one
	if(tail!=cmdhead)			// did we reach the end?
	{
		if(cmdbuff[tail]=='.')		// we have a change in bits \o/
		{
			cmdtail=(cmdtail+2)&(CMDBUFFSIZE-1);
			numbits=getint();
		}
	}
	return numbits;
}

// represent d in the current display mode. If numbits=8 also display the ascii representation
void printnum(uint32_t d)
{
	uint32_t mask, i;

	if (modeConfig.numbits<32) mask=((1<<modeConfig.numbits)-1);
	else mask=0xFFFFFFFF;
	d&=mask;

	switch(modeConfig.displaymode)
	{
		case 0:	if(modeConfig.numbits<=8)
				cdcprintf("%3u", d);
			else if(modeConfig.numbits<=16)
				cdcprintf("%5u", d);
			else if(modeConfig.numbits<=24)
				cdcprintf("%8u", d);
			else if(modeConfig.numbits<=32)
				cdcprintf("%10u", d);
			break;
		case 1:	if(modeConfig.numbits<=8)
				cdcprintf("0x%02X", d);
			else if(modeConfig.numbits<=16)
				cdcprintf("0x%04X", d);
			else if(modeConfig.numbits<=24)
				cdcprintf("0x%06X", d);
			else if(modeConfig.numbits<=32)
				cdcprintf("0x%08X", d);
			break;
		case 2:	if(modeConfig.numbits<=6)
				cdcprintf("0%02o", d);
			else if(modeConfig.numbits<=12)
				cdcprintf("0%04o", d);
			else if(modeConfig.numbits<=18)
				cdcprintf("0%06o", d);
			else if(modeConfig.numbits<=24)
				cdcprintf("0%08o", d);
			else if(modeConfig.numbits<=30)
				cdcprintf("0%010o", d);
			else if(modeConfig.numbits<=32)
				cdcprintf("0%012o", d);
			break;
		case 3:	cdcprintf("0b");
			for(i=0; i<modeConfig.numbits; i++)
			{
				mask=1<<(modeConfig.numbits-i-1);
				if(d&mask)
					cdcprintf("1");
				else
					cdcprintf("0");
			}
			break;
	}
	if(modeConfig.numbits!=8) cdcprintf(".%d", modeConfig.numbits);
		else cdcprintf(" (\'%c\')", ((d>=0x20)&&(d<0x7E)?d:0x20));
}


// order bits according to lsb/msb setting
uint32_t orderbits(uint32_t d)
{
	uint32_t result, mask;
	int i;

	if(!modeConfig.bitorder)			// 0=MSB
		return d;
	else
	{
		mask=0x80000000;
		result=0;

		for(i=0; i<32; i++)
		{
			if(d&mask)
			{
				result|=(1<<(i));
			}
			mask>>=1;
		}

		return result>>(32-modeConfig.numbits);
	}
}

#define BOOT_MAGIC_VALUE	0xB007

void jumptobootloader(void)
{
	rcc_periph_clock_enable(RCC_PWR);
	rcc_periph_clock_enable(RCC_BKP);
	pwr_disable_backup_domain_write_protect();
	BKP_DR1=BOOT_MAGIC_VALUE;
	pwr_enable_backup_domain_write_protect();
	rcc_periph_clock_disable(RCC_PWR);
	rcc_periph_clock_enable(RCC_BKP);

	SCB_AIRCR=0x0004|(0x5Fa<<16);
	while(1);
}

void reset(void)
{
	SCB_AIRCR=0x0004|(0x5Fa<<16);
	while(1);
}


void selftest(void)
{
	cdcprintf("Not implemented\r\n");
}
