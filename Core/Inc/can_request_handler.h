/*
 * can_request_handler.h
 *
 *  Created on: Jul 11, 2024
 *      Author: malhaar-k
 */

#ifndef INC_CAN_REQUEST_HANDLER_H_
#define INC_CAN_REQUEST_HANDLER_H_

/*----------------INCLUDES------------------*/
#include "main.h"
#include "cmsis_os.h"
#include <stdint.h>

/*-----------------DEFINES------------------*/
typedef enum _candatatype{
	NONE = 0,
	CELL_VAL = 0x0A,
	OV_FLAGS, // 0x0B
	UV_FLAGS,
	TEMP_VAL, // Pack temperature list
	PACK_VALUES,//0x0E // Current, Pack voltage, etc.

}canReqType;

/*------------------ENUMS-------------------*/
/*-----------------STRUCTS------------------*/
/*-----------FUNCITON PROTOTYPES------------*/
int init_can_request_handler(void);

void can_request_handler_task(void*);

void parse_received_CAN_request(uint8_t * data,uint8_t length, uint16_t rcvid);



#endif /* INC_CAN_REQUEST_HANDLER_H_ */
