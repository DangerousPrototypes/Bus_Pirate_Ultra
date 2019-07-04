

// development test platform
#define BP_PLATFORM		"BPULTRA v1.0 may 2019"

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
#define BP_FS_SPI		SPI3
#define BP_FS_SPI_CLK		RCC_SPI3
#define BP_FS_PORT		GPIOB
#define BP_FS_CLK_PIN		GPIO3
#define BP_FS_MOSI_PIN		GPIO5
#define BP_FS_MISO_PIN		GPIO4

// FPGA
#define BP_FPGA_SPI		SPI2
