//*******************************************************************
//Author: Gyouho Kim
//Description: Pressure Sensing System with RDC and temp sensor
//			Modified from 'Pstack_Ondemand_v3.6'
//			Modified from 'Tstack_Ondemand_v5.0a'
//			v5.0: PMUv9, Y8 chips
//			v5.1: SRAM tuning
//			v6.0: RDCv3, PRCv18; no calibration for snt due to space
//*******************************************************************
#include "PRCv18.h"
#include "PRCv18_RF.h"
#include "mbus.h"
#include "SNTv1_RF.h"
#include "PMUv7_RF.h"
#include "SRRv4_RF.h"
#include "RDCv3_RF.h"

// uncomment this for debug mbus message
#define DEBUG_MBUS_MSG_1

// TStack order  PRC->SRR->SNT->RDC->PMU->MEM
#define SRR_ADDR 0x4
#define SNT_ADDR 0x5
#define PMU_ADDR 0x6
#define MEM_ADDR 0x7
#define RDC_ADDR 0x8

// Temp Sensor parameters
#define	MBUS_DELAY 100 // Amount of delay between successive messages; 100: 6-7ms
#define SNS_CYCLE_INIT 3 

// Pstack states
#define	PSTK_IDLE		0x0
#define	PSTK_LDO		0x1
#define	PSTK_TEMP_START 0x2
#define	PSTK_TEMP_READ  0x3
#define PSTK_RDC_RUN 	0x4
#define PSTK_RDC_READ  	0x5

#define	PMU_10C 0x0
#define PMU_20C 0x1
#define	PMU_25C 0x2
#define	PMU_55C 0x3
#define	PMU_85C 0x4

#define NUM_TEMP_MEAS 1
#define NUM_RDC_MEAS 1

// Radio configurations
#define RADIO_DATA_LENGTH 192 // 96 bit header, 96 bit data
#define WAKEUP_PERIOD_RADIO_INIT 0x10 // About 2 sec (PRCv18)

#define DATA_STORAGE_SIZE 8192 // MEMv1: 16kB, 8k 2-byte data

#define TIMERWD_VAL 0xFFFFF // 0xFFFFF about 13 sec with Y5 run default clock (PRCv18)
#define TIMER32_VAL 0x50000 // 0x20000 about 1 sec with Y5 run default clock (PRCv18)


//********************************************************************
// Global Variables
//********************************************************************
// "static" limits the variables to this file, giving compiler more freedom
// "volatile" should only be used for MMIO --> ensures memory storage
volatile uint32_t irq_history;
volatile uint32_t enumerated;
volatile uint32_t wakeup_data;
volatile uint32_t Pstack_state;
volatile uint32_t wfi_timeout_flag;
volatile uint32_t exec_count;
volatile uint32_t meas_count;
volatile uint32_t exec_count_irq;
volatile uint32_t wakeup_period_count;
volatile uint32_t wakeup_timer_multiplier;
volatile uint32_t PMU_ADC_4P2_VAL;
volatile uint32_t pmu_setting_state;
volatile uint32_t PMU_10C_threshold_sns;
volatile uint32_t PMU_20C_threshold_sns;
volatile uint32_t PMU_55C_threshold_sns;
volatile uint32_t PMU_85C_threshold_sns;

volatile uint32_t NUM_MEAS_USER;


volatile uint32_t WAKEUP_PERIOD_CONT_USER; 
volatile uint32_t WAKEUP_PERIOD_CONT; 

volatile uint32_t temp_storage_latest = 150; // SNSv10
volatile uint32_t temp_storage_last_wakeup_adjust = 150; // SNSv10
volatile uint32_t temp_storage_diff = 0;
volatile uint32_t data_storage_count;
volatile uint32_t temp_storage_debug;
volatile uint32_t sns_running;
volatile uint32_t read_data_temp; // [23:0] Temp Sensor D Out
volatile uint32_t read_data_rdc;
volatile uint32_t TEMP_CALIB_A;
volatile uint32_t TEMP_CALIB_B;

volatile uint32_t snt_wup_counter_cur;
volatile uint32_t snt_timer_enabled = 0;
volatile uint32_t SNT_0P5S_VAL;

volatile uint32_t radio_tx_count;
volatile uint32_t radio_tx_option;
volatile uint32_t radio_ready;
volatile uint32_t radio_on;
volatile uint32_t RADIO_PACKET_DELAY;
volatile uint32_t radio_packet_count;

volatile uint32_t read_data_batadc_diff;
volatile uint32_t read_data_batadc;

volatile sntv1_r00_t sntv1_r00 = SNTv1_R00_DEFAULT;
volatile sntv1_r01_t sntv1_r01 = SNTv1_R01_DEFAULT;
volatile sntv1_r03_t sntv1_r03 = SNTv1_R03_DEFAULT;
volatile sntv1_r08_t sntv1_r08 = SNTv1_R08_DEFAULT;
volatile sntv1_r09_t sntv1_r09 = SNTv1_R09_DEFAULT;
volatile sntv1_r0A_t sntv1_r0A = SNTv1_R0A_DEFAULT;
volatile sntv1_r17_t sntv1_r17 = SNTv1_R17_DEFAULT;

volatile rdcv3_r01_t rdcv3_r01 = RDCv3_R01_DEFAULT;
volatile rdcv3_r20_t rdcv3_r20 = RDCv3_R20_DEFAULT;
volatile rdcv3_r21_t rdcv3_r21 = RDCv3_R21_DEFAULT;
volatile rdcv3_r22_t rdcv3_r22 = RDCv3_R22_DEFAULT;
volatile rdcv3_r24_t rdcv3_r24 = RDCv3_R24_DEFAULT;
volatile rdcv3_r25_t rdcv3_r25 = RDCv3_R25_DEFAULT;
volatile rdcv3_r28_t rdcv3_r28 = RDCv3_R28_DEFAULT;
volatile rdcv3_r2B_t rdcv3_r2B = RDCv3_R2B_DEFAULT;
volatile rdcv3_r2C_t rdcv3_r2C = RDCv3_R2C_DEFAULT;

volatile srrv4_r00_t srrv4_r00 = SRRv4_R00_DEFAULT;
volatile srrv4_r01_t srrv4_r01 = SRRv4_R01_DEFAULT;
volatile srrv4_r02_t srrv4_r02 = SRRv4_R02_DEFAULT;
volatile srrv4_r03_t srrv4_r03 = SRRv4_R03_DEFAULT;
volatile srrv4_r04_t srrv4_r04 = SRRv4_R04_DEFAULT;
volatile srrv4_r07_t srrv4_r07 = SRRv4_R07_DEFAULT;
volatile srrv4_r11_t srrv4_r11 = SRRv4_R11_DEFAULT;
volatile srrv4_r12_t srrv4_r12 = SRRv4_R12_DEFAULT;
volatile srrv4_r13_t srrv4_r13 = SRRv4_R13_DEFAULT;
volatile srrv4_r14_t srrv4_r14 = SRRv4_R14_DEFAULT;
volatile srrv4_r15_t srrv4_r15 = SRRv4_R15_DEFAULT;
volatile srrv4_r1E_t srrv4_r1E = SRRv4_R1E_DEFAULT;
volatile srrv4_r1F_t srrv4_r1F = SRRv4_R1F_DEFAULT;

volatile prcv18_r0B_t prcv18_r0B = PRCv18_R0B_DEFAULT;
volatile prcv18_r1C_t prcv18_r1C = PRCv18_R1C_DEFAULT;

//*******************************************************************
// INTERRUPT HANDLERS (Updated for PRCv18)
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
	// Report who woke up
	delay(MBUS_DELAY);
	mbus_write_message32(0xAA,*SREG_WAKEUP_SOURCE); // 0x1: GOC; 0x2: PRC Timer; 0x10: SNT
}

//***************************************************
// CRC16 Encoding
//***************************************************
#define DATA_LEN 96
#define CRC_LEN 16

uint32_t* crcEnc16(uint32_t data2, uint32_t data1, uint32_t data0)
{
    // intialization
    uint32_t i;
   
    uint16_t poly = 0xc002;
    uint16_t poly_not = ~poly;
    uint16_t remainder = 0x0000;
    uint16_t remainder_shift = 0x0000;
    data2 = (data2 << CRC_LEN) + (data1 >> CRC_LEN);
    data1 = (data1 << CRC_LEN) + (data0 >> CRC_LEN);
    data0 = data0 << CRC_LEN;
    
    // LFSR
    uint16_t input_bit;
    for (i = 0; i < DATA_LEN; i++)
    {
        uint16_t MSB;
        if (remainder > 0x7fff)
            MSB = 0xffff;
        else
            MSB = 0x0000;
        
        if (i < 32)
            input_bit = ((data2 << i) > 0x7fffffff);
        else if (i < 64)
            input_bit = (data1 << (i-32)) > 0x7fffffff;
        else
            input_bit = (data0 << (i-64)) > 0x7fffffff;

        remainder_shift = remainder << 1;
        remainder = (poly&((remainder_shift)^MSB))|((poly_not)&(remainder_shift))
                         + (input_bit^(remainder > 0x7fff));
    }

    data0 = data0 + remainder;

    static uint32_t msg_out[3];
    msg_out[0] = data2;
    msg_out[1] = data1;
    msg_out[2] = data0;

    return msg_out;    
}

//***************************************************
// Sleep Functions
//***************************************************
static void operation_sns_sleep_check(void);
static void operation_sleep(void);
static void operation_sleep_noirqreset(void);
static void operation_sleep_notimer(void);
static void operation_sleep_snt_timer(void);

//************************************
// PMU Related Functions
//************************************

