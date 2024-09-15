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

#define EPSILON						0.02f
#define TEMP_THRESH					1000  // TODO: Change later. Make configurable too


/*------------EXTERN VARIABLES--------------*/
/*-------------LOCAL VARIABLES--------------*/

unsigned int sampleInterval = 0;

//uint8_t cellVoltageRegs[64];
//uint8_t cellTempRegs[64];


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

///
void _bmsMainTaskFunc(void * args){
	int counter = 0;
	uint8_t storeStatRegs[REG_SIZE_BYTES];
	statErr_t statusFlags[TOTAL_IC];
	uint16_t cellVoltages[nCELLS];
	uint16_t cellTemperatures[nTEMPS];
	fs_bms_error_msg batteryErrors;
	int returnVals;

	uint32_t tickOne, tickTwo, passedTime;
	uint32_t tickFreq = osKernelGetTickFreq();
	configBMS(); // TODO: Fix this first.
	for(;;){

		returnVals = readStatErr(storeStatRegs, REG_SIZE_BYTES, statusFlags, TOTAL_IC); // FINAL IMPLEMENTATION DONE

		batteryErrors.error_type = returnVals;

		if(returnVals){
			// Keep trying to post error message.
			while(post_error_message(&batteryErrors) != osOK);
		}
		returnVals = 0;
		if(counter >= 10){ // TODO: Make configurable
			pollCellVoltage(cellVoltages); // Size is decided at compile time
			pollAuxVoltage(cellTemperatures);
		}

		for(int x = 0; x < nTEMPS; x++){
			returnVals += ( cellTemperatures[x] >  TEMP_THRESH ) ? 1 : 0 ; // Counts how many values are above threshold
		}

		if(returnVals){
			batteryErrors.error_val = returnVals;
			batteryErrors.error_type = HIGH_TEMP;
			while(post_error_message(&batteryErrors) != osOK); // High temp error posting
		}
		// Report findings
		tickOne = osKernelGetTickCount();
		osThreadYield(); // I think this lets other tasks run?
		tickTwo = osKernelGetTickCount();
		passedTime = (tickOne - tickTwo)*(1000/tickFreq);

		// This is an attempt to ensure that the battery is checked consistently.
		passedTime>=100? osDelay(1) : osDelay(100 - passedTime);
		// 100 - elapsed_ms = 100 - (endTime - startTime)*(1000/osKernelGetTickFrequency)
	}
}

int init_bms_ops(int nICs){


	bmsMainTask = osThreadNew(_bmsMainTaskFunc, NULL, &bmsMainTaskAttr);

	if(bmsMainTask == NULL) return 1;

	return 0;
}

int inline setInterval(unsigned int argInterval){
	sampleInterval = argInterval;
	return sampleInterval;
}

unsigned int inline  getInterval(void){
	return sampleInterval;
}

