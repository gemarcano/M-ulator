//*******************************************************************
//Author: Seokhyeon 
//Description: SNTv1's CLK Testing
//*******************************************************************
#include "PRCv17.h"
#include "PRCv17_RF.h"
#include "SNTv1_RF.h"
#include "mbus.h"

// uncomment this for debug mbus message
#define DEBUG_MBUS_MSG

// uncomment this for debug radio message
//#define DEBUG_RADIO_MSG

// uncomment this to only transmit average
//#define TX_AVERAGE

// Stack order  PRC->SNS->PMU
#define SNT_ADDR 0x4
//#define PMU_ADDR 0x6

// Temp Sensor parameters
#define	MBUS_DELAY 100 // Amount of delay between successive messages; 100: 6-7ms
#define WAKEUP_PERIOD_LDO 5 // About 1 sec (PRCv17)
#define TEMP_CYCLE_INIT 3 
// Tstack states
#define	TSTK_IDLE       0x0
#define	TSTK_LDO        0x1
#define TSTK_TEMP_START 0x2
#define TSTK_TEMP_READ  0x6

#define NUM_TEMP_MEAS 2
//***************************************************
// Global variables
//***************************************************
// "static" limits the variables to this file, giving compiler more freedom
// "volatile" should only be used for MMIO --> ensures memory storage
volatile uint32_t enumerated;
volatile uint32_t wakeup_data;
volatile uint32_t Tstack_state;
volatile uint32_t wfi_timeout_flag;
volatile uint32_t exec_count;
volatile uint32_t meas_count;
volatile uint32_t exec_count_irq;
volatile uint32_t wakeup_period_count;
volatile uint32_t wakeup_timer_multiplier;
volatile uint32_t PMU_ADC_4P2_VAL;
volatile uint32_t pmu_parkinglot_mode;
volatile uint32_t pmu_harvesting_on;

volatile uint32_t WAKEUP_PERIOD_CONT_USER; 
volatile uint32_t WAKEUP_PERIOD_CONT; 
volatile uint32_t WAKEUP_PERIOD_CONT_INIT; 

volatile prcv17_r0B_t prcv17_r0B = PRCv17_R0B_DEFAULT;

// Temperature Sensor
volatile sntv1_r00_t sntv1_r00 = SNTv1_R00_DEFAULT;
volatile sntv1_r01_t sntv1_r01 = SNTv1_R01_DEFAULT;
volatile sntv1_r03_t sntv1_r03 = SNTv1_R03_DEFAULT;

// WakeUp Timer
volatile sntv1_r08_t sntv1_r08 = SNTv1_R08_DEFAULT;
volatile sntv1_r09_t sntv1_r09 = SNTv1_R09_DEFAULT;
volatile sntv1_r0A_t sntv1_r0A = SNTv1_R0A_DEFAULT;

volatile sntv1_r17_t sntv1_r17 = SNTv1_R17_DEFAULT;


//*******************************************************************
// INTERRUPT HANDLERS (Updated for PRCv17)
//*******************************************************************