inline static void pmu_set_adc_period(uint32_t val){
	// PMU_CONTROLLER_DESIRED_STATE Active
	// Updated for PMUv9
	mbus_remote_register_write(PMU_ADDR,0x3C,
		((  1 << 0) //state_sar_scn_on
		| (0 << 1) //state_wait_for_clock_cycles
		| (1 << 2) //state_wait_for_time
		| (1 << 3) //state_sar_scn_reset
		| (1 << 4) //state_sar_scn_stabilized
		| (1 << 5) //state_sar_scn_ratio_roughly_adjusted
		| (1 << 6) //state_clock_supply_switched
		| (1 << 7) //state_control_supply_switched
		| (1 << 8) //state_upconverter_on
		| (1 << 9) //state_upconverter_stabilized
		| (1 << 10) //state_refgen_on
		| (0 << 11) //state_adc_output_ready
		| (0 << 12) //state_adc_adjusted
		| (0 << 13) //state_sar_scn_ratio_adjusted
		| (1 << 14) //state_downconverter_on
		| (1 << 15) //state_downconverter_stabilized
		| (1 << 16) //state_vdd_3p6_turned_on
		| (1 << 17) //state_vdd_1p2_turned_on
		| (1 << 18) //state_vdd_0P6_turned_on
		| (0 << 19) //state_vbat_read
		| (1 << 20) //state_state_horizon
	));
	delay(MBUS_DELAY*10);

	// Register 0x36: TICK_REPEAT_VBAT_ADJUST
    mbus_remote_register_write(PMU_ADDR,0x36,val); 
	delay(MBUS_DELAY*10);

	// Register 0x33: TICK_ADC_RESET
	mbus_remote_register_write(PMU_ADDR,0x33,2);
	delay(MBUS_DELAY);

	// Register 0x34: TICK_ADC_CLK
	mbus_remote_register_write(PMU_ADDR,0x34,2);
	delay(MBUS_DELAY);

	// PMU_CONTROLLER_DESIRED_STATE Active
	// Updated for PMUv9
	mbus_remote_register_write(PMU_ADDR,0x3C,
		((  1 << 0) //state_sar_scn_on
		| (1 << 1) //state_wait_for_clock_cycles
		| (1 << 2) //state_wait_for_time
		| (1 << 3) //state_sar_scn_reset
		| (1 << 4) //state_sar_scn_stabilized
		| (1 << 5) //state_sar_scn_ratio_roughly_adjusted
		| (1 << 6) //state_clock_supply_switched
		| (1 << 7) //state_control_supply_switched
		| (1 << 8) //state_upconverter_on
		| (1 << 9) //state_upconverter_stabilized
		| (1 << 10) //state_refgen_on
		| (0 << 11) //state_adc_output_ready
		| (0 << 12) //state_adc_adjusted
		| (0 << 13) //state_sar_scn_ratio_adjusted
		| (1 << 14) //state_downconverter_on
		| (1 << 15) //state_downconverter_stabilized
		| (1 << 16) //state_vdd_3p6_turned_on
		| (1 << 17) //state_vdd_1p2_turned_on
		| (1 << 18) //state_vdd_0P6_turned_on
		| (0 << 19) //state_vbat_read
		| (1 << 20) //state_state_horizon
	));
	delay(MBUS_DELAY);
}

inline static void pmu_set_active_clk(uint8_t r, uint8_t l, uint8_t base, uint8_t l_1p2){

	// The first register write to PMU needs to be repeated
	// Register 0x16: V1P2 Active
    mbus_remote_register_write(PMU_ADDR,0x16, 
		( (0 << 19) // Enable PFM even during periodic reset
		| (0 << 18) // Enable PFM even when Vref is not used as ref
		| (0 << 17) // Enable PFM
		| (3 << 14) // Comparator clock division ratio
		| (0 << 13) // Enable main feedback loop
		| (r << 9)  // Frequency multiplier R
		| (l_1p2 << 5)  // Frequency multiplier L (actually L+1)
		| (base) 		// Floor frequency base (0-63)
	));
	delay(MBUS_DELAY);
    mbus_remote_register_write(PMU_ADDR,0x16, 
		( (0 << 19) // Enable PFM even during periodic reset
		| (0 << 18) // Enable PFM even when Vref is not used as ref
		| (0 << 17) // Enable PFM
		| (3 << 14) // Comparator clock division ratio
		| (0 << 13) // Enable main feedback loop
		| (r << 9)  // Frequency multiplier R
		| (l_1p2 << 5)  // Frequency multiplier L (actually L+1)
		| (base) 		// Floor frequency base (0-63)
	));
	delay(MBUS_DELAY);
	// Register 0x18: V3P6 Active 
    mbus_remote_register_write(PMU_ADDR,0x18, 
		( (3 << 14) // Desired Vout/Vin ratio; defualt: 0
		| (0 << 13) // Enable main feedback loop
		| (r << 9)  // Frequency multiplier R
		| (l << 5)  // Frequency multiplier L (actually L+1)
		| (base) 		// Floor frequency base (0-63)
	));
	delay(MBUS_DELAY);
	// Register 0x1A: V0P6 Active
    mbus_remote_register_write(PMU_ADDR,0x1A,
		( (0 << 13) // Enable main feedback loop
		| (r << 9)  // Frequency multiplier R
		| (l << 5)  // Frequency multiplier L (actually L+1)
		| (base) 		// Floor frequency base (0-63)
	));
	delay(MBUS_DELAY);

}

inline static void pmu_set_sleep_clk(uint8_t r, uint8_t l, uint8_t base, uint8_t l_1p2){

	// Register 0x17: V3P6 Sleep
    mbus_remote_register_write(PMU_ADDR,0x17, 
		( (3 << 14) // Desired Vout/Vin ratio; defualt: 0
		| (0 << 13) // Enable main feedback loop
		| (r << 9)  // Frequency multiplier R
		| (l << 5)  // Frequency multiplier L (actually L+1)
		| (base) 		// Floor frequency base (0-63)
	));
	delay(MBUS_DELAY);
	// Register 0x15: V1P2 Sleep
    mbus_remote_register_write(PMU_ADDR,0x15, 
		( (0 << 19) // Enable PFM even during periodic reset
		| (0 << 18) // Enable PFM even when Vref is not used as ref
		| (0 << 17) // Enable PFM
		| (3 << 14) // Comparator clock division ratio
		| (0 << 13) // Enable main feedback loop
		| (r << 9)  // Frequency multiplier R
		| (l_1p2 << 5)  // Frequency multiplier L (actually L+1)
		| (base) 		// Floor frequency base (0-63)
	));
	delay(MBUS_DELAY);
	// Register 0x19: V0P6 Sleep
    mbus_remote_register_write(PMU_ADDR,0x19,
		( (0 << 13) // Enable main feedback loop
		| (r << 9)  // Frequency multiplier R
		| (l << 5)  // Frequency multiplier L (actually L+1)
		| (base) 		// Floor frequency base (0-63)
	));
	delay(MBUS_DELAY);

}

inline static void pmu_set_sleep_radio(){
	pmu_set_sleep_clk(0xF,0xA,0x5,0xF/*V1P2*/);
}

inline static void pmu_set_sleep_low(){
	pmu_set_sleep_clk(0x1,0x1,0x1,0x1/*V1P2*/);
}


inline static void pmu_setting_temp_based(){
	
	mbus_write_message32(0xB7, pmu_setting_state);
	if (pmu_setting_state == PMU_85C){
		pmu_set_active_clk(0x2,0x0,0x8,0x2/*V1P2*/);
		pmu_set_sleep_clk(0x1,0x0,0x1,0x0/*V1P2*/);

	}else if (pmu_setting_state == PMU_55C){
		pmu_set_active_clk(0x1,0x0,0x10,0x2/*V1P2*/);
		pmu_set_sleep_clk(0x1,0x1,0x1,0x1/*V1P2*/);

	}else if (pmu_setting_state == PMU_20C){
		pmu_set_active_clk(0x7,0x2,0x10,0x4/*V1P2*/);
		pmu_set_sleep_clk(0xF,0x0,0x1,0x1/*V1P2*/);

	}else if (pmu_setting_state == PMU_10C){
		pmu_set_active_clk(0xD,0x2,0x10,0x4/*V1P2*/);
		pmu_set_sleep_clk(0xF,0x0,0x1,0x1/*V1P2*/);

	}else{ // 25C, default
		pmu_set_active_clk(0x2,0x0,0x10,0x4/*V1P2*/);
		pmu_set_sleep_clk(0x1,0x1,0x1,0x1/*V1P2*/);
	}
}


inline static void pmu_set_clk_init(){
	pmu_setting_state = PMU_25C;
	pmu_setting_temp_based();
	// SAR_RATIO_OVERRIDE
	// Use the new reset scheme in PMUv3
    mbus_remote_register_write(PMU_ADDR,0x05, //default 12'h000
		( (0 << 13) // Enables override setting [12] (1'b1)
		| (0 << 12) // Let VDD_CLK always connected to vbat
		| (1 << 11) // Enable override setting [10] (1'h0)
		| (0 << 10) // Have the converter have the periodic reset (1'h0)
		| (0 << 9) // Enable override setting [8] (1'h0)
		| (0 << 8) // Switch input / output power rails for upconversion (1'h0)
		| (0 << 7) // Enable override setting [6:0] (1'h0)
		| (47) 		// Binary converter's conversion ratio (7'h00)
	));
	delay(MBUS_DELAY);
    mbus_remote_register_write(PMU_ADDR,0x05, //default 12'h000
		( (1 << 13) // Enables override setting [12] (1'b1)
		| (0 << 12) // Let VDD_CLK always connected to vbat
		| (1 << 11) // Enable override setting [10] (1'h0)
		| (0 << 10) // Have the converter have the periodic reset (1'h0)
		| (1 << 9) // Enable override setting [8] (1'h0)
		| (0 << 8) // Switch input / output power rails for upconversion (1'h0)
		| (1 << 7) // Enable override setting [6:0] (1'h0)
		| (47) 		// Binary converter's conversion ratio (7'h00)
	));
	delay(MBUS_DELAY);

	pmu_set_adc_period(1); // 0x100 about 1 min for 1/2/1 1P2 setting
}


inline static void pmu_adc_reset_setting(){
	// PMU ADC will be automatically reset when system wakes up
	// PMU_CONTROLLER_DESIRED_STATE Active
	// Updated for PMUv9
	mbus_remote_register_write(PMU_ADDR,0x3C,
		((  1 << 0) //state_sar_scn_on
		| (1 << 1) //state_wait_for_clock_cycles
		| (1 << 2) //state_wait_for_time
		| (1 << 3) //state_sar_scn_reset
		| (1 << 4) //state_sar_scn_stabilized
		| (1 << 5) //state_sar_scn_ratio_roughly_adjusted
		| (1 << 6) //state_clock_supply_switched
		| (1 << 7) //state_control_supply_switched
		| (1 << 8) //state_upconverter_on
		| (1 << 9) //state_upconverter_stabilized
		| (1 << 10) //state_refgen_on
		| (0 << 11) //state_adc_output_ready
		| (0 << 12) //state_adc_adjusted
		| (0 << 13) //state_sar_scn_ratio_adjusted
		| (1 << 14) //state_downconverter_on
		| (1 << 15) //state_downconverter_stabilized
		| (1 << 16) //state_vdd_3p6_turned_on
		| (1 << 17) //state_vdd_1p2_turned_on
		| (1 << 18) //state_vdd_0P6_turned_on
		| (0 << 19) //state_vbat_read
		| (1 << 20) //state_state_horizon
	));
	delay(MBUS_DELAY);
}

