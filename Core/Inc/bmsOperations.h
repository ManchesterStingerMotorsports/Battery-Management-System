/*
 * bmsOperations.h
 *
 *  Created on: Aug 23, 2024
 *      Author: malhaar-k
 */

#ifndef INC_BMSOPERATIONS_H_
#define INC_BMSOPERATIONS_H_





/*----------------INCLUDES------------------*/
/*-----------------DEFINES------------------*/


/*------------------ENUMS-------------------*/
/*-----------------STRUCTS------------------*/
/*-----------FUNCITON PROTOTYPES------------*/

int init_bms_ops(int);


/// @brief Sets the time between two status register reads.
int setInterval(unsigned int);

///
unsigned int getInterval(void);


#endif /* INC_BMSOPERATIONS_H_ */
