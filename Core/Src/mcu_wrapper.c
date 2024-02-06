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


#define CS_PORT			GPIOB
#define CS_PIN 			GPIO_PIN_1

#define WAKE_DELAY 		4
#define SPI_TIMEOUT 	100 // Unlikely to fail but might still



// Extern Variables
extern SPI_HandleTypeDef hspi1;




int spi_write(u8 * tx_data, u16 size){
	u16 pin_num = CS_PIN; // Lazy refactor
	HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_RESET);
	if(HAL_SPI_Transmit(&hspi1, tx_data, size, SPI_TIMEOUT) != HAL_OK){
		HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_SET);
		return -1;
	}
	HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_SET);
	return 0;
}

int spi_read(u8 * rx_data, u16 size){
	u16 pin_num = CS_PIN; // Lazy refactor
	HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_RESET);
	if(HAL_SPI_Receive(&hspi1,rx_data, size, SPI_TIMEOUT) != HAL_OK){
		HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_SET);
		printf("\n SPI COMM Failed");
		return -1;
	}
	printf("\n");
	FORIN(x, size){
		printf("%x ", rx_data[x]);
	}
	HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_SET);
	return 0;
}

int spi_read_write(u8 * tx_data, u8 * rx_data, u16 len ){
	u16 pin_num = CS_PIN; // Lazy refactor
	HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi1, tx_data, len, SPI_TIMEOUT);
	if(HAL_SPI_Receive(&hspi1,rx_data, len, SPI_TIMEOUT) != HAL_OK){
			HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_SET);
			return -1;
		}
	HAL_GPIO_WritePin(CS_PORT, pin_num, GPIO_PIN_SET);
	return 0;

}


void wakeup_chain(u8 num_ic){
	FORIN(_x, num_ic){
	HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
	HAL_Delay(WAKE_DELAY);
	HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
	HAL_Delay(WAKE_DELAY);
	}

}