inline static void pmu_adc_disable(){
	// PMU ADC will be automatically reset when system wakes up
	// PMU_CONTROLLER_DESIRED_STATE Sleep
	// Updated for PMUv9
	mbus_remote_register_write(PMU_ADDR,0x3B,
		((  1 << 0) //state_sar_scn_on
		| (1 << 1) //state_wait_for_clock_cycles
		| (1 << 2) //state_wait_for_time
		| (1 << 3) //state_sar_scn_reset
		| (1 << 4) //state_sar_scn_stabilized
		| (1 << 5) //state_sar_scn_ratio_roughly_adjusted
		| (1 << 6) //state_clock_supply_switched
		| (1 << 7) //state_control_supply_switched
		| (1 << 8) //state_upconverter_on
		| (1 << 9) //state_upconverter_stabilized
		| (1 << 10) //state_refgen_on
		| (0 << 11) //state_adc_output_ready
		| (0 << 12) //state_adc_adjusted
		| (1 << 13) //state_sar_scn_ratio_adjusted // Turn on for old adc, off for new adc
		| (1 << 14) //state_downconverter_on
		| (1 << 15) //state_downconverter_stabilized
		| (1 << 16) //state_vdd_3p6_turned_on
		| (1 << 17) //state_vdd_1p2_turned_on
		| (1 << 18) //state_vdd_0P6_turned_on
		| (0 << 19) //state_vbat_read // Turn off for old adc
		| (1 << 20) //state_state_horizon
	));
	delay(MBUS_DELAY);
}

inline static void pmu_adc_enable(){
	// PMU ADC will be automatically reset when system wakes up
	// PMU_CONTROLLER_DESIRED_STATE Sleep
	// Updated for PMUv9
	mbus_remote_register_write(PMU_ADDR,0x3B,
		((  1 << 0) //state_sar_scn_on
		| (1 << 1) //state_wait_for_clock_cycles
		| (1 << 2) //state_wait_for_time
		| (1 << 3) //state_sar_scn_reset
		| (1 << 4) //state_sar_scn_stabilized
		| (1 << 5) //state_sar_scn_ratio_roughly_adjusted
		| (1 << 6) //state_clock_supply_switched
		| (1 << 7) //state_control_supply_switched
		| (1 << 8) //state_upconverter_on
		| (1 << 9) //state_upconverter_stabilized
		| (1 << 10) //state_refgen_on
		| (1 << 11) //state_adc_output_ready
		| (0 << 12) //state_adc_adjusted // Turning off offset cancellation
		| (1 << 13) //state_sar_scn_ratio_adjusted // Turn on for old adc, off for new adc
		| (1 << 14) //state_downconverter_on
		| (1 << 15) //state_downconverter_stabilized
		| (1 << 16) //state_vdd_3p6_turned_on
		| (1 << 17) //state_vdd_1p2_turned_on
		| (1 << 18) //state_vdd_0P6_turned_on
		| (0 << 19) //state_vbat_read // Turn off for old adc
		| (1 << 20) //state_state_horizon
	));
	delay(MBUS_DELAY);
}


inline static void pmu_adc_read_latest(){

	// Grab latest PMU ADC readings
	// PMU register read is handled differently
	mbus_remote_register_write(PMU_ADDR,0x00,0x03);
	delay(MBUS_DELAY);
	// Updated for PMUv9
	read_data_batadc = *((volatile uint32_t *) REG0) & 0xFF;
	
	if (read_data_batadc<PMU_ADC_4P2_VAL){
		read_data_batadc_diff = 0;
	}else{
		read_data_batadc_diff = read_data_batadc - PMU_ADC_4P2_VAL;
	}

}

inline static void pmu_reset_solar_short(){
	// Updated for PMUv9
    mbus_remote_register_write(PMU_ADDR,0x0E, 
		( (0 << 12) // 1: solar short by latched vbat_high (new); 0: follow [10] setting
		| (1 << 11) // Reset of vbat_high latch for [12]=1
		| (1 << 10) // When to turn on harvester-inhibiting switch (0: PoR, 1: VBAT high)
		| (1 << 9)  // Enables override setting [8]
		| (0 << 8)  // Turn on the harvester-inhibiting switch
		| (3 << 4)  // clamp_tune_bottom (increases clamp thresh)
		| (0) 		// clamp_tune_top (decreases clamp thresh)
	));
	delay(MBUS_DELAY);
    mbus_remote_register_write(PMU_ADDR,0x0E, 
		( (0 << 12) // 1: solar short by latched vbat_high (new); 0: follow [10] setting
		| (1 << 11) // Reset of vbat_high latch for [12]=1
		| (1 << 10) // When to turn on harvester-inhibiting switch (0: PoR, 1: VBAT high)
		| (0 << 9)  // Enables override setting [8]
		| (0 << 8)  // Turn on the harvester-inhibiting switch
		| (3 << 4)  // clamp_tune_bottom (increases clamp thresh)
		| (0) 		// clamp_tune_top (decreases clamp thresh)
	));
	delay(MBUS_DELAY);
}

//***************************************************
// RDC Functions (RDCv3)
//***************************************************

static void rdc_enable_vref(){
	rdcv3_r2C.RDC_ENb_PG_VREF = 1;
	mbus_remote_register_write(RDC_ADDR,0x2C,rdcv3_r2C.as_int);
}
static void rdc_disable_vref(){
	rdcv3_r2C.RDC_ENb_PG_VREF = 0;
	mbus_remote_register_write(RDC_ADDR,0x2C,rdcv3_r2C.as_int);
}

static void rdc_disable_pg_v1p2(){
	rdcv3_r2C.RDC_EN_PG_FSM = 0;
	rdcv3_r2C.RDC_EN_PG_RC_OSC = 0;
	rdcv3_r2C.RDC_EN_PG_AMP_V1P2 = 0;
	rdcv3_r2C.RDC_EN_PG_ADC_V1P2 = 0;
	rdcv3_r2C.RDC_EN_PG_BUF_VH_V1P2 = 0;
	mbus_remote_register_write(RDC_ADDR,0x2C,rdcv3_r2C.as_int);
}
static void rdc_enable_pg_v1p2(){
	rdcv3_r2C.RDC_EN_PG_FSM = 1;
	rdcv3_r2C.RDC_EN_PG_RC_OSC = 1;
	rdcv3_r2C.RDC_EN_PG_AMP_V1P2 = 1;
	rdcv3_r2C.RDC_EN_PG_ADC_V1P2 = 1;
	rdcv3_r2C.RDC_EN_PG_BUF_VH_V1P2 = 1;
	mbus_remote_register_write(RDC_ADDR,0x2C,rdcv3_r2C.as_int);
}

static void rdc_disable_pg_vbat(){
	rdcv3_r2C.RDC_ENb_PG_AMP_VBAT = 1;
	rdcv3_r2C.RDC_ENb_PG_ADC_VBAT = 1;
	rdcv3_r2C.RDC_ENb_PG_BUF_VCM = 1;
	rdcv3_r2C.RDC_ENb_PG_BUF_VH_VBAT = 1;
	rdcv3_r2C.RDC_ENb_MIRROR_LDO = 0;
	mbus_remote_register_write(RDC_ADDR,0x2C,rdcv3_r2C.as_int);
}
static void rdc_enable_pg_vbat(){
	rdcv3_r2C.RDC_ENb_PG_AMP_VBAT = 0;
	rdcv3_r2C.RDC_ENb_PG_ADC_VBAT = 0;
	rdcv3_r2C.RDC_ENb_PG_BUF_VCM = 0;
	rdcv3_r2C.RDC_ENb_PG_BUF_VH_VBAT = 0;
	rdcv3_r2C.RDC_ENb_MIRROR_LDO = 1;
	mbus_remote_register_write(RDC_ADDR,0x2C,rdcv3_r2C.as_int);
}

static void rdc_disable_clock(){
	rdcv3_r28.RDC_RESET_RC_OSC = 1;
	rdcv3_r2B.RDC_CLK_ISOLATE = 1;
	mbus_remote_register_write(RDC_ADDR,0x28,rdcv3_r28.as_int);
	mbus_remote_register_write(RDC_ADDR,0x2B,rdcv3_r2B.as_int);
}
static void rdc_enable_clock(){
	rdcv3_r28.RDC_RESET_RC_OSC = 0;
	rdcv3_r2B.RDC_CLK_ISOLATE = 0;
	mbus_remote_register_write(RDC_ADDR,0x28,rdcv3_r28.as_int);
	mbus_remote_register_write(RDC_ADDR,0x2B,rdcv3_r2B.as_int);
}

static void rdc_start_meas(){
	rdcv3_r20.RDC_RESETn_FSM = 1;
	rdcv3_r20.RDC_ISOLATE = 0;
	mbus_remote_register_write(RDC_ADDR,0x20,rdcv3_r20.as_int);
}
static void rdc_reset(){
	rdcv3_r20.RDC_RESETn_FSM = 0;
	rdcv3_r20.RDC_ISOLATE = 1;
	mbus_remote_register_write(RDC_ADDR,0x20,rdcv3_r20.as_int);
}

//***************************************************
// Temp Sensor Functions (SNTv1)
//***************************************************

static void temp_sensor_start(){
	sntv1_r01.TSNS_RESETn = 1;
	mbus_remote_register_write(SNT_ADDR,1,sntv1_r01.as_int);
}
static void temp_sensor_reset(){
	sntv1_r01.TSNS_RESETn = 0;
	mbus_remote_register_write(SNT_ADDR,1,sntv1_r01.as_int);
}
static void temp_sensor_power_on(){
	// Turn on digital block
	sntv1_r01.TSNS_SEL_LDO = 1;
	mbus_remote_register_write(SNT_ADDR,1,sntv1_r01.as_int);
	// Turn on analog block
	sntv1_r01.TSNS_EN_SENSOR_LDO = 1;
	mbus_remote_register_write(SNT_ADDR,1,sntv1_r01.as_int);

	delay(MBUS_DELAY);

	// Release isolation
	sntv1_r01.TSNS_ISOLATE = 0;
	mbus_remote_register_write(SNT_ADDR,1,sntv1_r01.as_int);
}
static void temp_sensor_power_off(){
	sntv1_r01.TSNS_RESETn = 0;
	sntv1_r01.TSNS_SEL_LDO = 0;
	sntv1_r01.TSNS_EN_SENSOR_LDO = 0;
	sntv1_r01.TSNS_ISOLATE = 1;
	mbus_remote_register_write(SNT_ADDR,1,sntv1_r01.as_int);
}
static void sns_ldo_vref_on(){
	sntv1_r00.LDO_EN_VREF 	= 1;
	mbus_remote_register_write(SNT_ADDR,0,sntv1_r00.as_int);
}

