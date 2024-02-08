/*
 * mcu_wrapper.h
 *
 *  Created on: Jan 28, 2024
 *      Author: malhaar-k
 */

#ifndef SRC_MCU_WRAPPER_H_
#define SRC_MCU_WRAPPER_H_
#include <stdint.h>

#define u16 				uint16_t
#define u8 					uint8_t
#define f32 				float
#define f64 				double


#define FORIN(x,y)         for(int x = 0; x< y; x++) //This is a lazy define that I like



int spi_write(u8 * tx_data, u16 size);

int spi_read(u8 * rx_data, u16 size);

int spi_read_write(u8 * tx_data, u8 * rx_data, u16 len);

void wakeup_chain(u8);


void spiCSHigh(void);

void spiCSLow(void);



#endif /* SRC_MCU_WRAPPER_H_ */
