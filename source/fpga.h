




void upload(void);
int uploadfpga(uint32_t addr, uint32_t size);
void fpgainit(void);


#define FPGA_BASE	0x68000000
#define FPGA_REG(x)	MMIO16(FPGA_BASE+(2*x))



