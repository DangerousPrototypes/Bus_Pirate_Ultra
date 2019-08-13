

typedef struct {
char 		name [15];
uint8_t 	type;
uint32_t	addr;
uint32_t	size;
} file_struct;


void flashinit(void);
void showflashID(void);
void readflash (uint32_t addr, uint8_t *buff, uint16_t buffsize);
void printbuff(uint8_t *buff, uint16_t buffsize);
void writeflash(uint32_t addr, uint8_t *buff, int size);
void erasesector(uint32_t addr);
void formatflash(void);
void showdir(uint8_t type);
void addfile(char *name, uint8_t type, uint8_t *ptr, uint32_t size);
void chiperase(void);
file_struct *findfile(char *name, uint8_t type);
file_struct *findfileindex(uint8_t index);
int delfile(char *name, uint8_t type);

#define FLCMD_REMS	0x90		// Read Electronic Manufacturer ID & Device ID (REMS) 
#define FLCMD_RDID	0x9F		// Read Identification (RDID)  
#define FLCMD_READ	0x03		// Read Data Bytes (READ) 
#define FLCMD_FREAD	0x0B		// Fast Read Data Bytes (FREAD) 
#define FLCMD_RUID	0x4B		// Read Unique ID (RUID)
#define FLCMD_PE	0x81		// Page Erase (PE) 
#define FLCMD_WREN	0x06		// Write Enable (WREN) 
#define FLCMD_RDSR	0x05		// Read Status Register (RDSR) 
#define FLCMD_PP	0x02		// Page Program (PP) 
#define FLCMD_CE	0x60		// Chip Erase (CE) 




#define FILETYPE_DELETED	0x00
#define FILETYPE_BITSTREAM	0x01
#define FILETYPE_CONFIG		0x02
#define FILETYPE_UNUSED		0xFF

#define SUPERBLOCK_LOCATION	0x00000000l


/*

The filesystem is build on 256 bytes sectors, which can be erased individually on 
the flash chip we used. It takes advantage of the fact that a '1' can be flipped 
to '0' on a flash device. So unused partions are all '1's but when used set to
their value. Deletion of data is setting pointers to '0's. This will minimize the 
amount of erase cycles. will waste lots of space if lots of files are deleted.

TODO: some kind of compacting code

== SUPERBLOCK ==

The first sector stores 64 pointer to other sectors which holds a couple of 
directory entries.

special values:

0x00000000	entry is deleted
0xFFFFFFFF	entry is not used, but can be set to a new entry.

== DIRECTORY BLOCK ==

The directory block can hold upto 8 directory entries, which point to the actual
location in flash

typedef struct {
char 		name [15];
uint8_t 	type;
uint32_t	addr;
uint32_t	size;
} file_struct;

special values of filetype:

0x00	deleted
0xFF	unused entry

assuming: direntries are stored in order as the files stored in flash
*/

