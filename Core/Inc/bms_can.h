/*
 * bms_can.h
 *
 *  Created on: Aug 23, 2024
 *      Author: amrlxyz
 */

#ifndef INC_BMS_CAN_H_
#define INC_BMS_CAN_H_

typedef struct
{
    uint32_t StdId;
    uint32_t DLC;
    uint8_t data[8];
} CAN_msg_s;


void CAN_rtosSetup(void);

osStatus_t putToCanTxQueue(uint32_t StdId, uint32_t DLC, uint8_t data[]);


#endif /* INC_BMS_CAN_H_ */
