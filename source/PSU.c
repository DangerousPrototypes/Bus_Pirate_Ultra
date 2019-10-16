#include <stdint.h>
#include "buspirate.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dac.h>


void psuSetOutput(uint16_t value){
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO4); //todo:should be analog input according to the datasheet
    dac_enable(CHANNEL_1);
    dac_load_data_buffer_single(value, RIGHT12, CHANNEL_1);
    dac_software_trigger(CHANNEL_1);
    //enable the VREG
    gpio_set(BP_PSUEN_PORT, BP_PSUEN_PIN);
}

void psuDisable(){
    gpio_clear(BP_PSUEN_PORT, BP_PSUEN_PIN); //turn off MCP1824
    dac_disable(CHANNEL_1); //turn off DAC
    //gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO4); //DAC pin to output and ground
    //gpio_clear(GPIOA,GPIO4);

}

void psuinit(void){
    // programmable output voltage supply control
	gpio_clear(BP_PSUEN_PORT, BP_PSUEN_PIN);								// active hi
	gpio_set_mode(BP_PSUEN_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_PSUEN_PIN);		// PSU disable

    // setup DAC
    rcc_periph_clock_enable(RCC_DAC);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO4); //todo:should be analog input according to the datasheet
    dac_disable(CHANNEL_1);
    dac_buffer_disable(CHANNEL_1);
    dac_disable_waveform_generation(CHANNEL_1);
    dac_enable(CHANNEL_1);
    dac_set_trigger_source(DAC_CR_TSEL1_SW);
}