static void sns_ldo_power_on(){
	sntv1_r00.LDO_EN_IREF 	= 1;
	sntv1_r00.LDO_EN_LDO	= 1;
	mbus_remote_register_write(SNT_ADDR,0,sntv1_r00.as_int);
}
static void sns_ldo_power_off(){
	sntv1_r00.LDO_EN_VREF 	= 0;
	sntv1_r00.LDO_EN_IREF 	= 0;
	sntv1_r00.LDO_EN_LDO	= 0;
	mbus_remote_register_write(SNT_ADDR,0,sntv1_r00.as_int);
}

static void snt_read_wup_counter(){

	set_halt_until_mbus_rx();
	mbus_remote_register_read(SNT_ADDR,0x1B,1); // CNT_VALUE_EXT
	snt_wup_counter_cur = (*REG1 & 0xFF)<<24;
	mbus_remote_register_read(SNT_ADDR,0x1C,1); // CNT_VALUE
	snt_wup_counter_cur = snt_wup_counter_cur | (*REG1 & 0xFFFFFF);
	set_halt_until_mbus_tx();

}
	
static void snt_start_timer_presleep(){

	sntv1_r09.TMR_IBIAS_REF = 0x4; // Default : 4'h4
	mbus_remote_register_write(SNT_ADDR,0x09,sntv1_r09.as_int);

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

	// TIMER EN_SEL_CLK Reset 
	sntv1_r08.TMR_EN_SELF_CLK = 0x1; // Default : 0x0
	mbus_remote_register_write(SNT_ADDR,0x08,sntv1_r08.as_int);

	// TIMER SELF_EN 
	sntv1_r09.TMR_SELF_EN = 0x1; // Default : 0x0
	mbus_remote_register_write(SNT_ADDR,0x09,sntv1_r09.as_int);
    //delay(100000); 

	snt_timer_enabled = 1;
}

static void snt_start_timer_postsleep(){
	// Turn off sloscillator
	sntv1_r08.TMR_EN_OSC = 0x0; // Default : 0x0
	mbus_remote_register_write(SNT_ADDR,0x08,sntv1_r08.as_int);
}


static void snt_stop_timer(){

	// EN_OSC
	sntv1_r08.TMR_EN_OSC = 0x0; // Default : 0x0
	// RESET
	sntv1_r08.TMR_EN_SELF_CLK = 0x0; // Default : 0x0
	sntv1_r08.TMR_RESETB = 0x0;// Default : 0x0
	sntv1_r08.TMR_RESETB_DIV = 0x0; // Default : 0x0
	sntv1_r08.TMR_RESETB_DCDC = 0x0; // Default : 0x0
	mbus_remote_register_write(SNT_ADDR,0x08,sntv1_r08.as_int);
	snt_timer_enabled = 0;

	sntv1_r09.TMR_IBIAS_REF = 0x0; // Default : 4'h4
	mbus_remote_register_write(SNT_ADDR,0x09,sntv1_r09.as_int);

	sntv1_r17.WUP_ENABLE = 0x0; // Default : 0x
	mbus_remote_register_write(SNT_ADDR,0x17,sntv1_r17.as_int);

}

static void snt_set_wup_timer(uint32_t sleep_count){
	
	snt_wup_counter_cur = sleep_count + snt_wup_counter_cur; // should handle rollover

	mbus_remote_register_write(SNT_ADDR,0x19,snt_wup_counter_cur>>24);
	mbus_remote_register_write(SNT_ADDR,0x1A,snt_wup_counter_cur & 0xFFFFFF);
	
	sntv1_r17.WUP_ENABLE = 0x1; // Default : 0x
	mbus_remote_register_write(SNT_ADDR,0x17,sntv1_r17.as_int);

}

//***************************************************
// Radio transmission routines for SRR
//***************************************************

static void radio_power_on(){

	// Switch PMU to higher setting
	//pmu_set_sleep_radio();
	
    radio_on = 1;

    // Turn on Current Limter
    srrv4_r00.SRR_CL_EN = 1;  //Enable CL
    mbus_remote_register_write(SRR_ADDR,0x00,srrv4_r00.as_int);

    // Release timer power-gate
    srrv4_r04.RO_EN_RO_V1P2 = 1;  //Use V1P2 for TIMER
    mbus_remote_register_write(SRR_ADDR,0x04,srrv4_r04.as_int);
    delay(MBUS_DELAY);

	// Turn on timer
    srrv4_r04.RO_RESET = 0;  //Release Reset TIMER
    mbus_remote_register_write(SRR_ADDR,0x04,srrv4_r04.as_int);
    delay(MBUS_DELAY);

    srrv4_r04.RO_EN_CLK = 1; //Enable CLK TIMER
    mbus_remote_register_write(SRR_ADDR,0x04,srrv4_r04.as_int);
    delay(MBUS_DELAY);

    srrv4_r04.RO_ISOLATE_CLK = 0; //Set Isolate CLK to 0 TIMER
    mbus_remote_register_write(SRR_ADDR,0x04,srrv4_r04.as_int);

    // Release FSM Sleep
    srrv4_r11.SRR_RAD_FSM_SLEEP = 0;  // Power on BB
    mbus_remote_register_write(SRR_ADDR,0x11,srrv4_r11.as_int);
	delay(MBUS_DELAY*5); // Freq stab

}

static void radio_power_off(){

	// Switch PMU to lower setting
	//pmu_set_sleep_low();

    // Turn off everything
    srrv4_r03.SRR_TRX_ISOLATEN = 0;     //set ISOLATEN 0
    mbus_remote_register_write(SRR_ADDR,0x03,srrv4_r03.as_int);

    srrv4_r11.SRR_RAD_FSM_EN = 0;  //Stop BB
    srrv4_r11.SRR_RAD_FSM_RSTN = 0;  //RST BB
    srrv4_r11.SRR_RAD_FSM_SLEEP = 1;
    mbus_remote_register_write(SRR_ADDR,0x11,srrv4_r11.as_int);

    // Turn off Current Limter
    srrv4_r00.SRR_CL_EN = 0;  //Enable CL
    mbus_remote_register_write(SRR_ADDR,0x00,srrv4_r00.as_int);

    srrv4_r04.RO_RESET = 1;  //Release Reset TIMER
    srrv4_r04.RO_EN_CLK = 0; //Enable CLK TIMER
    srrv4_r04.RO_ISOLATE_CLK = 1; //Set Isolate CLK to 0 TIMER
    mbus_remote_register_write(SRR_ADDR,0x04,srrv4_r04.as_int);

    // Enable timer power-gate
    srrv4_r04.RO_EN_RO_V1P2 = 0;  //Use V1P2 for TIMER
    mbus_remote_register_write(SRR_ADDR,0x04,srrv4_r04.as_int);

    radio_on = 0;
	radio_ready = 0;
}

static void srr_configure_pulse_width_short(){

    srrv4_r12.SRR_RAD_FSM_TX_PW_LEN = 0; //4us PW
    srrv4_r13.SRR_RAD_FSM_TX_C_LEN = 32; // (PW_LEN+1):C_LEN=1:32
    srrv4_r12.SRR_RAD_FSM_TX_PS_LEN = 1; // PW=PS guard interval betwen 0 and 1 pulse

    mbus_remote_register_write(SRR_ADDR,0x12,srrv4_r12.as_int);
    mbus_remote_register_write(SRR_ADDR,0x13,srrv4_r13.as_int);
}

static void send_radio_data_srr_sub1(){

	// Use Timer32 as timeout counter
    wfi_timeout_flag = 0;
	config_timer32(TIMER32_VAL, 1, 0, 0); // 1/10 of MBUS watchdog timer default

    // Fire off data
	srrv4_r11.SRR_RAD_FSM_EN = 1;  //Start BB
	mbus_remote_register_write(SRR_ADDR,0x11,srrv4_r11.as_int);

	// Wait for radio response
	WFI();

	// Turn off Timer32
	*TIMER32_GO = 0;

	if (wfi_timeout_flag){
		mbus_write_message32(0xFA, 0xFAFAFAFA);
	}

	srrv4_r11.SRR_RAD_FSM_EN = 0;
	mbus_remote_register_write(SRR_ADDR,0x11,srrv4_r11.as_int);
}

static void send_radio_data_srr(uint32_t last_packet, uint8_t radio_packet_prefix, uint16_t radio_data_2, uint32_t radio_data_1, uint32_t radio_data_0, uint8_t num_tx_duplicate){
	// Sends 168 bit packet, of which 96b is actual data
	// MRR REG_9: reserved for header
	// MRR REG_A: reserved for header
	// MRR REG_B: reserved for header
	// MRR REG_C: reserved for header
	// MRR REG_D: DATA[23:0]
	// MRR REG_E: DATA[47:24]
	// MRR REG_F: DATA[71:48]
	// MRR REG_10: DATA[95:72]

	// CRC16 Encoding 
    uint32_t* output_data;
	
    output_data = crcEnc16(((radio_packet_count & 0xFF)<<8) | radio_packet_prefix, (radio_data_2 <<16) | ((radio_data_1 & 0xFFFFFF) >>8), (radio_data_1 << 24) | (radio_data_0 & 0xFFFFFF));

    mbus_remote_register_write(SRR_ADDR,0xD,radio_data_0);
    mbus_remote_register_write(SRR_ADDR,0xE,radio_data_1);
    mbus_remote_register_write(SRR_ADDR,0xF,(radio_packet_prefix<<16)|radio_data_2);
    mbus_remote_register_write(SRR_ADDR,0x10,(radio_packet_count&0xFF)|((output_data[2] & 0xFFFF)/*CRC16*/<<8));

    if (!radio_ready){
		radio_ready = 1;

		// Release FSM Reset
		srrv4_r11.SRR_RAD_FSM_RSTN = 1;  //UNRST BB
		mbus_remote_register_write(SRR_ADDR,0x11,srrv4_r11.as_int);
		delay(MBUS_DELAY);

    	srrv4_r03.SRR_TRX_ISOLATEN = 1;     //set ISOLATEN 1, let state machine control
    	mbus_remote_register_write(SRR_ADDR,0x03,srrv4_r03.as_int);
		delay(MBUS_DELAY);

    }
	uint8_t ii = 0;

	while (ii<=num_tx_duplicate){
		send_radio_data_srr_sub1();
		delay(RADIO_PACKET_DELAY);
		ii++;
	}

	radio_packet_count++;

	if (last_packet){
		radio_ready = 0;
		radio_power_off();
	}
}



