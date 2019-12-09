#include <stdint.h>
#include "buspirate.h"
#include "UI.h"
#include "lcd.h"
#include "PSU.h"
#include "delay.h"
#include "ADC.h"
#include "cdcacm.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dac.h>


void psuSetOutput(uint16_t value){
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO4); //todo:should be analog input according to the datasheet
    dac_enable(CHANNEL_1);
    dac_load_data_buffer_single(value, RIGHT12, CHANNEL_1);
    dac_software_trigger(CHANNEL_1);
    //enable the VREG
    gpio_set(VREGEN_PORT, VREGEN_PIN);
    modeConfig.psu=1;
    writePSUstate();
}

void psuDisable(){
    gpio_clear(VREGEN_PORT, VREGEN_PIN); //turn off MCP1824
    dac_disable(CHANNEL_1); //turn off DAC
    //gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO4); //DAC pin to output and ground
    //gpio_clear(GPIOA,GPIO4);

    modeConfig.psu=0;
    writePSUstate();

}

void psutest(void){
    int i;
    uint16_t temp;
//DAC_ADJ_CURRENT to full
//setup spi pins
	gpio_clear(DAC_CLOCK_PORT, DAC_CLOCK_PIN);
	gpio_set_mode(DAC_CLOCK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, DAC_CLOCK_PIN);
    gpio_clear(DAC_DATA_PORT, DAC_DATA_PIN);
	gpio_set_mode(DAC_DATA_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, DAC_DATA_PIN);
	gpio_clear(DAC_SYNC_PORT, DAC_SYNC_PIN);
	gpio_set_mode(DAC_SYNC_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, DAC_SYNC_PIN);
    //bitbang 0xfff to DAC
	psuwritedac(0,0b0111111111);
	psuwritedac(1,0b1111111111);

//setup MCU_CURRENT_INT1 and INT0 as inputs to see the results
	gpio_set_mode(CURRENT_INT0_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, CURRENT_INT0_PIN);
	gpio_set_mode(CURRENT_INT1_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,CURRENT_INT1_PIN);
//setup MCU_CURRENT_TEST and set to ground
	gpio_clear(CURRENT_TEST_PORT, CURRENT_TEST_PIN);
	gpio_set_mode(CURRENT_TEST_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, CURRENT_TEST_PIN);
//setup MCU_CURRENT_RESET as open drain. ground it for a second and release
    gpio_set_mode(CURRENT_RESET_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, CURRENT_RESET_PIN);
    gpio_set(CURRENT_RESET_PORT, CURRENT_RESET_PIN);


    gpio_clear(CURRENT_RESET_PORT, CURRENT_RESET_PIN);
    delayms(1);
    gpio_set(CURRENT_RESET_PORT, CURRENT_RESET_PIN);
    delayms(1);


//MCU_VREGEN setup and to high
	gpio_set_mode(VREGEN_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, VREGEN_PIN);
    gpio_set(VREGEN_PORT, VREGEN_PIN);
//bitbang 0x100 to DAC_ADJ_VOLTAGE
//setup measure MCU_ADC0 before back current protection
//setup measure MCU_ADC2 after back current protection
    gpio_set_mode(BP_VOUT_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, BP_VOUT_PIN);
    delayms(1);
    temp = voltage(BP_VOUT_CHAN, 1);
    cdcprintf("No Load Vout: %d.%02dV\r\n", temp/1000, (temp%1000)/10);

    psuwritedac(1,0x0100); //0x33 trip point...
    delayms(1);
    temp = voltage(BP_VOUT_CHAN, 1);
    cdcprintf("Decrease limit Vout: %d.%02dV\r\n", temp/1000, (temp%1000)/10);

    //enable load
    gpio_set(CURRENT_TEST_PORT, CURRENT_TEST_PIN);

    delayms(1);
    temp = voltage(BP_VOUT_CHAN, 1);
    cdcprintf("Load enabled Vout: %d.%02dV\r\n", temp/1000, (temp%1000)/10);

    gpio_clear(CURRENT_TEST_PORT, CURRENT_TEST_PIN);

}

void psuwritedac(uint16_t output,uint16_t value){

    uint16_t i,command=0;

    command=(output<<14)|(0b01<<12);
    value=value<<2;
    command|=value;

    gpio_set(DAC_SYNC_PORT, DAC_SYNC_PIN);
    gpio_clear(DAC_CLOCK_PORT, DAC_CLOCK_PIN);
    gpio_clear(DAC_DATA_PORT, DAC_DATA_PIN);
    delayms(1);
    gpio_clear(DAC_SYNC_PORT, DAC_SYNC_PIN);

    for(i=0;i<16;i++){
        if(command&0x8000){
           gpio_set(DAC_DATA_PORT, DAC_DATA_PIN);
        }else{
            gpio_clear(DAC_DATA_PORT, DAC_DATA_PIN);
        }
        gpio_set(DAC_CLOCK_PORT, DAC_CLOCK_PIN);
        delayms(1);
        gpio_clear(DAC_CLOCK_PORT, DAC_CLOCK_PIN);
        command=command<<1;
    }

    gpio_clear(DAC_CLOCK_PORT, DAC_CLOCK_PIN);
    gpio_clear(DAC_DATA_PORT, DAC_DATA_PIN);
	gpio_clear(DAC_SYNC_PORT, DAC_SYNC_PIN);


}

void psuinit(void){
    // programmable output voltage supply control
	gpio_clear(VREGEN_PORT, VREGEN_PIN);								// active hi
	gpio_set_mode(VREGEN_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, VREGEN_PIN);		// PSU disable

    // setup DAC
    rcc_periph_clock_enable(RCC_DAC);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO4); //todo:should be analog input according to the datasheet
    dac_disable(CHANNEL_1);
    dac_buffer_disable(CHANNEL_1);
    dac_disable_waveform_generation(CHANNEL_1);
    dac_enable(CHANNEL_1);
    dac_set_trigger_source(DAC_CR_TSEL1_SW);

    modeConfig.psu=0;
}
