

void flashinit(void);
void showFlashID(void);
void readFlash (uint32_t addr, uint8_t *buff, uint16_t buffsize);
void printbuff(uint8_t *buff, uint16_t buffsize);
void writePage(uint32_t addr, uint8_t *page);
void writeFlash(uint32_t addr, uint8_t *buff, uint8_t size);
void eraseSector(uint32_t addr);