//***************************************************
// End of Program Sleep Operation
//***************************************************
static void operation_sns_sleep_check(void){
	// Make sure LDO is off
	if (sns_running){
		sns_running = 0;
		temp_sensor_power_off();
		sns_ldo_power_off();
	}
}

static void operation_sleep(void){

	// Reset GOC_DATA_IRQ
	*GOC_DATA_IRQ = 0;

    // Go to Sleep
    mbus_sleep_all();
    while(1);

}

static void operation_sleep_snt_timer(void){

	// Reset GOC_DATA_IRQ
	*GOC_DATA_IRQ = 0;

	// Disable PRC Timer
	set_wakeup_timer(0, 0, 0);

    // Go to Sleep
    mbus_sleep_all();
    while(1);

}

static void operation_sleep_noirqreset(void){

    // Go to Sleep
    mbus_sleep_all();
    while(1);

}

static void operation_sleep_notimer(void){
    
	// Make sure the irq counter is reset    
    exec_count_irq = 0;

	operation_sns_sleep_check();

    // Make sure Radio is off
    if (radio_on){
		radio_power_off();
	}

	// Make sure SNT timer is off
	if (snt_timer_enabled){
		snt_stop_timer();
	}

	// Disable Timer
	set_wakeup_timer(0, 0, 0);

    // Go to sleep
    operation_sleep();

}


static void operation_tx_stored(uint8_t num_duplicate_packets){

    //Fire off stored data to radio
	uint32_t radio_tx_limit;
	if (data_storage_count < NUM_MEAS_USER){
		radio_tx_limit = data_storage_count;
	}else{
		radio_tx_limit = NUM_MEAS_USER;
	}

    while(radio_tx_count < radio_tx_limit){
		#ifdef DEBUG_MBUS_MSG_1
			mbus_write_message32(0xDD, radio_tx_count);
			//mbus_write_message32(0xDD, temp_storage[radio_tx_count]);
		#endif

		// Reset watchdog timer
		config_timerwd(TIMERWD_VAL);

		// Radio out data
		uint32_t mem_read_data[2];
		set_halt_until_mbus_rx();
		// Read 2 words (4 data points) at a time
		mbus_copy_mem_from_remote_to_any_bulk(MEM_ADDR, (uint32_t*)((radio_tx_count>>1)<<2), 0x01, (uint32_t*)&mem_read_data, 1);
		set_halt_until_mbus_tx();
	
		send_radio_data_srr(0,0xDD,mem_read_data[1]>>16,((mem_read_data[1]&0xFFFF)<<8) | (mem_read_data[0]>>24),mem_read_data[0]&0xFFFFFF,num_duplicate_packets);
		radio_tx_count = radio_tx_count + 4;
    }

	send_radio_data_srr(1,0xD3,*REG_CHIP_ID,(0xBB00|read_data_batadc_diff)<<8,data_storage_count,1);	
	// This is also the end of this IRQ routine
	exec_count_irq = 0;

	// Go to sleep without timer
	operation_sleep_notimer();
}


static void operation_init(void){
  
	// Set CPU & Mbus Clock Speeds
    prcv18_r0B.CLK_GEN_RING = 0x1; // Default 0x1
    prcv18_r0B.CLK_GEN_DIV_MBC = 0x1; // Default 0x1
    prcv18_r0B.CLK_GEN_DIV_CORE = 0x3; // Default 0x3
    prcv18_r0B.GOC_CLK_GEN_SEL_DIV = 0x0; // Default 0x0
    prcv18_r0B.GOC_CLK_GEN_SEL_FREQ = 0x6; // Default 0x6
	*REG_CLKGEN_TUNE = prcv18_r0B.as_int;

    prcv18_r1C.SRAM0_TUNE_ASO_DLY = 31; // Default 0x0, 5 bits
    prcv18_r1C.SRAM0_TUNE_DECODER_DLY = 15; // Default 0x2, 4 bits
    prcv18_r1C.SRAM0_USE_INVERTER_SA = 0; // Default 0x2, 4 bits
	*REG_SRAM0_TUNE = prcv18_r1C.as_int;
  
    //Enumerate & Initialize Registers
    Pstack_state = PSTK_IDLE; 	//0x0;
    enumerated = 0x70636000;
    exec_count = 0;
    exec_count_irq = 0;
	PMU_ADC_4P2_VAL = 0x4B;
  
    // Set CPU Halt Option as RX --> Use for register read e.g.
    //set_halt_until_mbus_rx();

    //Enumeration
    mbus_enumerate(SRR_ADDR);
	delay(MBUS_DELAY);
    mbus_enumerate(SNT_ADDR);
	delay(MBUS_DELAY);
    mbus_enumerate(RDC_ADDR);
	delay(MBUS_DELAY);
 	mbus_enumerate(PMU_ADDR);
	delay(MBUS_DELAY);
 	mbus_enumerate(MEM_ADDR);
	delay(MBUS_DELAY);

    // Set CPU Halt Option as TX --> Use for register write e.g.
	//set_halt_until_mbus_tx();

	// PMU Settings ----------------------------------------------
	pmu_set_clk_init();
	pmu_reset_solar_short();

	// New for PMUv9
	// VBAT_READ_TRIM Register
    mbus_remote_register_write(PMU_ADDR,0x45,
		( (0x0 << 9) // 0x0: no mon; 0x1: sar conv mon; 0x2: up conv mon; 0x3: down conv mon
		| (0x0 << 8) // 1: vbat_read_mode enable; 0: vbat_read_mode disable
		| (0x48 << 0) //sampling multiplication factor N; vbat_read out = vbat/1p2*N
	));
	delay(MBUS_DELAY);

	// Disable PMU ADC measurement in active mode
	// PMU_CONTROLLER_STALL_ACTIVE
	// Updated for PMUv9
    mbus_remote_register_write(PMU_ADDR,0x3A, 
		( (1 << 20) // ignore state_horizon; default 1
		| (0 << 19) // state_vbat_read
		| (1 << 13) // ignore adc_output_ready; default 0
		| (1 << 12) // ignore adc_output_ready; default 0
		| (1 << 11) // ignore adc_output_ready; default 0
	));
    delay(MBUS_DELAY);
	pmu_adc_reset_setting();
	pmu_adc_enable();

    // Temp Sensor Settings --------------------------------------
    // sntv1_r01
	sntv1_r01.TSNS_RESETn = 0;
    sntv1_r01.TSNS_EN_IRQ = 1;
    sntv1_r01.TSNS_BURST_MODE = 0;
    sntv1_r01.TSNS_CONT_MODE = 0;
    mbus_remote_register_write(SNT_ADDR,1,sntv1_r01.as_int);

	// Set temp sensor conversion time
	sntv1_r03.TSNS_SEL_STB_TIME = 0x1; 
	sntv1_r03.TSNS_SEL_CONV_TIME = 0x6; // Default: 0x6
	mbus_remote_register_write(SNT_ADDR,0x03,sntv1_r03.as_int);

    // SNT Wakeup Timer Settings --------------------------------------
    // Config Register A
	sntv1_r0A.TMR_S = 0x1; // Default: 0x4, use 1 for good TC
	// Tune R for TC
    sntv1_r0A.TMR_DIFF_CON = 0x3FEF; // Default: 0x3FFB
    sntv1_r0A.TMR_POLY_CON = 0x1; // Default: 0x1
    mbus_remote_register_write(SNT_ADDR,0x0A,sntv1_r0A.as_int);

	// TIMER CAP_TUNE  
	// Tune C for freq
	sntv1_r09.TMR_SEL_CAP = 0x80; // Default : 8'h8
	sntv1_r09.TMR_SEL_DCAP = 0x3F; // Default : 6'h4

	sntv1_r09.TMR_EN_TUNE1 = 0x1; // Default : 1'h1
	sntv1_r09.TMR_EN_TUNE2 = 0x1; // Default : 1'h1

	// to reduce standby current
	sntv1_r09.TMR_IBIAS_REF = 0x0; // Default : 4'h4

	mbus_remote_register_write(SNT_ADDR,0x09,sntv1_r09.as_int);

	// Wakeup Counter
	sntv1_r17.WUP_CLK_SEL = 0x0; 
	sntv1_r17.WUP_AUTO_RESET = 0x0; // Automatically reset counter to 0 upon sleep 
	mbus_remote_register_write(SNT_ADDR,0x17,sntv1_r17.as_int);

	// RDC Settings ----------------------------------------
	// MBUS return
	rdcv3_r01.IRQ_RPLY_REG_ADDR = 1;
	
	// rdcv3_r20
	rdcv3_r20.RDC_CNT_AMP1 = 0xC;
	
	// rdcv3_r21
	rdcv3_r21.RDC_CNT_AMP2 = 0x8;
	
	// rdcv3_r22
	rdcv3_r22.RDC_CNT_SKIP = 0x3;
	rdcv3_r22.RDC_OSR = 0xB;
	
	// rdcv3_r24 & r25
	rdcv3_r24.RDC_SEL_DLY = 0xA;

	rdcv3_r24.RDC_SEL_GAIN_LC = 0xA;
	rdcv3_r25.RDC_OFFSET_P_LC = 0x19;
	rdcv3_r25.RDC_OFFSET_PB_LC = 0x6;
	mbus_remote_register_write(RDC_ADDR,0x01,rdcv3_r01.as_int);
	mbus_remote_register_write(RDC_ADDR,0x20,rdcv3_r20.as_int);
	mbus_remote_register_write(RDC_ADDR,0x21,rdcv3_r21.as_int);
	mbus_remote_register_write(RDC_ADDR,0x22,rdcv3_r22.as_int);
	mbus_remote_register_write(RDC_ADDR,0x24,rdcv3_r24.as_int);
	mbus_remote_register_write(RDC_ADDR,0x25,rdcv3_r25.as_int);
        

    // Radio Settings (SRRv4) -------------------------------------------
    srrv4_r1F.LC_CLK_RING = 0x3;  // ~ 150 kHz
    srrv4_r1F.LC_CLK_DIV = 0x3;  // ~ 150 kHz
    mbus_remote_register_write(SRR_ADDR,0x1F,srrv4_r1F.as_int);

	srr_configure_pulse_width_short();

	// RO setup (SFO)
	// Adjust Diffusion R
	mbus_remote_register_write(SRR_ADDR,0x06,0x1000); // RO_PDIFF

	// Adjust Poly R
	mbus_remote_register_write(SRR_ADDR,0x08,0x400000); // RO_POLY

	// Adjust C
	srrv4_r07.RO_MOM = 0x10;
	srrv4_r07.RO_MIM = 0x10;
	mbus_remote_register_write(SRR_ADDR,0x07,srrv4_r07.as_int);

    // Current Limter set-up 
    srrv4_r00.SRR_CL_CTRL = 16; //Set CL 1-finite 16-20uA

    // TX Setup Carrier Freq
    srrv4_r00.SRR_TRX_CAP_ANTP_TUNE = 0x0000;  //ANT CAP 14b unary 830.5 MHz
	//srrv4_r00.SRR_TRX_CAP_ANTP_TUNE = 0x00FF;  //ANT CAP 14b unary 813.8 MHz
	//srrv4_r00.SRR_TRX_CAP_ANTP_TUNE = 0x0FFF;  //ANT CAP 14b unary 805.5 MHz
    mbus_remote_register_write(SRR_ADDR,0x00,srrv4_r00.as_int);
    srrv4_r01.SRR_TRX_CAP_ANTN_TUNE = 0x0000; //ANT CAP 14b unary 830.5 MHz
	//srrv4_r01.SRR_TRX_CAP_ANTN_TUNE = 0x00FF; //ANT CAP 14b unary 813.8 MHz
	//srrv4_r01.SRR_TRX_CAP_ANTN_TUNE = 0x0FFF;  //ANT CAP 14b unary 805.5 MHz
    mbus_remote_register_write(SRR_ADDR,0x01,srrv4_r01.as_int);
    srrv4_r02.SRR_TX_BIAS_TUNE = 0x0FFF;  //Set TX BIAS TUNE 13b // Set to max
    mbus_remote_register_write(SRR_ADDR,0x02,srrv4_r02.as_int);

    // RX Setup
	srrv4_r03.SRR_RX_BIAS_TUNE    = 0x000F;//  turn on Q_enhancement
    srrv4_r03.SRR_RX_SAMPLE_CAP    = 0x1;  // RX_SAMPLE_CAP
    mbus_remote_register_write(SRR_ADDR,3,srrv4_r03.as_int);

    srrv4_r14.SRR_RAD_FSM_RX_POWERON_LEN = 0x0;  //Set RX Power on length
    //srrv4_r14.SRR_RAD_FSM_RX_SAMPLE_LEN = 0x3;  //Set RX Sample length  16us
    srrv4_r14.SRR_RAD_FSM_RX_SAMPLE_LEN = 0x7;  //Set RX Sample length  4us
    srrv4_r14.SRR_RAD_FSM_GUARD_LEN = 0x001C; //Set TX_RX Guard length, TX_RX guard 32 cycle (28+5)
    mbus_remote_register_write(SRR_ADDR,0x14,srrv4_r14.as_int);

    srrv4_r15.SRR_RAD_FSM_RX_HDR_BITS = 0x00;  //Set RX header
    srrv4_r15.SRR_RAD_FSM_RX_HDR_TH = 0x00;    //Set RX header threshold
    srrv4_r15.SRR_RAD_FSM_RX_DATA_BITS = 0x1; //Set RX data 16b
    mbus_remote_register_write(SRR_ADDR,0x15,srrv4_r15.as_int);

    srrv4_r1E.SRR_IRQ_REPLY_PACKET = 0x061400; //Read RX data Reply
    mbus_remote_register_write(SRR_ADDR,0x1E,srrv4_r1E.as_int);

    mbus_remote_register_write(SRR_ADDR,0x15,srrv4_r15.as_int);
    
	// Use pulse generator
    srrv4_r02.SRR_TX_PULSE_FINE = 1;
    srrv4_r02.SRR_TX_PULSE_FINE_TUNE = 1;
    mbus_remote_register_write(SRR_ADDR,0x02,srrv4_r02.as_int);

    srrv4_r13.SRR_RAD_FSM_SEED = 1; //default
    srrv4_r14.SRR_RAD_FSM_TX_POWERON_LEN = 7; //3bits
    mbus_remote_register_write(SRR_ADDR,0x14,srrv4_r14.as_int);

	// RAD_FSM set-up 
	// Using first 48 bits of data as header
	mbus_remote_register_write(SRR_ADDR,0x09,0x0);
	mbus_remote_register_write(SRR_ADDR,0x0A,0x0);
	mbus_remote_register_write(SRR_ADDR,0x0B,0x0);
	mbus_remote_register_write(SRR_ADDR,0x0C,0x7AC800);
	srrv4_r11.SRR_RAD_FSM_TX_H_LEN = 0; //31-31b header (max)
	srrv4_r11.SRR_RAD_FSM_TX_D_LEN = RADIO_DATA_LENGTH; //0-skip tx data
	mbus_remote_register_write(SRR_ADDR,0x11,srrv4_r11.as_int);

	srrv4_r13.SRR_RAD_FSM_TX_MODE = 3; //code rate 0:4 1:3 2:2 3:1(baseline) 4:1/2 5:1/3 6:1/4
	mbus_remote_register_write(SRR_ADDR,0x13,srrv4_r13.as_int);

	// Mbus return address
	mbus_remote_register_write(SRR_ADDR,0x1E,0x1002);


    // Initialize other global variables
	WAKEUP_PERIOD_CONT_USER = 0x2710;
    data_storage_count = 0;
    radio_tx_count = 0;
    radio_tx_option = 0; //enables radio tx for each measurement 
    sns_running = 0;
    radio_ready = 0;
    radio_on = 0;
	wakeup_data = 0;
	RADIO_PACKET_DELAY = 1200;
	radio_packet_count = 0;
	
	pmu_setting_state = PMU_25C;
	PMU_10C_threshold_sns = 600; // Around 10C
	PMU_20C_threshold_sns = 1000; // Around 20C
	PMU_55C_threshold_sns = 4000; // Around 55C
	PMU_85C_threshold_sns = 9000; // Around 85C

	SNT_0P5S_VAL = 1000;
	TEMP_CALIB_A = 24000;
	TEMP_CALIB_B = 3750000;

	NUM_MEAS_USER = DATA_STORAGE_SIZE;

    // Go to sleep without timer
    operation_sleep_notimer();
}


