
#include <stdint.h>
#include "buspirate.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "cdcacm.h"
#include "fs.h" 
#include "delay.h"
#include "string.h"
#include "UI.h"

char filetype[][4]={
"DEL",
"BIT",
"CFG"
};

void chiperase(void)
{
	uint8_t busy, status;

	busy=1;

	// write enable
	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_WREN);
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

	// erase chip
	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_CE);
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

	while(busy)
	{
		delayus(10);

		// check WIP bit 
		gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);
		spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_RDSR);
		status=(uint8_t) spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
		gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

		busy=(status&0x01);
	}


}

void erasesector(uint32_t addr)
{
	uint8_t busy, status;

	busy=1;
	addr&=0xFFFFFF00;		// page align

	// write enable
	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_WREN);
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

	// erase page
	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_PE);
	spi_xfer(BP_FS_SPI, (uint16_t) ((addr>>16)&0x000000FF));	// address
	spi_xfer(BP_FS_SPI, (uint16_t) ((addr>>8)&0x000000FF));		// 
	spi_xfer(BP_FS_SPI, (uint16_t) (addr&0x000000FF));		// 
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

	while(busy)
	{
		delayus(10);

		// check WIP bit 
		gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);
		spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_RDSR);
		status=(uint8_t) spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
		gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

		busy=(status&0x01);
	}
}


void writeflash(uint32_t addr, uint8_t *buff, int size) 
{
	uint8_t busy, status;
	int i;

	// write enable
	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_WREN);
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

	// page write
	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_PP);
	spi_xfer(BP_FS_SPI, (uint16_t) ((addr>>16)&0x000000FF));	// address
	spi_xfer(BP_FS_SPI, (uint16_t) ((addr>>8)&0x000000FF));		// 
	spi_xfer(BP_FS_SPI, (uint16_t) (addr&0x000000FF));		// 

	for(i=0; i<(size); i++)
	{
		spi_xfer(BP_FS_SPI, (uint16_t) buff[i]);
	}

	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

	busy=1;

	while(busy)
	{
		delayus(10);

		// check WIP bit 
		gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);
		spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_RDSR);
		status=(uint8_t) spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
		gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

		busy=(status&0x01);
	}
}

void flashinit(void)
{
	// enable peripheral
	rcc_periph_clock_enable(BP_FS_SPI_CLK);

	// SPI pins of FPGA
	gpio_set_mode(BP_FS_MOSI_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FS_MOSI_PIN);
	gpio_set_mode(BP_FS_CS_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_FS_CS_PIN);
	gpio_set_mode(BP_FS_CLK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_FS_CLK_PIN);
	gpio_set_mode(BP_FS_MISO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_FS_MISO_PIN);

	// setup SPI (cpol=1, cpha=1) 4Mhz
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);
	spi_reset(BP_FS_SPI);
	spi_init_master(BP_FS_SPI, SPI_CR1_BAUDRATE_FPCLK_DIV_128, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE, SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	spi_set_full_duplex_mode(BP_FS_SPI);
	spi_enable(BP_FS_SPI);

}

void showflashID(void)
{
	uint16_t manid, devid1, devid2;
	int i;

	cdcprintf("Flash info:\r\n");

	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);	// cs low
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_REMS);
	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);		// dummy
	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);		// dummy
	spi_xfer(BP_FS_SPI, (uint16_t) 0x00);		// 00= manid, devid 01= devid, manid
	manid=spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
	devid1=spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

	cdcprintf("REMS=%02X %02X\r\n", (uint8_t) manid, (uint8_t)devid1);

	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);	// cs low
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_RDID);
	manid=spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
	devid1=spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
	devid2=spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);

	cdcprintf("RDID=%02X %02X %02X\r\n", (uint8_t) manid, (uint8_t)devid1, (uint8_t)devid2);


	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);	// cs low
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_RUID);
	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);		// dummy
	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);		// dummy
	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);		// dummy
	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);		// dummy

	cdcprintf("RUID=");
	
	for(i=0; i<16; i++)
	{
		cdcprintf("%02X ", spi_xfer(BP_FS_SPI, (uint16_t) 0xFF));
	}

	cdcprintf("\r\n");

	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);
	
}

