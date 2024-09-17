/*
 * bms_rtosHelper.h
 *
 *  Created on: Aug 23, 2024
 *      Author: amrlxyz
 */

#ifndef INC_BMS_RTOSHELPER_H_
#define INC_BMS_RTOSHELPER_H_

#include "main.h"
#include "cmsis_os.h"


void rtosHelperSetup(void);

void printfMutex(const char * format, ...);

void uartTransmitMutex(const uint8_t *pData, uint16_t Size);


#endif /* INC_BMS_RTOSHELPER_H_ */
