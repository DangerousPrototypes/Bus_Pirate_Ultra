
#include <stdlib.h>
#include <stdint.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>

#include "buspirate.h"
#include "delay.h"

void initdelay(void)
{
	rcc_periph_clock_enable(BP_DELAYTIMER_CLOCK);

	TIM_CNT(BP_DELAYTIMER) = 0;
	TIM_PSC(BP_DELAYTIMER) = 72;
	TIM_ARR(BP_DELAYTIMER) = 65535;
	TIM_CR1(BP_DELAYTIMER) |= TIM_CR1_CEN;
}

// delays num ms
void delayms(uint32_t num)
{
	while(num)
	{
		delayus(1000);
		num--;
	}
}


// delay num us
void delayus(uint32_t num)
{
	TIM_CNT(BP_DELAYTIMER)=0;
	while(TIM_CNT(BP_DELAYTIMER)<=num);

}

