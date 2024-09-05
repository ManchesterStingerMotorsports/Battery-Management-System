/*
 * bmsOperations.c
 *
 *  Created on: Aug 23, 2024
 *      Author: malhaar-k
 */


/*----------------INCLUDES------------------*/
#include "bmsOperations.h"

#include <stdint.h>
#include "custom_bms.h"
#include "main.h"
#include "adBms6830GenericType.h"
#include "adBms6830Data.h"
#include "mcu_wrapper.h"
#include "error_handler.h"

/*-----------------DEFINES------------------*/

/*------------EXTERN VARIABLES--------------*/
/*-------------LOCAL VARIABLES--------------*/

unsigned int sampleInterval = 0;

uint8_t cellVoltageRegs[64];
uint8_t cellTempRegs[64];


osThreadId_t bmsMainTask;
const osThreadAttr_t bmsMainTaskAttr = {
  .name = "bmsMainTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};


// Local function prototypes
int _current_loop(void);
int _voltage_loop(void);

void _parse_print_cell_measurement(uint8_t* buff);
void _parse_print_gpio_measurement(uint8_t* buff);
void _hex_dump(u8 * buff, int nBytes);

/*----------------FUNCITONS-----------------*/
int initBMSOps(int nICs){


	bmsMainTask = osThreadNew(bmsMainTaskFunc, NULL, bmsMainTaskAttr);

	if(bmsMainTask == NULL) return 1;

	return 0;
}


// @brief Parses the status register. The return value only contains the number of flags set. This function
// 		  loses information about which cell set the flag. Since the BMS cannot isolate or cutt off cells,
// 		  knowing which cell set a flag is not very useful.
// @param argStatReg Pointer to the array containing all measured status registers.
// @param size	Number of byte in the array
// @return Returns a uint32_t value where the top 16 bits contain number of overvoltage flags set, and
//		   and the bottom 16 bits contain the number of undervoltage flags set.
unsigned int _parseStatusRegister(uint8_t * argStatReg, size_t size){
	uint16_t retValHigh = 0;
	uint16_t retValLow = 0;
	// TODO: Check if ADBMS 2950 has the same position for flags in the status register
	for(int i = 0; i < size; i ++){ // Iterating through all the registers.
		if(i%4 || i%5){ // Only check every 4th and 5th register.
			for(int x = 0; x < 4; x ++){
				uint8_t checkVal = (argStatReg[i]>>(x*2) & 0b11) ; // Magic equation extracts high and low bits for a cell
				// Checking two bits at a time.
				// Bit 0 => UnderVoltage
				// Bit 1 => OverVoltage
				switch(checkVal){
				case 1:
					retValLow += 1; // Counts OverVoltage bits
					break;
				case 2:
					retValHigh += 1; // Counts UnderVoltage bits
					break;
				default:
					break;
				} // End switch statement
			} // End for loop
		} // End if statement
		else{
			pass;
		}
	}
	// I think it should record positional information. That way, we won't need to measure all cells
	// to see which one set the flag. I'm not sure how I would go about doing tha without using a
	// significant amount of memory.
	return (retValHigh<<16|retValLow); // Techincally, this can work for 16^2 cells.
}


void bmsMainTaskFunc(void * args){
	int counter = 0;
	configBMS();
	for(;;){

		if(1 == readStatErr()){
			counter == 10; // Force read cellVoltages.This is lossy.
		}
		if(counter >= 10){
			pollCellVoltage(cellVoltageRegs);
			pollAuxVoltage(cellTempRegs);
		}


		// Report findings
		osThreadYield(); // I think this lets other tasks run?
		osDelay(100);
	}
}


int inline setInterval(unsigned int argInterval){
	sampleInterval = argInterval;
	return sampleInterval;
}

unsigned int inline  getInterval(void){
	return sampleInterval;
}