void handler_ext_int_wakeup   (void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_gocep    (void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_timer32  (void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_reg0     (void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_reg1     (void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_reg2     (void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_reg3     (void) __attribute__ ((interrupt ("IRQ")));

void handler_ext_int_timer32(void) { // TIMER32
    *NVIC_ICPR = (0x1 << IRQ_TIMER32);
    *REG1 = *TIMER32_CNT;
    *REG2 = *TIMER32_STAT;
    *TIMER32_STAT = 0x0;
	wfi_timeout_flag = 1;
    }
void handler_ext_int_reg0(void) { // REG0
    *NVIC_ICPR = (0x1 << IRQ_REG0);
}
void handler_ext_int_reg1(void) { // REG1
    *NVIC_ICPR = (0x1 << IRQ_REG1);
}
void handler_ext_int_reg2(void) { // REG2
    *NVIC_ICPR = (0x1 << IRQ_REG2);
}
void handler_ext_int_reg3(void) { // REG3
    *NVIC_ICPR = (0x1 << IRQ_REG3);
}
void handler_ext_int_gocep(void) { // GOCEP
    *NVIC_ICPR = (0x1 << IRQ_GOCEP);
}
void handler_ext_int_wakeup(void) { // WAKE-UP
    *NVIC_ICPR = (0x1 << IRQ_WAKEUP); 
    *SREG_WAKEUP_SOURCE = 0;
}

//***************************************************
// End of Program Sleep Operation
//***************************************************
static void operation_sleep(void){

	// Reset GOC_DATA_IRQ
	*GOC_DATA_IRQ = 0;

    // Go to Sleep
    mbus_sleep_all();
    while(1);

}
static void operation_sleep_notimer(void){
	// Disable Timer
	set_wakeup_timer(0, 0, 0);

    // Go to sleep
    operation_sleep();

}

static void operation_init(void){
  
	// Set CPU & Mbus Clock Speeds
    prcv17_r0B.CLK_GEN_RING = 0x1; // Default 0x1
    prcv17_r0B.CLK_GEN_DIV_MBC = 0x1; // Default 0x1
    prcv17_r0B.CLK_GEN_DIV_CORE = 0x3; // Default 0x3
    prcv17_r0B.GOC_CLK_GEN_SEL_DIV = 0x0; // Default 0x0
    prcv17_r0B.GOC_CLK_GEN_SEL_FREQ = 0x6; // Default 0x6
	*REG_CLKGEN_TUNE = prcv17_r0B.as_int;

    // Disable MBus Watchdog Timer
    //*REG_MBUS_WD = 0;
	*((volatile uint32_t *) 0xA000007C) = 0;

  
    //Enumerate & Initialize Registers
    Tstack_state = TSTK_IDLE; 	//0x0;
    enumerated = 0xDEADBEE0;
    exec_count = 0;
    exec_count_irq = 0;
	PMU_ADC_4P2_VAL = 0x4B;
	pmu_parkinglot_mode = 0;
	pmu_harvesting_on = 1;
  
    // Set CPU Halt Option as RX --> Use for register read e.g.
    //set_halt_until_mbus_rx();

    //Enumeration
    //mbus_enumerate(RAD_ADDR);
	//delay(MBUS_DELAY);
    mbus_enumerate(SNT_ADDR);
	delay(MBUS_DELAY);
    //mbus_enumerate(HRV_ADDR);
	//delay(MBUS_DELAY);
 	//mbus_enumerate(PMU_ADDR);
	//delay(MBUS_DELAY);

    // Set CPU Halt Option as TX --> Use for register write e.g.
	//set_halt_until_mbus_tx();


    // Wakeup Timer Settings --------------------------------------

    // Config Register A
	sntv1_r0A.TMR_S = 0x1; // Default: 0x4
	//sntv1_r0A.TMR_S = 0x4; // Default: 0x4
    //sntv1_r0A.TMR_DIFF_CON = 0x1FFF; // Default: 0x3FFB
    //sntv1_r0A.TMR_POLY_CON = 0x1; // Default: 0x1
    sntv1_r0A.TMR_DIFF_CON = 0x3FFD; // Default: 0x3FFB
    sntv1_r0A.TMR_POLY_CON = 0x1; // Default: 0x1

    mbus_remote_register_write(SNT_ADDR,0x0A,sntv1_r0A.as_int);

	// TIMER CAP_TUNE  
	sntv1_r09.TMR_SEL_CAP = 0x80; // Default : 8'h8
	sntv1_r09.TMR_SEL_DCAP = 0x3F; // Default : 6'h4

	//sntv1_r09.TMR_SEL_CAP = 0x8; // Default : 8'h8
	//sntv1_r09.TMR_SEL_DCAP = 0x4; // Default : 6'h4
	sntv1_r09.TMR_EN_TUNE1 = 0x1; // Default : 1'h1
	sntv1_r09.TMR_EN_TUNE2 = 0x1; // Default : 1'h1
	mbus_remote_register_write(SNT_ADDR,0x09,sntv1_r09.as_int);

	// Enalbe Frequency Monitoring 
	sntv1_r17.WUP_ENABLE_CLK_SLP_OUT = 0x1; 
	mbus_remote_register_write(SNT_ADDR,0x17,sntv1_r17.as_int);

    // Initialize other global variables
    WAKEUP_PERIOD_CONT = 100;   // 10: 2-4 sec with PRCv17
	wakeup_data = 0;

    delay(MBUS_DELAY);

    // Go to sleep
	//set_wakeup_timer(WAKEUP_PERIOD_CONT, 0x1, 0x1);
    //operation_sleep();
}

//********************************************************************
// MAIN function starts here             
//***************************************************************-*****

int main() {

    // Initialize Interrupts
    // Only enable register-related interrupts
	//enable_reg_irq();
	*NVIC_ISER = (1 << IRQ_WAKEUP) | (1 << IRQ_GOCEP) | (1 << IRQ_TIMER32) | (1 << IRQ_REG0)| (1 << IRQ_REG1)| (1 << IRQ_REG2)| (1 << IRQ_REG3);
  
    // Config watchdog timer to about 10 sec; default: 0x02FFFFFF
    //config_timerwd(0xFFFFF); // 0xFFFFF about 13 sec with Y2 run default clock
	disable_timerwd();

    // Initialization sequence
    if (enumerated != 0xDEADBEEF){
        // Set up PMU/GOC register in PRC layer (every time)
        // Enumeration & RAD/SNS layer register configuration
        operation_init();

    }

	// TIMER SELF_EN Disable 
	sntv1_r09.TMR_SELF_EN = 0x0; // Default : 0x1
	mbus_remote_register_write(SNT_ADDR,0x09,sntv1_r09.as_int);

	// EN_OSC 
	sntv1_r08.TMR_EN_OSC = 0x1; // Default : 0x0
	mbus_remote_register_write(SNT_ADDR,0x08,sntv1_r08.as_int);

	// Release Reset 
	sntv1_r08.TMR_RESETB = 0x1; // Default : 0x0
	sntv1_r08.TMR_RESETB_DIV = 0x1; // Default : 0x0
	sntv1_r08.TMR_RESETB_DCDC = 0x1; // Default : 0x0
	mbus_remote_register_write(SNT_ADDR,0x08,sntv1_r08.as_int);
    delay(2000); 

	// TIMER EN_SEL_CLK Reset 
	sntv1_r08.TMR_EN_SELF_CLK = 0x1; // Default : 0x0
	mbus_remote_register_write(SNT_ADDR,0x08,sntv1_r08.as_int);
    delay(10); 

	// TIMER SELF_EN 
	sntv1_r09.TMR_SELF_EN = 0x1; // Default : 0x0
	mbus_remote_register_write(SNT_ADDR,0x09,sntv1_r09.as_int);
    delay(100000); 

	// TIMER EN_SEL_CLK Reset 
	sntv1_r08.TMR_EN_OSC = 0x0; // Default : 0x0
	mbus_remote_register_write(SNT_ADDR,0x08,sntv1_r08.as_int);
    delay(100); 


//    operation_sleep_notimer();

 while (1){
     delay(1000);
 }

 return 1;
}

