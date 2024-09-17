/*
 * bms_can.c
 *
 *  Created on: Aug 23, 2024
 *      Author: amrlxyz
 */

#include <string.h>
#include <stdio.h>
#include "main.h"
#include "cmsis_os.h"
#include "bms_can.h"
#include "bms_rtosHelper.h"


#define CAN_handle hcan1
#define CAN_txQueueHandle canTxQueueHandle

//////// ------------------------ RTOS SETUP ----------------------------- ////////////

/* Definitions for canTransmit */
osThreadId_t canTransmitHandle;
const osThreadAttr_t canTransmit_attributes = {
  .name = "canTransmit",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Definitions for canTxQueue */
osMessageQueueId_t canTxQueueHandle;
const osMessageQueueAttr_t canTxQueue_attributes = {
  .name = "canTxQueue"
};

/* Definitions for canGenerator */
osThreadId_t canGeneratorHandle;
const osThreadAttr_t canGenerator_attributes = {
  .name = "canGenerator",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};


void startCanTransmit(void *argument);
void startCanGenerator(void *argument);


void CAN_rtosSetup(void)
{
    canTxQueueHandle = osMessageQueueNew (8, sizeof(CAN_msg_s), &canTxQueue_attributes);
    if (canTxQueueHandle == NULL)
    {
        Error_Handler();
    }

    canTransmitHandle = osThreadNew(startCanTransmit, NULL, &canTransmit_attributes);
    if (canTransmitHandle == NULL)
    {
        Error_Handler();
    }

    // Used to test can Tx - uncomment to test can
//    canGeneratorHandle = osThreadNew(startCanGenerator, NULL, &canGenerator_attributes);
//    if (canGeneratorHandle == NULL)
//    {
//        Error_Handler();
//    }
}



//////// ------------------------ FUNCTIONS ----------------------------- ////////////

// Prototypes
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);
void CAN_uartHexDump(CAN_RxHeaderTypeDef *rxHeader, uint8_t rxData[]);
osStatus_t putToCanTxQueue(uint32_t StdId, uint32_t DLC, uint8_t data[]);
void CAN_setup(void);


// ISR Callback to process recieved can messages:
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rxHeader;
    uint8_t rxData[8];

    // Get CAN Message
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData);

    // Process recieved can messages here <---------------------------------------------------
    // Parse incoming message bla2

    CAN_uartHexDump(&rxHeader, rxData); // CAUTION: CAUSES ERROR SINCE MUTEX ACQ CANNOT BE CALLED FROM ISR
}


void CAN_uartHexDump(CAN_RxHeaderTypeDef *rxHeader, uint8_t rxData[])
{
    char txBuff[100] = {0};

    // Format string to display hex for uart
    sprintf(txBuff, "ID: 0x%03X, DATA: ",
            (uint16_t) rxHeader->StdId);
    int index = strlen(txBuff);
    for (int i = 0; i < rxHeader->DLC; i++)
    {
        sprintf((txBuff + index + 3 * i), "%02X ", rxData[i]);
    }
    *(txBuff + index + 3 * rxHeader->DLC) = '\n';

//    // Send buffer to uart with mutex protection
    uartTransmitMutex((uint8_t*) txBuff, index + 3 * rxHeader->DLC + 1);

    // Bypasses Mutex protection (Will still crash):
//    if (HAL_UART_Transmit(&huart3, (uint8_t*) txBuff, index + 3 * rxHeader->DLC + 1, HAL_MAX_DELAY) != HAL_OK)
//    {
//        Error_Handler();
//    }
}


osStatus_t putToCanTxQueue(uint32_t StdId, uint32_t DLC, uint8_t data[])
{
    CAN_msg_s canMsg;
    canMsg.StdId = StdId;
    canMsg.DLC = DLC;
    memcpy(canMsg.data, data, DLC);

    if (osMessageQueuePut(CAN_txQueueHandle, &canMsg, 0, 0) != osOK)
    {
//        HAL_UART_Transmit(&UART_handle, (uint8_t*) "PutQError\n", 10, HAL_MAX_DELAY);
        return osError;
    }
    return osOK;
}


void CAN_setup(void)
{
    // This filter allows for all message to pass
    // Rx FIFO0 is used
    CAN_FilterTypeDef sf;
    sf.FilterIdHigh = 0x0000;
    sf.FilterIdLow = 0x0000;
    sf.FilterMaskIdHigh = 0x0000;
    sf.FilterMaskIdLow = 0x0000;
    sf.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sf.FilterBank = 0;
    sf.FilterMode = CAN_FILTERMODE_IDMASK;
    sf.FilterScale = CAN_FILTERSCALE_32BIT;
    sf.FilterActivation = CAN_FILTER_ENABLE;

    if (HAL_CAN_ConfigFilter(&CAN_handle, &sf) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_CAN_Start(&CAN_handle) != HAL_OK)
    {
        Error_Handler();
    }

    // Enable Interrupt callback when a CAN message is recieved
    if (HAL_CAN_ActivateNotification(&CAN_handle, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        Error_Handler();
    }

}


//////// ------------------------ TASKS ----------------------------- ////////////

void startCanTransmit(void *argument)
{
    CAN_setup();
    CAN_msg_s canMsg;

    for (;;)
    {
        // Check for empty TxMailbox (3 Tx Mailbox per CAN)
        if (HAL_CAN_GetTxMailboxesFreeLevel(&CAN_handle))
        {
            // If mailbox is empty wait forever for a new message from the queue
            if (osMessageQueueGet(CAN_txQueueHandle, &canMsg, NULL, osWaitForever) == osOK)
            {
                CAN_TxHeaderTypeDef txHeader;

                txHeader.StdId = canMsg.StdId;       // ID - Lower ID - higher priority
                txHeader.IDE = CAN_ID_STD;       // Extended 2.0B or Standard 2.0A
                txHeader.RTR = CAN_RTR_DATA;     // Remote or Data Frame
                txHeader.DLC = canMsg.DLC;      // Data Length Code

                uint32_t mb;                // Stores which Tx Mailbox is used

                if (HAL_CAN_AddTxMessage(&CAN_handle, &txHeader, canMsg.data, &mb) != HAL_OK)
                {
                    // CAN peripheral tx error
                    Error_Handler();
                }
            }
            else
            {
                // Get Queue Error
            }
        }
        else
        {
            // TODO: Error Detection if Mailbox keeps being full (TxMailbox is full)
            printfMutex("Err: TxMailbox Full \n");
            osDelay(50);
        }
    }
}

void startCanGenerator(void *argument)
{
  /* USER CODE BEGIN startCanGenerator */
    /* Infinite loop */
    int loopCount = 0;

    for (;;)
    {
//        printfMutex("sending can data ***************** \n");
        static uint8_t data[] = {2, 1};
        putToCanTxQueue(loopCount, 2, data);

        if (loopCount++ >= 250)
        {
            loopCount = 0;
        }

        osDelay(100);

    }
  /* USER CODE END startCanGenerator */
}

