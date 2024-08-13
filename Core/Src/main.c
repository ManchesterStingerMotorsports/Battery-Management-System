/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <string.h>
#include "custom_bms.h"
#include "adBms6830GenericType.h"
#include "adBms6830Data.h"
#include "mcu_wrapper.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define SHUNT 						50
#define C_VOLT_CONV(v)				((v + 10000) * 0.000150)
/*
 This formula is really weird. It gives us the reading in minivolts.
 The key is that >> and << are arithmetic shifts and >>> and <<< are logical shifts.
 The reason why we left shift, type cast to int32 and the right shift again is because 2950
 stores the value as a signed 18 bit number with the 17th bit(0 indexing) being the sign bit.
 By left shifting the value by [32 - 18 = 14], and type casting it to an int32, we put the
 17th bit as the 31st bit. Now if we arithmetic right shift by 14, the 31st bit is not shifted
 and it maintains its polarity.
 */
#define AMP_CONV(v)					1e-6*((int32_t)((v) << 14)>>14); // Needs to give us Amps.

// old formula (v*0.00015) + 1.5
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart5;
UART_HandleTypeDef huart2;

/* Definitions for isoSPIcomm */
osThreadId_t isoSPIcommHandle;
const osThreadAttr_t isoSPIcomm_attributes = {
        .name = "isoSPIcomm",
        .stack_size = 128 * 4,
        .priority = (osPriority_t) osPriorityNormal, };
/* Definitions for canHandler */
osThreadId_t canHandlerHandle;
const osThreadAttr_t canHandler_attributes = {
        .name = "canHandler",
        .stack_size = 128 * 4,
        .priority = (osPriority_t) osPriorityNormal, };
/* Definitions for errorHandler */
osThreadId_t errorHandlerHandle;
const osThreadAttr_t errorHandler_attributes = {
        .name = "errorHandler",
        .stack_size = 128 * 4,
        .priority = (osPriority_t) osPriorityLow, };
/* Definitions for canTxQueue */
osMessageQueueId_t canTxQueueHandle;
const osMessageQueueAttr_t canTxQueue_attributes = {.name = "canTxQueue"};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN1_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
static void MX_UART5_Init(void);
static void MX_USART2_UART_Init(void);
void startIsoSPIcomm(void *argument);
void startCanHandler(void *argument);
void startErrorHandler(void *argument);

/* USER CODE BEGIN PFP */
int __io_putchar(int ch); // Printf works on uart4. Usart1 pins are used for something else on DISCO Board

int current_loop(void);
int voltage_loop(void);

void parse_print_cell_measurement(uint8_t *buff);
void parse_print_gpio_measurement(uint8_t *buff);
void hex_dump(u8 *buff, int nBytes);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_CAN1_Init();
    MX_SPI1_Init();
    MX_TIM2_Init();
    MX_UART5_Init();
    MX_USART2_UART_Init();
    /* USER CODE BEGIN 2 */

    /* USER CODE END 2 */

    /* Init scheduler */
    osKernelInitialize();

    /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
    /* USER CODE END RTOS_MUTEX */

    /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
    /* USER CODE END RTOS_SEMAPHORES */

    /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
    /* USER CODE END RTOS_TIMERS */

    /* Create the queue(s) */
    /* creation of canTxQueue */
    canTxQueueHandle = osMessageQueueNew(16, sizeof(uint16_t),
                                         &canTxQueue_attributes);

    /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
    /* USER CODE END RTOS_QUEUES */

    /* Create the thread(s) */
    /* creation of isoSPIcomm */
    isoSPIcommHandle = osThreadNew(startIsoSPIcomm, NULL,
                                   &isoSPIcomm_attributes);

    /* creation of canHandler */
    canHandlerHandle = osThreadNew(startCanHandler, NULL,
                                   &canHandler_attributes);

    /* creation of errorHandler */
    errorHandlerHandle = osThreadNew(startErrorHandler, NULL,
                                     &errorHandler_attributes);

    /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
    if (init_error_handler())
        Error_Handler();

    /* USER CODE END RTOS_THREADS */

    /* USER CODE BEGIN RTOS_EVENTS */
    /* add events, ... */
    /* USER CODE END RTOS_EVENTS */

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    configBMS();
    while (1)
    {

//	  readSID();
//	  configBMS();

        readStatErr();

//	  readCFG();
        voltage_loop();

        HAL_Delay(1500);
//	  printf("\n\rALIVE \n\r");
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 64;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
            | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV16;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief CAN1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_CAN1_Init(void)
{

    /* USER CODE BEGIN CAN1_Init 0 */

    /* USER CODE END CAN1_Init 0 */

    /* USER CODE BEGIN CAN1_Init 1 */

    /* USER CODE END CAN1_Init 1 */
    hcan1.Instance = CAN1;
    hcan1.Init.Prescaler = 16;
    hcan1.Init.Mode = CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan1.Init.TimeSeg1 = CAN_BS1_1TQ;
    hcan1.Init.TimeSeg2 = CAN_BS2_1TQ;
    hcan1.Init.TimeTriggeredMode = DISABLE;
    hcan1.Init.AutoBusOff = DISABLE;
    hcan1.Init.AutoWakeUp = DISABLE;
    hcan1.Init.AutoRetransmission = DISABLE;
    hcan1.Init.ReceiveFifoLocked = DISABLE;
    hcan1.Init.TransmitFifoPriority = DISABLE;
    if (HAL_CAN_Init(&hcan1) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN CAN1_Init 2 */

    /* USER CODE END CAN1_Init 2 */

}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void)
{

    /* USER CODE BEGIN SPI1_Init 0 */

    /* USER CODE END SPI1_Init 0 */

    /* USER CODE BEGIN SPI1_Init 1 */

    /* USER CODE END SPI1_Init 1 */
    /* SPI1 parameter configuration*/
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN SPI1_Init 2 */

    /* USER CODE END SPI1_Init 2 */

}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void)
{

    /* USER CODE BEGIN TIM2_Init 0 */

    /* USER CODE END TIM2_Init 0 */

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /* USER CODE BEGIN TIM2_Init 1 */

    /* USER CODE END TIM2_Init 1 */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 4294967295;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM2_Init 2 */

    /* USER CODE END TIM2_Init 2 */

}

