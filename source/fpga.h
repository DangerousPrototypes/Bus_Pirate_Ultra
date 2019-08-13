




int uploadfpga(uint32_t addr, uint32_t size);
void fpgainit(void);
int bitstreaminfo(uint32_t addr, uint32_t size);

void writefpga(uint16_t *data, int size);
void readfpga(uint16_t *data, int size);

#define FPGA_BASE	0x68000000
#define FPGA_REG(x)	MMIO16(FPGA_BASE+(2*x))



