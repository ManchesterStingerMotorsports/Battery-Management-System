/*
 * can_request_handler.c
 *
 *  Created on: Jul 11, 2024
 *      Author: malhaar-k
 */

#include "can_request_handler.h"
#include "cmsis_os.h"
#include "main.h"
#include <stdint.h>
/*----------------INCLUDES------------------*/

/*-----------------DEFINES------------------*/
#define CHECK_NULL(x)  						if(x==NULL)return 1; //usefull macro

/*------------EXTERN VARIABLES--------------*/
/*-------------LOCAL VARIABLES--------------*/
volatile canReqType requestType = NONE;
volatile int canSendFlag =0;

osEventFlagsId_t canReqEventId;



osThreadId_t canReqHandlerId;
osThreadAttr_t canReqHandlerAttr ={
		.name = "CAN Handler",
		.priority = (osPriority_t) osPriorityNormal,
		.stack_size = 128*4

};
/*----------------FUNCITONS-----------------*/

int init_can_request_handler(void){
	canReqEventId = osEventFlagsNew(NULL);
	CHECK_NULL(canReqEventId);


	canReqHandlerId = osThreadNew(can_request_handler_task, NULL, &canReqHandlerAttr);
	CHECK_NULL(canReqHandlerId);

	return 0;
}

void can_request_handler_task(void*){

	for(;;){
		osEventFlagsWait(canReqEventId, 0x01U, NULL, 0);
		switch(requestType){
			case CELL_VAL:
				// Get cell values and write to buffer CAN


				break;
			case OV_FLAGS:
				break;
			case UV_FLAGS:
				break;
			case TEMP_VAL:
				break;
			case PACK_VALUES:
				break;
			default:
				// Handle unknown request
				break;
			}

	}
}


// Not sure what to do with this function other than just setting flags or smth
void parse_received_CAN_request(uint8_t * data,uint8_t length, uint16_t rcvid){
	requestType = *data; // First byte will holde the type of data

	osEventFlagsSet(canReqEventId, 0x01U);

}

