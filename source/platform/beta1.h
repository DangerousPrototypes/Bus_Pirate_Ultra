

// development test platform
#define BP_PLATFORM		"BPULTRA v1.0 may 2019"
#define FIRMWARE_VERSION 	"v8"

// LEDs
#define BP_LED_MODE_PORT	GPIOA
#define BP_LED_MODE_PIN		GPIO15
#define BP_LED_USB_PORT		GPIOD
#define BP_LED_USB_PIN		GPIO3

// debug USART
#define BP_DEBUG_TX_PORT	GPIOB
#define BP_DEBUG_TX_PIN		GPIO6
#define BP_DEBUG_USART		USART1
#define BP_DEBUG_USART_CLK	RCC_USART1

// delay
#define BP_DELAYTIMER		TIM4
#define BP_DELAYTIMER_CLOCK	RCC_TIM4

// usb
#define BP_CONTROLS_USBPU
#define	BP_USB_PULLUP_PORT	GPIOE
#define BP_USB_PULLUP_PIN	GPIO4

// flash storage
#define BP_FS_SPI		    SPI3
#define BP_FS_SPI_CLK		RCC_SPI3
#define BP_FS_CLK_PIN		GPIO3
#define BP_FS_MOSI_PIN		GPIO5
#define BP_FS_MISO_PIN		GPIO4
#define BP_FS_CS_PIN		GPIO7		// misuse RX pin on debug header?
#define BP_FS_CLK_PORT		GPIOB
#define BP_FS_MOSI_PORT		GPIOB
#define BP_FS_MISO_PORT		GPIOB
#define BP_FS_CS_PORT		GPIOB

// FPGA
#define BP_FPGA_SPI		    SPI2
#define BP_FPGA_SPI_CLK		RCC_SPI2
#define BP_FPGA_CLK_PIN		GPIO13
#define BP_FPGA_MOSI_PIN	GPIO15
#define BP_FPGA_MISO_PIN	GPIO14
#define BP_FPGA_CS_PIN		GPIO12
#define BP_FPGA_CLK_PORT	GPIOB
#define BP_FPGA_MOSI_PORT	GPIOB
#define BP_FPGA_MISO_PORT	GPIOB
#define BP_FPGA_CS_PORT		GPIOB

#define BP_FPGA_CDONE_PIN	GPIO10
#define BP_FPGA_CRESET_PIN	GPIO11
#define BP_FPGA_CDONE_PORT	GPIOB
#define BP_FPGA_CRESET_PORT	GPIOB



/*
mcu_aux1	pc10
mcu_aux2	pc11
mcu_aux3	pc12
mcu_aux4	pd6
mcu_aux5	pb8
mcu_aux6	pb9
mcu_aux7	pe0
mcu_aux8	pe1
mcu_aux9	pf13
mcu_aux10	pb10
mcu_aux11	pc5
mcu_aux12	pa6
mcu_aux		pc5
mcu_int0	pe2
mcu_int1	pe3
*/


// FSMC
#define BP_FSMC_A0_PORT		GPIOF
#define BP_FSMC_A0_PIN		GPIO0
#define BP_FSMC_A1_PORT		GPIOF
#define BP_FSMC_A1_PIN		GPIO1
#define BP_FSMC_A2_PORT		GPIOF
#define BP_FSMC_A2_PIN		GPIO2
#define BP_FSMC_A3_PORT		GPIOF
#define BP_FSMC_A3_PIN		GPIO3
#define BP_FSMC_A4_PORT		GPIOF
#define BP_FSMC_A4_PIN		GPIO4
#define BP_FSMC_A5_PORT		GPIOF
#define BP_FSMC_A5_PIN		GPIO5

#define BP_FSMC_D0_PORT		GPIOD
#define BP_FSMC_D0_PIN		GPIO14
#define BP_FSMC_D1_PORT		GPIOD
#define BP_FSMC_D1_PIN		GPIO14
#define BP_FSMC_D2_PORT		GPIOD
#define BP_FSMC_D2_PIN		GPIO0
#define BP_FSMC_D3_PORT		GPIOD
#define BP_FSMC_D3_PIN		GPIO1

#define BP_FSMC_D4_PORT		GPIOE
#define BP_FSMC_D4_PIN		GPIO7
#define BP_FSMC_D5_PORT		GPIOE
#define BP_FSMC_D5_PIN		GPIO8
#define BP_FSMC_D6_PORT		GPIOE
#define BP_FSMC_D6_PIN		GPIO9
#define BP_FSMC_D7_PORT		GPIOE
#define BP_FSMC_D7_PIN		GPIO10

#define BP_FSMC_D8_PORT		GPIOE
#define BP_FSMC_D8_PIN		GPIO11
#define BP_FSMC_D9_PORT		GPIOE
#define BP_FSMC_D9_PIN		GPIO12
#define BP_FSMC_D10_PORT	GPIOE
#define BP_FSMC_D10_PIN		GPIO13
#define BP_FSMC_D11_PORT	GPIOE
#define BP_FSMC_D11_PIN		GPIO14

#define BP_FSMC_D12_PORT	GPIOE
#define BP_FSMC_D12_PIN		GPIO15
#define BP_FSMC_D13_PORT	GPIOD
#define BP_FSMC_D13_PIN		GPIO8
#define BP_FSMC_D14_PORT	GPIOD
#define BP_FSMC_D14_PIN		GPIO9
#define BP_FSMC_D15_PORT	GPIOD
#define BP_FSMC_D15_PIN		GPIO10

#define BP_FSMC_NOE_PORT	GPIOD
#define BP_FSMC_NOE_PIN		GPIO4
#define BP_FSMC_NWE_PORT	GPIOD
#define BP_FSMC_NWE_PIN		GPIO5
#define BP_FSMC_NCE_PORT	GPIOG
#define BP_FSMC_NCE_PIN		GPIO10




// pullups and poewrsupplies
#define BP_VPU50EN_PORT		GPIOG
#define BP_VPU50EN_PIN		GPIO1
#define BP_VPU33EN_PORT		GPIOG
#define BP_VPU33EN_PIN		GPIO0
#define BP_VPUEN_PORT		GPIOF
#define BP_VPUEN_PIN		GPIO15
#define BP_PSUEN_PORT		GPIOF
#define BP_PSUEN_PIN		GPIO14

// adc
#define BP_ADC			ADC1
#define BP_ADC_CLK		RCC_ADC1
#define BP_ADC_CHAN		9
#define BP_VPU_CHAN		7	// 7= on pint Vext 14= before 4066
#define BP_3V3_CHAN		5
#define BP_5V0_CHAN		4
#define BP_USB_CHAN		3

#define	BP_ADC_PORT		GPIOB
#define	BP_ADC_PIN		GPIO1
#define BP_3V3_PORT		GPIOA
#define BP_3V3_PIN		GPIO5
#define BP_5V0_PORT		GPIOA
#define BP_5V0_PIN		GPIO4
#define BP_VPU_PORT		GPIOA
#define BP_VPU_PIN		GPIO7
#define BP_VSUP_PORT		GPIOA
#define BP_VSUP_PIN		GPIO3








