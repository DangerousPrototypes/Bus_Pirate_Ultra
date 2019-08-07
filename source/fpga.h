




void upload(char c);
void progressbar(uint32_t count, uint32_t maxcount);
int uploadfpga(void);
void fpgainit(void);


#define FPGA_BASE	0x68000000


#define FPGA_REG_00	MMIO16(FPGA_BASE+0)
#define FPGA_REG_01	MMIO16(FPGA_BASE+2)
#define FPGA_REG_02	MMIO16(FPGA_BASE+4)
#define FPGA_REG_03	MMIO16(FPGA_BASE+6)
#define FPGA_REG_04	MMIO16(FPGA_BASE+8)
#define FPGA_REG_05	MMIO16(FPGA_BASE+10)
#define FPGA_REG_06	MMIO16(FPGA_BASE+12)
#define FPGA_REG_07	MMIO16(FPGA_BASE+14)