void readflash (uint32_t addr, uint8_t *buff, uint16_t buffsize)
{
	int i;

	gpio_clear(BP_FS_CS_PORT, BP_FS_CS_PIN);	// cs low
	spi_xfer(BP_FS_SPI, (uint16_t) FLCMD_READ);
	spi_xfer(BP_FS_SPI, (uint16_t) ((addr>>16)&0x000000FF));	// address
	spi_xfer(BP_FS_SPI, (uint16_t) ((addr>>8)&0x000000FF));		// 
	spi_xfer(BP_FS_SPI, (uint16_t) (addr&0x000000FF));		// 
//	spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);		// dummy

	for(i=0; i<buffsize; i++)
	{
		*(buff+i)=(uint8_t)spi_xfer(BP_FS_SPI, (uint16_t) 0xFF);
	}

	gpio_set(BP_FS_CS_PORT, BP_FS_CS_PIN);
}

#define printable(x)	((x>=0x20)&&(x<0x7F))

void printbuff(uint8_t *buff, uint16_t buffsize)
{
	uint32_t i, j;
	
	for(i=0; i<buffsize; i+=8)
	{
		cdcprintf("%08X: ", i);
		for(j=0; j<8; j++)
		{
			cdcprintf("%02X ", buff[i+j]);
		}
		for(j=0; j<8; j++)
		{
			cdcprintf("%c", (printable(buff[i+j])?buff[i+j]:'.')) ;
		}
		cdcprintf("\r\n");
	}
}


void formatflash(void)
{
	uint8_t buffer[256];
	uint32_t *superblock;
	int i;

	superblock=(uint32_t*)buffer;
	for(i=0; i<256; i++) buffer[i]=0xFF;


	// erase whole chip
	chiperase();


	superblock[0]=256;
	writeflash(SUPERBLOCK_LOCATION, buffer, 256);

}


void showdir(uint8_t type)
{
	uint8_t buffer1[256], buffer2[256];
	uint32_t *superblock, offset;
	file_struct *file;
	int i, j, num_files;

	i=0;
	num_files=0;
	offset=0;
	superblock=(uint32_t*)buffer1;
	file=(file_struct*)buffer2;

	// read superblock (sector 0)
	// contains a list of sectors with fileentries
	//
	// 0x00000000 = deleted
	// 0xFFFFFFFF = not allocated (yet)
	// other      = a list of fileentries

	readflash(SUPERBLOCK_LOCATION, buffer1, 256);

	// travel through the directory block
	while((superblock[i]!=0xFFFFFFFFl)&&(i<64))
	{
		if(superblock[i]==0x00000000)				// skip if directoryblock is deleted
		{
			i++;
			continue;
		}

		readflash(superblock[i], buffer2, 256);

		j=0;

		while((file[j].type!=0xFF)&&(j<8))
		{
			offset=(file[j].addr+file[j].size+256)&0xFFFFFF00;
			if((file[j].type==0x00)||((file[j].type!=type)&&type!=0x00))	// skip deleted files and other files 
			{
				j++;
				continue;
			}

			cdcprintf("%d: %s.%s stored @ %08X, len=%d\r\n", (i*8)+j, file[j].name, filetype[file[j].type], file[j].addr, file[j].size);
			num_files++;
			j++;
		}
		i++;
	}

	cdcprintf(" %d files found. %d bytes free\r\n", num_files, ((32*1024*1024)/8)-offset );
}

void addfile(char *name, uint8_t type, uint8_t *ptr, uint32_t size)
{
	uint8_t buffer1[256], buffer2[256];
	uint32_t *superblock, offset;
	file_struct *file;
	uint32_t i, j, k;

	superblock=(uint32_t*)buffer1;
	file=(file_struct*)buffer2;

	readflash(SUPERBLOCK_LOCATION, buffer1, 256);	// read superblock

	// find last directory block
	i=0;
	j=0;
	offset=512;

	while((superblock[i]!=0xFFFFFFFF)&&(i<64))	// read up to an empty spot
	{
		i++;
	}

	readflash(superblock[i-1], buffer2, 256);	// read last directory block

	while((file[j].type!=0xFF)&&(j<8))		// get latest directoryentry
	{
		offset=(file[j].addr+file[j].size+256)&0xFFFFFF00;	// we assume last directory entry (deleted or not) has the highest (address+size)
		j++;
	}

	cdcprintf("Offset=%08X i=%d j=%d\r\n", offset, i, j);

							// write directory entry first
	if(j==8)					// is there room to store this directory entry?
	{						// create new directory entry+update offset
		superblock[i]=offset;
		writeflash(SUPERBLOCK_LOCATION, buffer1, 256);
		offset+=256;
		j=0;
		i++;
		readflash(superblock[i-1], buffer2, 256);	// reread directory

	}

							// fill file directory structure
	for(k=0; k<15; k++) file[j].name[k]=name[k];
	file[j].type=type;
	file[j].addr=offset;
	file[j].size=size;

	writeflash(superblock[i-1], buffer2, 256);	// write directory entry
							// write file to flash
	for(i=0; i<((size+256)&0xFFFFFF00l); i+=256)
	{	
		writeflash(offset+i, ptr+i, 256);
		progressbar(i, ((size+256)&0xFFFFFF00));
	}
	progressbar(((size+256)&0xFFFFFF00), ((size+256)&0xFFFFFF00));
	cdcprintf("\r\n");

	
}

