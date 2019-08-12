#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/spi.h>
#include "debug.h"
#include "cdcacm.h"
#include "buspirate.h"
#include "delay.h"
#include "fpga.h"
#include "fs.h"
#include "LA.h"
#include "UI.h"
#include "ADC.h"



//globals
uint32_t usbflushtime;				// usb poll timer
volatile uint32_t systicks;

// systick timer
// handles the USB polling
void sys_tick_handler(void)
{
	usbflushtime++;
	systicks++;

	// check usb for new data
	if (usbflushtime==50)
	{
		cdcflush();
		usbflushtime=0;
	}
}

// all the fun starts here
int main(void)
{
	char c;
	int i;
	uint8_t buff[256];

    unsigned char temp;

	// init vars
	usbflushtime=0;
//#define LADEBUG
	// init clock
	rcc_clock_setup_in_hse_8mhz_out_72mhz();
//	rcc_clock_setup_in_hsi_out_48mhz();

	// enable clocks for IO and alternate functions
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOD);
	rcc_periph_clock_enable(RCC_GPIOE);
	rcc_periph_clock_enable(RCC_GPIOF);
	rcc_periph_clock_enable(RCC_GPIOG);
	rcc_periph_clock_enable(RCC_AFIO);

    /*
    la_sram_mode_setup();
    la_sram_mode_spi();
    la_sram_quad_setup();
    la_sram_quad_output();
    la_sram_arm_setup();
    la_sram_arm_stop();
    */

	AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON;		// disable jtag/enable swd

	// setup pins (move to a seperate function??)
#ifdef BP_CONTROLS_USBPU
	gpio_set_mode(BP_USB_PULLUP_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_USB_PULLUP_PIN);	// USB d+ pullup
	gpio_clear(BP_USB_PULLUP_PORT, BP_USB_PULLUP_PIN);							// pull down
#endif


	// setup systick
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);	// 9000000 Hz
	systick_set_reload(89);					// 10us 89
	systick_interrupt_enable();
	systick_counter_enable();				// go!
	systicks=0;

	// init the delay timer
	initdelay();

#ifdef BP_CONTROLS_USBPU
	// enable USB pullup

	delayms(100);
	gpio_set(BP_USB_PULLUP_PORT, BP_USB_PULLUP_PIN);
#else
	//toggle the usb pullup
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);
	gpio_clear(GPIOA, GPIO12);
	delayms(100);
#endif

	// setup USB
	cdcinit();

	// setupflash
	flashinit();

	// init fpga
	fpgainit();

	// init UI
	initUI();

	// LEDs
	gpio_set_mode(BP_LED_MODE_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LED_MODE_PIN);
	gpio_clear(BP_LED_MODE_PORT, BP_LED_MODE_PIN);
	gpio_set_mode(BP_LED_USB_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LED_USB_PIN);
	gpio_clear(BP_LED_USB_PORT, BP_LED_USB_PIN);

	//control lines
	gpio_clear(BP_PSUEN_PORT, BP_PSUEN_PIN);								// active hi
	gpio_set_mode(BP_PSUEN_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_PSUEN_PIN);		// PSU disable

	gpio_clear(BP_VPUEN_PORT, BP_VPUEN_PIN);								// active hi
	gpio_set_mode(BP_VPUEN_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_VPUEN_PIN);		// VPU disable

	gpio_clear(BP_VPU50EN_PORT, BP_VPU50EN_PIN);								// active low
	gpio_set_mode(BP_VPU50EN_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_VPU50EN_PIN);	// VPU3v3 disable

	gpio_clear(BP_VPU33EN_PORT, BP_VPU33EN_PIN);								// active low
	gpio_set_mode(BP_VPU33EN_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_VPU33EN_PIN);	// VPU5v0 disable


	// ADC
	initADC();

    if(uploadfpga()==1){
       gpio_set(BP_LED_MODE_PORT,BP_LED_MODE_PIN);
    }else{
        gpio_clear(BP_LED_MODE_PORT,BP_LED_MODE_PIN);
    }

    logicAnalyzerSetup();

	while (1)
	{
		doUI();


	}//while
}













