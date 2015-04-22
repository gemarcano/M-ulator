//*******************************************************************
//Author: Yejoong Kim
//Description: PRCv9E Extended Layer for controlling Flash thru GPIO
//Note: Based on PREv9_TEST_SYS_TKJANG
//*******************************************************************
#include "mbus.h"
#include "PRCv9E.h"
#include "FLSv1_GPIO.h"

volatile uint32_t enumerated;

//***************************************************
//Interrupt Handlers
//Must clear pending bit!
//***************************************************
void handler_ext_int_0(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_1(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_2(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_3(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_4(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_5(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_6(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_7(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_8(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_9(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_10(void) __attribute__ ((interrupt ("IRQ")));
void handler_ext_int_11(void) __attribute__ ((interrupt ("IRQ")));


void handler_ext_int_0(void){ *((volatile uint32_t *) 0xE000E280) = 0x1; }
void handler_ext_int_1(void){ *((volatile uint32_t *) 0xE000E280) = 0x2; }
void handler_ext_int_2(void){ *((volatile uint32_t *) 0xE000E280) = 0x4; }
void handler_ext_int_3(void){ *((volatile uint32_t *) 0xE000E280) = 0x8; }
void handler_ext_int_4(void){ *((volatile uint32_t *) 0xE000E280) = 0x10; }
void handler_ext_int_5(void){ *((volatile uint32_t *) 0xE000E280) = 0x20; }
void handler_ext_int_6(void){ *((volatile uint32_t *) 0xE000E280) = 0x40; }
void handler_ext_int_7(void){ *((volatile uint32_t *) 0xE000E280) = 0x80; }
void handler_ext_int_8(void){ *((volatile uint32_t *) 0xE000E280) = 0x100; }
void handler_ext_int_9(void){
	*((volatile uint32_t *) 0xE000E280) = 0x200;
	*((volatile uint32_t *) 0xA8000000) = 0x0;
}
void handler_ext_int_10(void){ *((volatile uint32_t *) 0xE000E280) = 0x400; }
void handler_ext_int_11(void){ *((volatile uint32_t *) 0xE000E280) = 0x800; }

//***************************************************
// Subfunctions
//***************************************************

void initialize(){

	enumerated = 0xABCDEF01;
	//Clear All Pending Interrupts
	*((volatile uint32_t *) 0xE000E280) = 0x3FF;
	//Enable Interrupts
//	*((volatile uint32_t *) 0xE000E100) = 0x07F;	// All timer ignored
//	*((volatile uint32_t *) 0xE000E100) = 0x3FF;	// All interrupt enable
	*((volatile uint32_t *) 0xE000E100) = 0;	// All interrupt disable
	// MEM&CORE CLK /16
	*((volatile uint32_t *) 0xA2000008) = 0x120F903;	//Isolation disable
	//Truning off the watch dog
	*((volatile uint32_t *) 0xA5000000) = 0;
	write_mbus_message(0x13,0xEEEEEEEE);
}

//***************************************************************************************
// MAIN function starts here             
//***************************************************************************************

int main() {
	volatile uint32_t temp_addr_0;
	volatile uint32_t temp_data_0;
	volatile uint32_t i;

	//initialize	
	if( enumerated != 0xABCDEF01 ) initialize();
	FLSMBusGPIO_initialization();

	// Enumerate FLS
	FLSMBusGPIO_sendMBus8bit (0x00, 0x24000000);
	FLSMBusGPIO_rxMsg(); // Rx Response

	// Turn on the Flash
	FLSMBusGPIO_sendMBus32bit (0x40, 0x11000003);
	FLSMBusGPIO_rxMsg();

	// Set CRT_TUNE=0x3F
	FLSMBusGPIO_sendMBus32bit (0x40, 0x0A00023F);

	for (i=0; i<40; i=i+4) {
		// Read-Out SRAM
		FLSMBusGPIO_sendMBus64bit (0x43, (i << 24), i);
		FLSMBusGPIO_rxMsg();
	
		// Store result
		temp_addr_0 = FLSMBusGPIO_getRxAddr();
		temp_data_0 = FLSMBusGPIO_getRxData0();
	
		// send MBus message for snooping
		write_mbus_message(0xFF, temp_addr_0);
		write_mbus_message(0xFF, temp_data_0);
	}

	// Copy Flash -> SRAM (Length = 0x7FE)
	FLSMBusGPIO_sendMBus32bit (0x40, 0x0500FFE3);
	FLSMBusGPIO_rxMsg(); // Rx Interrupt


	for (i=0; i<40; i=i+4) {
		// Read-Out SRAM
		FLSMBusGPIO_sendMBus64bit (0x43, (i << 24), i);
		FLSMBusGPIO_rxMsg();
	
		// Store result
		temp_addr_0 = FLSMBusGPIO_getRxAddr();
		temp_data_0 = FLSMBusGPIO_getRxData0();
	
		// send MBus message for snooping
		write_mbus_message(0xFF, temp_addr_0);
		write_mbus_message(0xFF, temp_data_0);
	}

	// Turn off the Flash
	FLSMBusGPIO_sendMBus32bit (0x40, 0x11000002);
	FLSMBusGPIO_rxMsg();

	// Make FLS go to sleep
	FLSMBusGPIO_sendMBus8bit (0x01, 0x00000000);

	// System goes to sleep
	sleep();


	while(1){
		delay(5000); //1s
	}

}
