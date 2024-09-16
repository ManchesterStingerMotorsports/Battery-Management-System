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
#include "can_request_handler.h"
/*-----------------DEFINES------------------*/

#define sendCAN(x)
#define MAXVOLTAGE 			600
#define MAXCURRENT 			100
#define OVER_VOLTAGE_MASK 	0xFFFF0000

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
	uint8_t chargerParameters[8] = {0};
	uint32_t errorState;

	// TODO: Add hardware checks here


	chargerParameters[0] = (uint8_t)(MAXVOLTAGE & 0xFF00) >> 8; // High byte
	chargerParameters[1] = (uint8_t)(MAXVOLTAGE & 0xFF);
	chargerParameters[2] = (uint8_t)(MAXCURRENT & 0xFF00) >> 8; // High byte
	chargerParameters[3] = (uint8_t)(MAXCURRENT & 0xFF);
	chargerParameters[4] = 0; // Enables charging
	chargerParameters[5] = 0;
	chargerParameters[6] = 0;
	chargerParameters[7] = 0;

	while(1){
		errorState = get_error();
		if(errorState & OVER_VOLTAGE_MASK){
			chargerParameters[4] = 1; // Disable charging

			//TODO: Disconnect terminals?

			sendCAN(chargerParameters); // TODO: Add a function to send this CAN message

			// if Charging is done or charger is disconnected, terminate this thread and continue the other thread
			osThreadTerminate(chargerComms);
		}
		else{
			sendCAN(chargerParameters);
			osDelay(1000); // Just make sure this is 1 second
		}

	}



}

/// @brief Initiates the the task that handles all charger communication. Suspends the normal CAN task to
/// 	   since it requires full control over the CAN bus to communicate with the charger.
int init_charger_task(){
	osThreadTerminate(canReqHandlerId);



	chargerComms = osThreadNew(_chargerCommTask, NULL, &oChargerCommsAttr);
	if(chargerComms == NULL) return -1;
	 return 0;

}
