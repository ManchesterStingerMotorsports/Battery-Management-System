/*
 * mcu_wrapper.h
 *
 *  Created on: Jan 28, 2024
 *      Author: malhaar-k
 */

#ifndef SRC_MCU_WRAPPER_H_
#define SRC_MCU_WRAPPER_H_

#define u16 				uint16_t
#define u8 					uint8_t
#define f32 				float
#define f64 				double



enum spi_num {
	ONE= 0,
	TWO= 1,
	THREE,
	FOUR,
	FIVE,
	SIX,
	SEVEN
};


int spi_write(u8 * tx_data, u16 size, enum spi_num SegNo);

int spi_read(u8 * rx_data, u16 size, enum spi_num SegNo);

int spi_read_write(u8 * tx_data, u8 * rx_data, u16 len , enum spi_num SegNo);

void wakeup_seg(enum spi_num SegNo);


#endif /* SRC_MCU_WRAPPER_H_ */
