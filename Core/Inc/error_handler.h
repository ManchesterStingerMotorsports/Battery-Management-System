/*
 * error_handler.h
 *
 *  Created on: Jul 9, 2024
 *      Author: malhaar-k
 */

#ifndef INC_ERROR_HANDLER_H_
#define INC_ERROR_HANDLER_H_

/*---------------------------INCLUDES--------------------------------------*/

#include "main.h"
#include "stdint.h"
#include "cmsis_os.h"

/*----------------------------DEFINES-------------------------------------*/


/*-----------------------------ENUMS--------------------------------------*/

enum fs_bms_error_t{
	NO_ERROR = 0,
	CELL_UV = 1, //cell undervoltage
	CELL_OV, //Cell overvoltage
	PACK_OC,// pack over current
	HIGH_TEMP, // high temperature
};

//TODO: List more components of the bms
enum fs_bms_component_t{
	NO_COMP = 0,
	CELL_V = 1,
	PACK_CURR = 2,
	TEMPERATURE  = 3,
};


/*---------------------------STRUCTS--------------------------------------*/
typedef struct _error_message{
	int error_val; // Largely unused right now.
	enum fs_bms_error_t error_type;
}fs_bms_error_msg;


/*----------------------FUNCTION PROTOTYPES-------------------------------*/

int init_error_handler(void);

void bms_error_handler_task(void*);

osStatus_t post_error_message(fs_bms_error_msg * );

void set_error(enum fs_bms_error_t);

uint32_t get_error(void);

void clear_error(enum fs_bms_error_t);



#endif /* INC_ERROR_HANDLER_H_ */