/**
 * @brief UART5 Initialization Function
 * @param None
 * @retval None
 */
static void MX_UART5_Init(void)
{

    /* USER CODE BEGIN UART5_Init 0 */

    /* USER CODE END UART5_Init 0 */

    /* USER CODE BEGIN UART5_Init 1 */

    /* USER CODE END UART5_Init 1 */
    huart5.Instance = UART5;
    huart5.Init.BaudRate = 115200;
    huart5.Init.WordLength = UART_WORDLENGTH_8B;
    huart5.Init.StopBits = UART_STOPBITS_1;
    huart5.Init.Parity = UART_PARITY_NONE;
    huart5.Init.Mode = UART_MODE_TX_RX;
    huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart5.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart5) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN UART5_Init 2 */

    /* USER CODE END UART5_Init 2 */

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{

    /* USER CODE BEGIN USART2_Init 0 */

    /* USER CODE END USART2_Init 0 */

    /* USER CODE BEGIN USART2_Init 1 */

    /* USER CODE END USART2_Init 1 */
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN USART2_Init 2 */

    /* USER CODE END USART2_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* USER CODE BEGIN MX_GPIO_Init_1 */
    /* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2,
                      GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);

    /*Configure GPIO pins : PB0 PB1 PB2 */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : PD12 */
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* USER CODE BEGIN MX_GPIO_Init_2 */
    /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
int current_loop(void)
{
    /*
     * Current measurement from 2950
     */
    return 0;
}

int can_loop(void)
{
    /*
     * Handle comm with aux computer
     */

    return 0;
}

int voltage_loop(void)
{
//	wakeup_chain(TOTAL_IC);
    u8 cellVReg[32 * TOTAL_IC];		//RDCVALL Size .. Just padding
    u8 auxVReg[24 * TOTAL_IC];			// Total Aux2 reg size + 2 for padding
    memset(cellVReg, 0x00, 32 * TOTAL_IC);
    memset(auxVReg, 0x00, 24 * TOTAL_IC);
    uint32_t timeStamp = 0;
    uint32_t timeStamp2 = 0;

    /*
     * Tasks:
     * 1. Measure cell voltages
     * 2. Measure cell temperatures
     */

    timeStamp = HAL_GetTick();
    printf("\n\rPEC: %d", pollCellVoltage(cellVReg));
    timeStamp2 = HAL_GetTick() - timeStamp;
    printf("\n\rCell Mes Time stamp: %lu ", timeStamp2);
    printf("\n \r CELL VOLTAGE \n\r");
    parse_print_cell_measurement(cellVReg);

//	hex_dump(cellVReg, 32*TOTAL_IC);

    timeStamp = HAL_GetTick();
    printf("PEC: %d", pollAuxVoltage(auxVReg));
    printf("\n\rGPIO Time stamp: %lu ", HAL_GetTick() - timeStamp);
//	parse_print_gpio_measurement(auxVReg);
    printf("\n \r AUX VALUES \n\r");
//	hex_dump(auxVReg, 24*TOTAL_IC);

    return 0;
}

/*
 * @brief Sends config data at startup
 */

// Just printing values for now
//void parse_print_cell_measurement(uint8_t* buff){
//	int16_t cell_values[16*TOTAL_IC];
//	memset(cell_values, 0x0000, 16*TOTAL_IC*sizeof(uint16_t));
//
//		FORIN(x, 16*(TOTAL_IC-1)){
//		float temp = 0.0;
//		cell_values[x] = buff[2*x]|buff[2*x + 1]<<8; // Not using memcpy because I am not sure about endianness
//		temp = C_VOLT_CONV(cell_values[x]);
//
//		printf("%.02f  ", temp);
////		printf("%d  ", cell_values[x]);
//		int __x = (x+1)%8? 0:  printf("\n\r"); // Hacky Prints a new line only every eight values
//	}
//	double Val2950[2] = {0, 0};
//	Val2950[0] = AMP_CONV(buff[36]|buff[37]<<8|buff[38]<<16);
//	Val2950[1] = AMP_CONV(buff[39]|buff[40]<<8|buff[41]<<16);
//
//	printf("\n\r Current Values: %.02lf %.02lf \n\r", Val2950[0], Val2950[1]);
//
//
//}
// Dirty parsing function. ONLY FOR TESTING
// Final parsing needs to be more intelligent
void parse_print_cell_measurement(uint8_t *buff)
{
    double curr_1 = AMP_CONV(buff[2] << 16 | buff[1] << 8 | buff[0])
    ;
    double curr_2 = AMP_CONV(buff[5] << 16 | buff[4] << 8 | buff[3])
    ;

    int16_t cell_0 = buff[6] | buff[7] << 8;
    int16_t cell_1 = buff[8] | buff[9] << 8;
    int16_t cell_2 = buff[10] | buff[11] << 8;

    printf("\n\r Cell measurements: %.02f %.02f %.02f", C_VOLT_CONV(cell_0),
           C_VOLT_CONV(cell_1), C_VOLT_CONV(cell_2));
    printf("\n\r Current measurements: %.06f %.06f \n\r", curr_1 / 0.00005,
           curr_2 / 0.00005);

}

void parse_print_gpio_measurement(uint8_t *buff)
{
    int16_t gpio_values[20 * TOTAL_IC];
    float temp = 0.0;
    printf("\n\rGPIO VALUES: \n\r"); // Remove later. Only prints in debug mode.
    FORIN(x, 10*TOTAL_IC)
    {
        gpio_values[x] = buff[2 * x] | buff[2 * x + 1] << 8; // Not using memcpy because I am not sure about endianness
        temp = C_VOLT_CONV(gpio_values[x]);

        printf("%.02f  ", temp);
        printf("%d  ", gpio_values[x]);
        int __x = (x + 1) % 8 ? 0 : printf("\n\r"); // Hacky// Prints a new line only every eight values
    }

}

void hex_dump(u8 *buff, int nBytes)
{
    FORIN(i, nBytes)
    {
        printf("%02x ", buff[i]);
        int __x = (i + 1) % 6 ? 0 : printf("\n\r"); // Hacky// Prints a new line only every eight values
    }
}

/*
 * Not using itm since it needs debug mode to run and isoSPI needs
 */
int __io_putchar(int ch)
{

//		ITM_SendChar(ch);
    HAL_UART_Transmit(&huart2, (uint8_t*) &ch, 1, HAL_MAX_DELAY);
    return (ch);
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_startIsoSPIcomm */
/**
 * @brief Function implementing the isoSPIcomm thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_startIsoSPIcomm */
void startIsoSPIcomm(void *argument)
{
    /* USER CODE BEGIN 5 */
    /* Infinite loop */
    for (;;)
    {
        osDelay(1);
    }
    /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_startCanHandler */
/**
 * @brief Function implementing the canHandler thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_startCanHandler */
void startCanHandler(void *argument)
{
    /* USER CODE BEGIN startCanHandler */
    /* Infinite loop */
    for (;;)
    {
        osDelay(1);
    }
    /* USER CODE END startCanHandler */
}

/* USER CODE BEGIN Header_startErrorHandler */
/**
 * @brief Function implementing the errorHandler thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_startErrorHandler */
void startErrorHandler(void *argument)
{
    /* USER CODE BEGIN startErrorHandler */
    /* Infinite loop */
    for (;;)
    {
        osDelay(1);
    }
    /* USER CODE END startErrorHandler */
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM6 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    /* USER CODE BEGIN Callback 0 */

    /* USER CODE END Callback 0 */
    if (htim->Instance == TIM6)
    {
        HAL_IncTick();
    }
    /* USER CODE BEGIN Callback 1 */

    /* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
