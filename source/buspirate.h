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



