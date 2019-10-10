

enum
{
	HIZ = 0,
#ifdef BP_USE_DUMMY1
	DUMMY1,
#endif
#ifdef BP_USE_DUMMY1
	DUMMY2,
#endif
#ifdef BP_USE_HWSPI
	HWSPI,
#endif
#ifdef BP_USE_HWUSART
	HWUSART,
#endif
#ifdef BP_USE_HWI2C
	HWI2C,
#endif
#ifdef BP_USE_LA
	LA,
#endif
#ifdef BP_USE_SW2W
	SW2W,
#endif
#ifdef BP_USE_SW3W
	SW3W,
#endif
#ifdef BP_USE_DIO
	DIO,
#endif
#ifdef BP_USE_LCDSPI
	LCDSPI,
#endif
#ifdef BP_USE_LCDI2C	// future
	LCDI2C,
#endif
#ifdef BP_USE_1WIRE
	ONEWIRE,
#endif
	MAXPROTO
};

typedef struct _bytecode
{
    uint8_t command; //user command or write=0
    uint32_t data; //data to go with command
    uint32_t repeat; //how many times to repeat
    uint32_t repeatSent; //tracking state machine: how many sent to FPGA (not implemented)
    uint16_t option1;//optional info with command and data (bits)
    uint8_t blocking; //do we block until FPGA is done to do this command?
    uint16_t fgpaCommand; //command written to FPGA, used to stay in sync
    uint32_t returnBytesExpected; //unused
    uint32_t returnBytesReceived; //unused
}bytecode;


typedef struct _protocol
{
    //preprocessing stage
	void (*protocol_start)(void);			// start
	void (*protocol_startR)(void);			// start with read
	void (*protocol_stop)(void);			// stop
	void (*protocol_stopR)(void);			// stop with read
	uint32_t (*protocol_send)(uint32_t,uint32_t,uint8_t);	// send(/read) max 32 bit
	uint32_t (*protocol_read)(void);		// read max 32 bit
	void (*protocol_clkh)(void);			// set clk high
	void (*protocol_clkl)(void);			// set clk low
	void (*protocol_dath)(void);			// set dat hi
	void (*protocol_datl)(void);			// set dat lo
	uint32_t (*protocol_dats)(void);		// toglle dat (?)
	void (*protocol_clk)(void);			    // toggle clk (?)
	uint32_t (*protocol_bitr)(void);		// read 1 bit (?)
	uint32_t (*protocol_periodic)(void);	// service to regular poll whether a byte has arrived or something interesting has happened
	void (*protocol_macro)(uint32_t);		// macro
	//post processing stage
    void (*protocol_start_post)(void);			// start
	void (*protocol_startR_post)(void);			// start with read
	void (*protocol_stop_post)(void);			// stop
	void (*protocol_stopR_post)(void);			// stop with read
	uint32_t (*protocol_send_post)(uint32_t,uint32_t,uint8_t);	// send(/read) max 32 bit
	uint32_t (*protocol_read_post)(void);		// read max 32 bit
	void (*protocol_clkh_post)(void);			// set clk high
	void (*protocol_clkl_post)(void);			// set clk low
	void (*protocol_dath_post)(void);			// set dat hi
	void (*protocol_datl_post)(void);			// set dat lo
	uint32_t (*protocol_dats_post)(void);		// toglle dat (?)
	void (*protocol_clk_post)(void);			    // toggle clk (?)
	uint32_t (*protocol_bitr_post)(void);		// read 1 bit (?)
	uint32_t (*protocol_periodic_post)(void);	// service to regular poll whether a byte has arrived or something interesting has happened
	void (*protocol_macro_post)(uint32_t);		// macro
	//non-pipelined commands
	void (*protocol_setup)(void);			// setup UI
	void (*protocol_setup_exc)(void);		// real setup
	void (*protocol_cleanup)(void);			// cleanup for HiZ
	void (*protocol_pins)(void);			// display pin config
	void (*protocol_settings)(void);		// display settings
	void (*protocol_help)(void);			// display protocol specific help
	char protocol_name[10];				// friendly name (promptname)
} protocol;

extern struct _protocol protocols[MAXPROTO];


void nullfunc1(void);
uint32_t nullfunc2(uint32_t c);
uint32_t nullfunc3(void);
void nullfunc4(uint32_t c);
uint32_t nullfunc5(uint32_t c,uint32_t r,uint8_t b);
void nohelp(void);
uint32_t noperiodic(void);

