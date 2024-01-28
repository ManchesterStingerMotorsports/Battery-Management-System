/*
 * mcu_wrapper.c
 *
 *  Created on: Jan 28, 2024
 *      Author: malhaar-k
 */

#include "mcu_wrapper.h"
#include "main.h"
#include "stdlib.h"
#include "stdio.h"


#define CS_PORT			GPIOD

#define WAKE_DELAY 		4
#define SPI_TIMEOUT 	10 // Unlikely to fail but might still


#define FORIN(x,y)         for(int x = 0; x< y; x++) //This is a lazy define that I like

// Extern Variables
extern SPI_HandleTypeDef hspi1;




int spi_write(u8 * tx_data, u16 size, enum spi_num SegNo){
	u16 pin_num = 0x1<<SegNo;
	HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_SET);
	if(HAL_SPI_Transmit(&hspi1, tx_data, size, SPI_TIMEOUT) != HAL_OK){
		HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_RESET);
		return -1;
	}
	HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_RESET);
	return 0;
}

int spi_read(u8 * rx_data, u16 size, enum spi_num SegNo){
	u16 pin_num = 0x1<<SegNo;
	HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_SET);
	if(HAL_SPI_Receive(&hspi1,rx_data, size, SPI_TIMEOUT) != HAL_OK){
		HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_RESET);
		return -1;
	}
	HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_RESET);
	return 0;
}

int spi_read_write(u8 * tx_data, u8 * rx_data, u16 len , enum spi_num SegNo){
	u16 pin_num = 0x1<<SegNo;
	HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_SET);
	HAL_SPI_Transmit(&hspi1, tx_data, len, SPI_TIMEOUT);
	if(HAL_SPI_Receive(&hspi1,rx_data, len, SPI_TIMEOUT) != HAL_OK){
			HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_RESET);
			return -1;
		}
	HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_RESET);
	return 0;

}


void wakeup_seg(enum spi_num SegNo, u8 num_ic){
	u16 pin_num = 0x1<<SegNo;
	FORIN(_x, num_ic){
	HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_RESET);
	HAL_Delay(WAKE_DELAY);
	HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_SET);
	HAL_Delay(WAKE_DELAY);
	}

}