//***************************************************
// Pressure / Temperature measurement operation (SNTv1, RDCv1)
//***************************************************
static void operation_sns_run(void){

	if (Pstack_state == PSTK_IDLE){
		Pstack_state = PSTK_LDO;

		wfi_timeout_flag = 0;
		meas_count = 0;

		// Turn on SNS LDO VREF; requires settling
		sns_ldo_vref_on();

		// Turn on RDC VREF; requires settling
		rdc_enable_vref();  
        //delay(MBUS_DELAY*10);

		// Power on radio
		if (radio_tx_option || (exec_count < SNS_CYCLE_INIT)){
			radio_power_on();
			// Put system to sleep
			//set_wakeup_timer(WAKEUP_PERIOD_RADIO_INIT, 0x1, 0x1);
			//operation_sleep();
		}

    }else if (Pstack_state == PSTK_LDO){
		Pstack_state = PSTK_TEMP_START;

		// Power on SNS LDO
		sns_ldo_power_on();

		// Power on temp sensor
		temp_sensor_power_on();
		delay(MBUS_DELAY);

	}else if (Pstack_state == PSTK_TEMP_START){
		// Start temp measurement

		wfi_timeout_flag = 0;

		// Use Timer32 as timeout counter
		config_timer32(TIMER32_VAL, 1, 0, 0); // 1/10 of MBUS watchdog timer default

		// Start Temp Sensor
		temp_sensor_start();

		// Wait for temp sensor output
		WFI();

		// Turn off Timer32
		*TIMER32_GO = 0;
		Pstack_state = PSTK_TEMP_READ;

	}else if (Pstack_state == PSTK_TEMP_READ){

		// Grab Temp Sensor Data
		if (wfi_timeout_flag){
			mbus_write_message32(0xFA, 0xFAFAFAFA);
			// If there was a timeout, measure one more time; meas_count not incremented
		}else{
			// Read register
			set_halt_until_mbus_rx();
			mbus_remote_register_read(SNT_ADDR,0x6,1);
			read_data_temp = *REG1;
			set_halt_until_mbus_tx();
			meas_count++;
		}

		// Last measurement from this wakeup
		if (meas_count == NUM_TEMP_MEAS){
			if (wfi_timeout_flag){
				// In case of timeout, wakeup counter needs to be adjusted 
				snt_read_wup_counter();
			}else{
				temp_storage_latest = read_data_temp;
			}
		}

		// Option to take multiple measurements per wakeup
		if (meas_count < NUM_TEMP_MEAS){	
			// Repeat measurement while awake
			temp_sensor_reset();
			Pstack_state = PSTK_TEMP_START;
				
		}else{
			meas_count = 0;

			// Read latest PMU ADC measurement
			pmu_adc_read_latest();
		
			// Change PMU based on temp
			if (temp_storage_latest > PMU_85C_threshold_sns){
				if (pmu_setting_state != PMU_85C){
					pmu_setting_state = PMU_85C;
					pmu_setting_temp_based();
				}
			}else if (temp_storage_latest > PMU_55C_threshold_sns){
				if (pmu_setting_state != PMU_55C){
					pmu_setting_state = PMU_55C;
					pmu_setting_temp_based();
				}
			}else if (temp_storage_latest < PMU_10C_threshold_sns){
				if (pmu_setting_state != PMU_10C){
					pmu_setting_state = PMU_10C;
					pmu_setting_temp_based();
				}
			}else if (temp_storage_latest < PMU_20C_threshold_sns){
				if (pmu_setting_state != PMU_20C){
					pmu_setting_state = PMU_20C;
					pmu_setting_temp_based();
				}
			}else{
				if (pmu_setting_state != PMU_25C){
					pmu_setting_state = PMU_25C;
					pmu_setting_temp_based();
				}
			}

			// Assert temp sensor isolation & turn off temp sensor power
			temp_sensor_power_off();
			sns_ldo_power_off();
			Pstack_state = PSTK_RDC_RUN;
		}

	}else if (Pstack_state == PSTK_RDC_RUN){
	// Start RDC measurement
		wfi_timeout_flag = 0;

		if (meas_count == 0){
			// Release RDC power gates / isolation
            rdc_disable_pg_v1p2();
            delay(MBUS_DELAY);
            rdc_disable_pg_vbat();
            delay(MBUS_DELAY);
            rdc_enable_clock();
            delay(MBUS_DELAY);
		}

		// Use Timer32 as timeout counter
		config_timer32(TIMER32_VAL, 1, 0, 0); // 1/10 of MBUS watchdog timer default

		// Start RDC
        rdc_start_meas();

		// Wait for output
		WFI();

		// Turn off Timer32
		*TIMER32_GO = 0;
		Pstack_state = PSTK_RDC_READ;

	}else if (Pstack_state == PSTK_RDC_READ){

		// Grab RDC Data
		if (wfi_timeout_flag){
			read_data_rdc = 0xFAFA;
			mbus_write_message32(0xFA, 0xFAFAFAFA);
		}else{
			// Read register
			set_halt_until_mbus_rx();
			mbus_remote_register_read(RDC_ADDR,0x11,1);
			read_data_rdc = *REG1;
			set_halt_until_mbus_tx();
		}
		meas_count++;

		// Option to take multiple measurements per wakeup
		if (meas_count < NUM_RDC_MEAS){	
			// Repeat measurement while awake
			rdc_reset();
			Pstack_state = PSTK_RDC_RUN;
				
		}else{
			meas_count = 0;

			// Assert RDC isolation & turn off RDC power
			rdc_reset();
			rdc_disable_clock();
			rdc_enable_pg_vbat();
			rdc_enable_pg_v1p2();
			rdc_disable_vref();  
			Pstack_state = PSTK_IDLE;

			// Don't use median for now
			//uint32_t median_idx = rdc_data_median();

			#ifdef DEBUG_MBUS_MSG_1
			mbus_write_message32(0xCC, exec_count);
			#endif
			mbus_write_message32(0xC0, (exec_count << 16) | temp_storage_latest);
			mbus_write_message32(0xC1, read_data_rdc);


			if (temp_storage_debug){
				temp_storage_latest = exec_count;
			}

			// Store results in MEMv1; unless memory is full
			if (data_storage_count + 1 < DATA_STORAGE_SIZE){
				// Read destination address
				// Memory address is per byte
				// Memory is word-addressable; first word: 0x0, second word: 0x4, third word: 0x8 ...
			
				uint32_t mem_write_data;
				mem_write_data = (temp_storage_latest & 0xFFFF) | (read_data_rdc<<16);
				mbus_copy_mem_from_local_to_remote_bulk(MEM_ADDR, (uint32_t*)((data_storage_count>>1)<<2), (uint32_t*)&mem_write_data, 0);
				data_storage_count = data_storage_count + 2;
			}


			// Optionally transmit the data
			if (radio_tx_option || (exec_count < SNS_CYCLE_INIT)){
				send_radio_data_srr(0,0xC0,*REG_CHIP_ID,((0xBB00|read_data_batadc_diff)<<8)|exec_count,temp_storage_latest,0);
				send_radio_data_srr(1,0xC4,*REG_CHIP_ID,((0xBB00|read_data_batadc_diff)<<8)|exec_count,read_data_rdc,0);
			}

			exec_count++;

			// Make sure Radio is off
			if (radio_on){
				radio_ready = 0;
				radio_power_off();
			}

			// Use SNT Timer	
			snt_set_wup_timer(WAKEUP_PERIOD_CONT_USER);
			operation_sleep_snt_timer();

		}

    }else{
        //default:  // THIS SHOULD NOT HAPPEN
		// Reset Temp Sensor 
		temp_sensor_power_off();
		sns_ldo_power_off();
		// Reset RDC 
		rdc_reset();
		rdc_disable_clock();
		rdc_enable_pg_vbat();
		rdc_enable_pg_v1p2();
		rdc_disable_vref();  
		operation_sleep_notimer();
    }

}


