// global config file

// UI stuff
#define CMDBUFFSIZE	512		// must be power of 2

// USB shit :/

// USB VID/PID
#define		USB_VID		0x1209
#define		USB_PID		0x7331
#define		USB_VENDOR	"DangerousPrototypes.com"
#define		USB_PRODUCT	"BusPirateULTRA"

// systicks (for delays) systick is 10us
extern volatile uint32_t systicks;

// include platform
// TODO we need some makefile tricks for this
#include	"platform/beta1.h"				// BP Ultra may 2019

// included protocols
// enable protocols
//#define		BP_USE_1WIRE
//#define		BP_USE_HWUSART
//#define		BP_USE_HWI2C
#define		BP_USE_HWSPI
//#define		BP_USE_SW2W
//#define		BP_USE_SW3W
//#define 	BP_USE_DIO
//#define		BP_USE_LCDSPI
//#define		BP_USE_LCDI2C
//#define		BP_USE_LA
#define 	BP_USE_DUMMY1
#define 	BP_USE_DUMMY2




