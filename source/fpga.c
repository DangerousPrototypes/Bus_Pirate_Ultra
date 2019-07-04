
#include <stdint.h>
#include "cdcacm.h"
#include "delay.h"
#include "fpga.h"

void progressbar(uint32_t count, uint32_t maxcount)
{
	uint32_t i;
	char bar[21];

	for(i=0; i<=((count*20)/maxcount); i++) bar[i]='#';
	for(i=((count*20)/maxcount)+1; i<21; i++) bar[i]='-';

	cdcprintf("[%s] %d/%d\r", bar, count, maxcount);


}

void upload(void)
{
	int i;

	cdcprintf("\r\nuploading\r\n");


	for(i=0; i<=1000; i++)
	{
		progressbar(i, 1000);
		delayms(10);
	}
}