static void operation_goc_trigger_init(void){

	// This is critical
	set_halt_until_mbus_tx();

	// Debug
	mbus_write_message32(0xBA,wakeup_data);

	// Read latest PMU ADC measurement
	pmu_adc_read_latest();

	// Initialize variables & registers
	sns_running = 0;
	Pstack_state = PSTK_IDLE;
	
	radio_power_off();
	temp_sensor_power_off();
	sns_ldo_power_off();

}

static void operation_goc_trigger_radio(uint32_t radio_tx_num, uint32_t wakeup_timer_val, uint8_t radio_tx_prefix, uint8_t radio_tx_data1, uint32_t radio_tx_data0){

	// Prepare radio TX
	radio_power_on();

	if (exec_count_irq < radio_tx_num){
		exec_count_irq++;
		// radio
		send_radio_data_srr(1,radio_tx_prefix,*REG_CHIP_ID,((0xBB00|read_data_batadc_diff)<<8)|radio_tx_data1,radio_tx_data0,1);	
		// set timer
		set_wakeup_timer (wakeup_timer_val, 0x1, 0x1);
		// go to sleep and wake up with same condition
		operation_sleep_noirqreset();

	}else{
		// radio
		send_radio_data_srr(1,radio_tx_prefix,*REG_CHIP_ID,((0xBB00|read_data_batadc_diff)<<8)|radio_tx_data1,radio_tx_data0,1);	
		// Go to sleep without timer
		operation_sleep_notimer();
	}
}



//********************************************************************
// MAIN function starts here             
//********************************************************************