file_struct result;

file_struct *findfile(char *name, uint8_t type)
{
	int i, j, k, found;
	uint8_t buffer1[256], buffer2[256];
	uint32_t *superblock;
	file_struct *file;

	// clear result
	for(i=0; i<15; i++) result.name[i]=0x00;
	result.type=0x00;
	result.addr=0;
	result.size=0;


	i=0;
	found=0;
	superblock=(uint32_t*)buffer1;
	file=(file_struct*)buffer2;

	readflash(SUPERBLOCK_LOCATION, buffer1, 256);

	// travel through the directory block
	while((superblock[i]!=0xFFFFFFFFl)&&(i<64)&&(!found))
	{
		if(superblock[i]==0x00000000)				// skip if directoryblock is deleted
		{
			i++;
			continue;
		}

		readflash(superblock[i], buffer2, 256);

		j=0;

		while((file[j].type!=0xFF)&&(j<8)&&(!found))
		{
			if(file[j].type==0x00)				// skip deleted files
			{
				j++;
				continue;
			}
			if((strcmp(file[j].name, name)==0)&&(file[j].type==type))
			{
				for(k=0; k<15; k++) result.name[k]=file[j].name[k];
				result.type=file[j].type;
				result.addr=file[j].addr;
				result.size=file[j].size;
				found=1;
			}

			j++;
		}
		i++;
	}

	return &result;
}

file_struct *findfileindex(uint8_t index)
{
	uint8_t buffer1[256], buffer2[256];
	uint32_t *superblock;
	file_struct *file;
	int i;

	superblock=(uint32_t*)buffer1;
	file=(file_struct*)buffer2;

	// clear result
	for(i=0; i<15; i++) result.name[i]=0x00;
	result.type=0x00;
	result.addr=0;
	result.size=0;

	readflash(SUPERBLOCK_LOCATION, buffer1, 256);
	readflash(superblock[(index>>3)], buffer2, 256);

	for(i=0; i<15; i++) result.name[i]=file[(index&0x07)].name[i];
	result.type=file[(index&0x07)].type;
	result.addr=file[(index&0x07)].addr;
	result.size=file[(index&0x07)].size;

	return &result;
}


int delfile(char *name, uint8_t type)
{
	int i, j, found;
	uint8_t buffer1[256], buffer2[256];
	uint32_t *superblock;
	file_struct *file;

	// clear result
	for(i=0; i<15; i++) result.name[i]=0x00;
	result.type=0x00;
	result.addr=0;
	result.size=0;


	i=0;
	found=0;
	superblock=(uint32_t*)buffer1;
	file=(file_struct*)buffer2;

	readflash(SUPERBLOCK_LOCATION, buffer1, 256);

	// travel through the directory block
	while((superblock[i]!=0xFFFFFFFFl)&&(i<64)&&(!found))
	{
		if(superblock[i]==0x00000000)				// skip if directoryblock is deleted
		{
			i++;
			continue;
		}

		readflash(superblock[i], buffer2, 256);

		j=0;

		while((file[j].type!=0xFF)&&(j<8)&&(!found))
		{
			if(file[j].type==0x00)				// skip deleted files
			{
				j++;
				continue;
			}
			if((strcmp(file[j].name, name)==0)&&(file[j].type==type))
			{
				found=1;
				file[j].type=0x00;
				writeflash(superblock[i], buffer2, 256);	// write updated directory
				return 1;
			}

			j++;
		}
		i++;
	}
	return 0;
}

