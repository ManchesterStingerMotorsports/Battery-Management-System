/*
 * bms_rtosHelper.c
 *
 *  Created on: Aug 23, 2024
 *      Author: amrlxyz
 */

//#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "main.h"
#include "cmsis_os.h"
#include "bms_rtosHelper.h"

#define UART_handle huart3
#define UART_mutexHandle mutex_uart3Handle


/* Definitions for mutex_uart3 */
osMutexId_t mutex_uart3Handle;
const osMutexAttr_t mutex_uart3_attributes = {
  .name = "mutex_uart3"
};


void rtosHelperSetup(void)
{
    mutex_uart3Handle = osMutexNew(&mutex_uart3_attributes);
    if (mutex_uart3Handle == NULL)
    {
        Error_Handler();
    }
}


void printfMutex(const char *format, ...)
{
    char buffer[64];

    va_list args;
    va_start(args, format);
    int length = vsprintf(buffer, format, args);
    va_end(args);

    osMutexAcquire(UART_mutexHandle, osWaitForever);
    if (HAL_UART_Transmit(&UART_handle, (uint8_t*) buffer, (uint16_t) length, HAL_MAX_DELAY) != HAL_OK)
    {
        Error_Handler();
    }
    osMutexRelease(UART_mutexHandle);
}


void uartTransmitMutex(const uint8_t *pData, uint16_t Size)
{
    osMutexAcquire(UART_mutexHandle, osWaitForever);
    if (HAL_UART_Transmit(&UART_handle, pData, Size, HAL_MAX_DELAY) != HAL_OK)
    {
        Error_Handler();
    }
    osMutexRelease(UART_mutexHandle);
}

