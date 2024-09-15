/*
 * chargerComms.c
 *
 *  Created on: Sep 15, 2024
 *      Author: malhaar-k
 *
 *     Includes a the task that handles charger communication for the BMS
 */


/*----------------INCLUDES------------------*/

#include <stdint.h>
#include "error_handler.h"
#include "chargerComms.h"
#include "main.h"
#include "cmsis_os.h"
/*-----------------DEFINES------------------*/

/*------------EXTERN VARIABLES--------------*/
/*-------------LOCAL VARIABLES--------------*/
osThreadId_t chargerComms;
osThreadAttr_t oChargerCommsAttr = {
		.priority = (osPriority_t) osPriorityNormal,
		.stack_size = 1024,
		.name = "chargerCommTask"
};

/*----------------FUNCITONS-----------------*/

// TODO: Implemenet later
int _localCANTransmit(uint8_t * array){

	return 0;
}


/// @brief All CAN communication should be taken up by this task
void _chargerCommTask(void * args){


	// if Charging is done or charger is disconnected, terminate this thread and continue the other thread
	osThreadTerminate(chargerComms);
}

/// @brief Initiates the the task that handles all charger communication. Suspends the normal CAN task to
/// 	   since it requires full control over the CAN bus to communicate with the charger.
int init_charger_task(){
	osThreadTerminate(canReqHandlerId);


	chargerComms = osThreadNew(_chargerCommTask, NULL, oChargerCommsAttr);
	if(chargerComms == NULL) return -1;
	 return 0;

}
