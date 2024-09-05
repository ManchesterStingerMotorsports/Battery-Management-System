/*
 * custom_6830.h
 *
 *  Created on: Feb 8, 2024
 *      Author: malhaar-k
 */

#ifndef INC_CUSTOM_BMS_H_
#define INC_CUSTOM_BMS_H_

#include "main.h"
#include "mcu_wrapper.h"

#define statErr_t 	uint32_t // Just didn't want it to be the same signature.


int configBMS(void);

int pollCellVoltage(u8*);

int pollAuxVoltage(u8*);

int readCFG(void);

int readSID(void);

int readStatErr(void);




#endif /* INC_CUSTOM_BMS_H_ */
