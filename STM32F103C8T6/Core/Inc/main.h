/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
extern SPI_HandleTypeDef hspi1;
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define FET0_Pin GPIO_PIN_1
#define FET0_GPIO_Port GPIOA
#define FET1_Pin GPIO_PIN_2
#define FET1_GPIO_Port GPIOA
#define FET2_Pin GPIO_PIN_3
#define FET2_GPIO_Port GPIOA
#define FET3_Pin GPIO_PIN_4
#define FET3_GPIO_Port GPIOA
#define FET4_Pin GPIO_PIN_5
#define FET4_GPIO_Port GPIOA
#define FET5_Pin GPIO_PIN_6
#define FET5_GPIO_Port GPIOA
#define OE_Pin GPIO_PIN_12
#define OE_GPIO_Port GPIOA
#define LATCH_Pin GPIO_PIN_15
#define LATCH_GPIO_Port GPIOA
#define SCK_Pin GPIO_PIN_3
#define SCK_GPIO_Port GPIOB
#define MOSI_Pin GPIO_PIN_5
#define MOSI_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
