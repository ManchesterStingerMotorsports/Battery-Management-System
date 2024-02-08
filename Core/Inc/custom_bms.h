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


int configBMS(void);

int requestCellVotlage(void);
int pollCellVoltage(u8*);
int requestAuxVoltage(void);
int pollAuxVoltage(u8*);




#endif /* INC_CUSTOM_BMS_H_ */
