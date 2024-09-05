/*
 * error_handler.c
 *
 *  Created on: Jul 9, 2024
 *      Author: malhaar-k
 */

#include "error_handler.h"

// INCLUDES
#include "cmsis_os.h"
#include <stdint.h>
#include <string.h>

//DEFINES
#define BMS_ID 				0x60
#define ERROR_QUEUE_SIZE 	10

//EXTERN VARIABLES
//LOCAL VARIABLES
uint8_t bmsCANErrorMsg[8];

uint32_t bmsErrorState; // Each bit represents different type of error

osThreadId_t bmsErrorTaskHandle;

const osThreadAttr_t bmsErrorTask_attributes = {
  .name = "bmsErrorTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};


//FUNCTIONS

int init_error_handler(void){
	bmsErrorState = 0;
	memset(bmsCANErrorMsg,0x00, 8);
	bmsCANErrorMsg[0] = 'B';
	bmsCANErrorMsg[0] = 'M';
	bmsCANErrorMsg[0] = 'S';
	bmsCANErrorMsg[0] = 'E';

	// Create message queue for error messages
	oBMSErrorMsgQueue = osMessageQueueNew(ERROR_QUEUE_SIZE, sizeof(fs_bms_error_msg), NULL);
	if(oBMSErrorMsgQueue == NULL) return 1;

	// Create error handler task
	bmsErrorTaskHandle = osThreadNew(bms_error_handler_task, NULL, &bmsErrorTask_attributes);
	if(bmsErrorTaskHandle == NULL) return 1;

	return 0;
}

void bms_error_handler_task(void*){

	while(1){
		uint32_t errorCount = osMessageQueueGetCount(oBMSErrorMsgQueue);

		// All messages checked if they exist
		for(int msg = 0; msg < errorCount; msg ++){
			fs_bms_error_msg errMsg;
			osMessageQueueGet(oBMSErrorMsgQueue, &errMsg, NULL, 1); // Won't need delays since it checks if there are messages
			if(errMsg.error_val == 0){
				clear_error(errMsg.error_type);
			}
			else {
				set_error(errMsg.error_type);
			}
		}

		if(bmsErrorState){
		// Add OS Timers later so it doesn't just send it
			if(osMutexAcquire(oCANMutex, osWaitForever) == osOK){
				memcpy(bmsCANErrorMsg + 4, &bmsErrorState, 4);
//				send_CAN_buffer(BMS_ID, bmsCANErrorMsg); // ADD LATER

				osMutexRelease(oCANMutex); // RELEASE THE MUTEX. IT BLOCKS ALL CAN IF YOU DON'T
				osDelay(1000); // If errors exist, they only get transmitted once every second
			}
		}
	} // While loop end

}

void set_error(enum fs_bms_error_t err_num){
	bmsErrorState |= (1<<(int)err_num);
}

uint32_t get_error(void){
	return bmsErrorState;
}

void clear_error(enum fs_bms_error_t err_num){;
	bmsErrorState &= ~(1<<(int)err_num);
}