int main() {

    // Only enable relevant interrupts (PRCv18)
	//enable_reg_irq();
	//enable_all_irq();
	*NVIC_ISER = (1 << IRQ_WAKEUP) | (1 << IRQ_GOCEP) | (1 << IRQ_TIMER32) | (1 << IRQ_REG0)| (1 << IRQ_REG1)| (1 << IRQ_REG2)| (1 << IRQ_REG3);
  
    // Config watchdog timer to about 10 sec; default: 0x02FFFFFF
    config_timerwd(TIMERWD_VAL);

    // Initialization sequence
    if (enumerated != 0x70636000){
        operation_init();
    }

    // Check if wakeup is due to GOC interrupt  
    // 0x8C is reserved for GOC-triggered wakeup (Named GOC_DATA_IRQ)
    // 8 MSB bits of the wakeup data are used for function ID
    wakeup_data = *GOC_DATA_IRQ;
    uint32_t wakeup_data_header = (wakeup_data>>24) & 0xFF;
    uint32_t wakeup_data_field_0 = wakeup_data & 0xFF;
    uint32_t wakeup_data_field_1 = wakeup_data>>8 & 0xFF;
    uint32_t wakeup_data_field_2 = wakeup_data>>16 & 0xFF;

	// In case GOC triggered in the middle of routines
	if ((wakeup_data_header != 0) && (exec_count_irq == 0)){
		operation_goc_trigger_init();
	}

    if(wakeup_data_header == 0x01){
        // Debug mode: Transmit something via radio and go to sleep w/o timer
        // wakeup_data[7:0] is the # of transmissions
        // wakeup_data[15:8] is the user-specified period
        // wakeup_data[23:16] is the MSB of # of transmissions
		operation_goc_trigger_radio(wakeup_data_field_0 + (wakeup_data_field_2<<8), wakeup_data_field_1, 0xA0, read_data_batadc, exec_count_irq);


    }else if(wakeup_data_header == 0x02){
		// Slow down PMU sleep osc and run temp sensor code with desired wakeup period
		// wakeup_data[15:0] is the desired wakeup period in 0.5s (min 0.5s: 0x1, max 32767.5s: 0xFFFF)
        // wakeup_data[20] enables radio tx for each measurement
        // wakeup_data[21] specifies which wakeup timer to use 0: SNT timer, 1: PRC timer
        // wakeup_data[23] is debug mode: logs the execution count as opposed to temp data
        radio_tx_option = wakeup_data_field_2 & 0x10;
		temp_storage_debug = wakeup_data_field_2 & 0x80;
		
		// Use SNT timer
		WAKEUP_PERIOD_CONT_USER = (wakeup_data & 0xFFFF)*SNT_0P5S_VAL; // Unit is 0.5s

        exec_count_irq++;

		if (exec_count_irq == 1){
			// SNT pulls higher current in the beginning
			pmu_set_sleep_radio();
			snt_start_timer_presleep();
			// Go to sleep for >3s for timer stabilization
			set_wakeup_timer (WAKEUP_PERIOD_RADIO_INIT*3, 0x1, 0x1);
			operation_sleep_noirqreset();
		}else if (exec_count_irq == 2){
			snt_start_timer_postsleep();
			// Read existing counter value; in case not reset to zero
			snt_read_wup_counter();
			// Restore sleep setting to low
			pmu_set_sleep_low();
		}

		if (!sns_running){
			sns_running = 1;
		}

		exec_count = 0;
		meas_count = 0;
		data_storage_count = 0;
		radio_tx_count = 0;

		// Reset GOC_DATA_IRQ
		*GOC_DATA_IRQ = 0;
        exec_count_irq = 0;

		// Run Temp Sensor Program
    	Pstack_state = PSTK_IDLE;
		operation_sns_run();

    }else if(wakeup_data_header == 0x03){
		// Stop temp sensor program and transmit the battery reading and execution count (alternating n times)
        // wakeup_data[7:0] is the # of transmissions
        // wakeup_data[15:8] is the user-specified period 

		if (pmu_setting_state != PMU_25C){
			// Set PMU to room temp setting
			pmu_setting_state = PMU_25C;
			pmu_setting_temp_based();
		}
		operation_sns_sleep_check();
		snt_stop_timer();

		Pstack_state = PSTK_IDLE;

		operation_goc_trigger_radio(wakeup_data_field_0, wakeup_data_field_1, 0xC1, exec_count_irq, data_storage_count);


    }else if(wakeup_data_header == 0x04){
        // Transmit the stored temp sensor data
		// wakeup_data[7:0] specifies # of duplicate packets sent during Data TX; if 0, no duplicate packets

		operation_sns_sleep_check();
		snt_stop_timer();

		Pstack_state = PSTK_IDLE;

		radio_tx_count = 0;

		uint32_t not_enough_batt = 0;
	
		// Prepare radio TX
		radio_power_on();

		if (exec_count_irq == 0){

			radio_packet_count = 0;

			// Make sure the battery capacity is enough for transmission
			// FIXME
			if (data_storage_count > 4000){
				if (read_data_batadc_diff > 3){ // 0x4D okay
					not_enough_batt = 1;
				}
			}else if (data_storage_count > 2000){ // 0x4E okay
				if (read_data_batadc_diff > 4){
					not_enough_batt = 1;
				}
			}else{
				if (read_data_batadc_diff > 5){ // 0x4F okay
					not_enough_batt = 1;
				}
			}
		
			if (not_enough_batt){
				// Send Error Message & Go to sleep
				send_radio_data_srr(1,0xDF,*REG_CHIP_ID,(0xBB00|read_data_batadc_diff)<<8,0xFAFAFA,1);	
				// Go to sleep without timer
				operation_sleep_notimer();
			}
		}

        if (exec_count_irq < 2){
			exec_count_irq++;

			if (exec_count_irq == 1){
				send_radio_data_srr(1,0xD0,*REG_CHIP_ID,(0xBB00|read_data_batadc_diff)<<8,data_storage_count,1);	
			}else{
				send_radio_data_srr(1,0xD1,*REG_CHIP_ID,TEMP_CALIB_A,TEMP_CALIB_B,1);	
			}
			set_wakeup_timer(WAKEUP_PERIOD_RADIO_INIT, 0x1, 0x1);
			operation_sleep_noirqreset();
		
		}else{
			operation_tx_stored(wakeup_data_field_0);
		}
		
	}else if(wakeup_data_header == 0x12){
	// Configure # of measurement
		NUM_MEAS_USER = wakeup_data & 0x3FFF;
		if (NUM_MEAS_USER > 8192) NUM_MEAS_USER = 8192;
		if (NUM_MEAS_USER < 5) NUM_MEAS_USER = 5;
		if ((wakeup_data & 0x3FFF) == 0) NUM_MEAS_USER = 0;
        operation_sleep_notimer();

	}else if(wakeup_data_header == 0x13){
	// Tune SNT Timer R for TC
		sntv1_r0A.TMR_DIFF_CON = wakeup_data & 0x3FFF; // Default: 0x3FFB
		mbus_remote_register_write(SNT_ADDR,0x0A,sntv1_r0A.as_int);
        operation_sleep_notimer();

	}else if(wakeup_data_header == 0x14){
		// Update SNT wakeup counter value for 0.5s
		SNT_0P5S_VAL = wakeup_data & 0xFFFF;
		if (SNT_0P5S_VAL == 0){
			SNT_0P5S_VAL = 1000;
		}		

		// Go to sleep without timer
		operation_sleep_notimer();

    }else if(wakeup_data_header == 0x17){
		// Change the 4.2V battery reference
		if (wakeup_data_field_0 == 0){
			// Update with the current value
			PMU_ADC_4P2_VAL = read_data_batadc;
		}else{
			PMU_ADC_4P2_VAL = wakeup_data_field_0;
		}

		// Go to sleep without timer
		operation_sleep_notimer();

    }else if(wakeup_data_header == 0x1A){
		PMU_10C_threshold_sns = wakeup_data & 0xFFFF; // Around 10C
		// Go to sleep without timer
		operation_sleep_notimer();

    }else if(wakeup_data_header == 0x1B){
		PMU_20C_threshold_sns = wakeup_data & 0xFFFF; // Around 20C
		// Go to sleep without timer
		operation_sleep_notimer();

    }else if(wakeup_data_header == 0x1C){
		PMU_55C_threshold_sns = wakeup_data & 0xFFFFFF; // Around 55C
		// Go to sleep without timer
		operation_sleep_notimer();

    }else if(wakeup_data_header == 0x1D){
		PMU_85C_threshold_sns = wakeup_data & 0xFFFFFF; // Around 85C
		// Go to sleep without timer
		operation_sleep_notimer();

	}else if(wakeup_data_header == 0x20){
        // wakeup_data[7:0] is the # of transmissions
        // wakeup_data[23:8] is the desired chip id value
		//					-->  if zero: reports current chip id
		
		uint32_t chip_id_user;
		chip_id_user = (wakeup_data>>8) & 0xFFFF;

		if (chip_id_user == 0){
			chip_id_user = *REG_CHIP_ID;
		}else{
			*REG_CHIP_ID = chip_id_user;
		}

		operation_goc_trigger_radio(wakeup_data_field_0, WAKEUP_PERIOD_RADIO_INIT, 0xA1, 0, chip_id_user);


	}else if(wakeup_data_header == 0x21){
		// Change the radio tx packet delay
		uint32_t user_val = wakeup_data & 0xFFFFFF;
		if (user_val < 500){
			RADIO_PACKET_DELAY = 2000;
		}else{
			RADIO_PACKET_DELAY = user_val;
		}
		// Go to sleep without timer
		operation_sleep_notimer();

	}else if(wakeup_data_header == 0x22){
		// Change carrier frequency
		srrv4_r00.SRR_TRX_CAP_ANTP_TUNE = (wakeup_data & 0x3FFF);
		mbus_remote_register_write(SRR_ADDR,0x00,srrv4_r00.as_int);
		srrv4_r01.SRR_TRX_CAP_ANTN_TUNE = (wakeup_data & 0x3FFF);
		mbus_remote_register_write(SRR_ADDR,0x01,srrv4_r01.as_int);

		// Go to sleep without timer
		operation_sleep_notimer();

	}else if(wakeup_data_header == 0x23){
		// Change the baseband frequency of SRR (SFO)
		srrv4_r07.RO_MOM = wakeup_data_field_0 & 0x3F;
		srrv4_r07.RO_MIM = wakeup_data_field_1 & 0x3F;
		mbus_remote_register_write(SRR_ADDR,0x07,srrv4_r07.as_int);
		
		// Go to sleep without timer
		operation_sleep_notimer();

	}else if(wakeup_data_header == 0x24){
		// Pulse generator settings

		// Use pulse generator
		srrv4_r02.SRR_TX_PULSE_FINE = wakeup_data_field_1;
		srrv4_r02.SRR_TX_PULSE_FINE_TUNE = wakeup_data_field_0 & 0xF;
		mbus_remote_register_write(SRR_ADDR,0x02,srrv4_r02.as_int);

		// Go to sleep without timer
		operation_sleep_notimer();

	}else if(wakeup_data_header == 0x25){
		// Change the conversion time of the temp sensor

		sntv1_r03.TSNS_SEL_CONV_TIME = wakeup_data & 0xF; // Default: 0x6
		mbus_remote_register_write(SNT_ADDR,0x03,sntv1_r03.as_int);

		// Go to sleep without timer
		operation_sleep_notimer();

	}else if(wakeup_data_header == 0x2A){
		// Update calibration coefficient A
		// A is the slope, typical value is around 24.000, stored as A*1000
		TEMP_CALIB_A = wakeup_data & 0xFFFFFF; 

		// Go to sleep without timer
		operation_sleep_notimer();

	}else if(wakeup_data_header == 0x2B){
		// Update calibration coefficient B 
		// B is the offset, typical value is around -3750.000, stored as -B*1000
		TEMP_CALIB_B = wakeup_data & 0xFFFFFF; 

		// Go to sleep without timer
		operation_sleep_notimer();

	}else if(wakeup_data_header == 0x2C){
		// Change SRR C_LEN

		srrv4_r13.SRR_RAD_FSM_TX_C_LEN = wakeup_data & 0xFFFF; // (PW_LEN+1):C_LEN=1:32
		mbus_remote_register_write(SRR_ADDR,0x13,srrv4_r13.as_int);

		// Go to sleep without timer
		operation_sleep_notimer();

	}else if(wakeup_data_header == 0x2E){
		// Change SRR TX Amp Current
    	srrv4_r02.SRR_TX_BIAS_TUNE = wakeup_data & 0x1FFF;  //Set TX BIAS TUNE 13b // Set to max
	    mbus_remote_register_write(SRR_ADDR,0x02,srrv4_r02.as_int);

		// Go to sleep without timer
		operation_sleep_notimer();

	}else if(wakeup_data_header == 0x2F){
		// Change SRR Current Limiter Settings

    	srrv4_r00.SRR_CL_CTRL = wakeup_data & 0xFF; //Set CL 1-finite 16-20uA
		mbus_remote_register_write(SRR_ADDR,0x00,srrv4_r00.as_int);

		// Go to sleep without timer
		operation_sleep_notimer();

	}else if(wakeup_data_header == 0x30){
		// Change RDC Gain
        rdcv3_r24.RDC_SEL_GAIN_LC = wakeup_data & 0x1F;
        mbus_remote_register_write(RDC_ADDR,0x24,rdcv3_r24.as_int);

		// Go to sleep without timer
		operation_sleep_notimer();

	}else if(wakeup_data_header == 0x31){
		// Change RDC Offset value (5bit x2)
        rdcv3_r25.RDC_OFFSET_P_LC = wakeup_data_field_1 & 0x1F;
        rdcv3_r25.RDC_OFFSET_PB_LC = wakeup_data & 0x1F;
        mbus_remote_register_write(RDC_ADDR,0x25,rdcv3_r25.as_int);


		// Go to sleep without timer
		operation_sleep_notimer();

	}else if(wakeup_data_header == 0x41){
		// Change PMU solar short clamp value

		mbus_remote_register_write(PMU_ADDR,0x0E, 
			( (1 << 10) // When to turn on harvester-inhibiting switch (0: PoR, 1: VBAT high)
			| (1 << 9)  // Enables override setting [8]
			| (0 << 8)  // Turn on the harvester-inhibiting switch
			| (wakeup_data & 0xF0)  // clamp_tune_bottom (increases clamp thresh)
			| (wakeup_data & 0xF) 		// clamp_tune_top (decreases clamp thresh)
		));
		delay(MBUS_DELAY);
		mbus_remote_register_write(PMU_ADDR,0x0E, 
			( (1 << 10) // When to turn on harvester-inhibiting switch (0: PoR, 1: VBAT high)
			| (0 << 9)  // Enables override setting [8]
			| (0 << 8)  // Turn on the harvester-inhibiting switch
			| (wakeup_data & 0xF0)  // clamp_tune_bottom (increases clamp thresh)
			| (wakeup_data & 0xF) 		// clamp_tune_top (decreases clamp thresh)
		));
		delay(MBUS_DELAY);

		// Go to sleep without timer
		operation_sleep_notimer();

	}else if(wakeup_data_header == 0xF0){

		operation_goc_trigger_radio(wakeup_data_field_0, WAKEUP_PERIOD_RADIO_INIT, 0xA2, 0, enumerated);

	}else if(wakeup_data_header == 0xFA){
	// Soft reset routine

		// Extra protection
		if ((wakeup_data&0xFFFFFF) == 0x89D7E2){
			config_timerwd(0xFF);
			while(1){
				mbus_write_message32(0xE0, 0x0);
				delay(MBUS_DELAY);
			}
		}else{
			operation_sleep_notimer();
		}

    }else{
		if (wakeup_data_header != 0){
			// Invalid GOC trigger
            // Go to sleep without timer
            operation_sleep_notimer();
		}
	}


	if (sns_running){
		// Proceed to continuous mode
		while(1){
			operation_sns_run();
		}
	}

	
	operation_sleep_notimer();

    while(1);
}


