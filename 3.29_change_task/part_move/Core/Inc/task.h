/*
 * task.h
 *
 * Description: Competitor use this file for tasks
 */

#ifndef INC_TASK_H_
#define INC_TASK_H_

/* Includes */

#include <stdio.h>

#include "main.h"

#include "ssd1306.h"
#include "fonts.h"

#include "SK6812.h"

#include "DS3231.h"

#include "stm32l0xx_EEPROM.h"

/* Macro */

#define ADC_JOY_X (adcV[0])
#define ADC_JOY_Y (adcV[1])

#define JOY_R (ADC_JOY_X > 3500)
#define JOY_L (ADC_JOY_X < 300)
#define JOY_U (ADC_JOY_Y > 3500)
#define JOY_D (ADC_JOY_Y < 300)
#define JOY_P (!HAL_GPIO_ReadPin(JOY_SW_GPIO_Port, JOY_SW_Pin))

#define BUZ(x) (HAL_GPIO_WritePin(BUZ_GPIO_Port, BUZ_Pin, x))

/* Variables */

extern ADC_HandleTypeDef hadc;

extern I2C_HandleTypeDef hi2c1;

extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_tx;

extern TIM_HandleTypeDef htim2;

/* Functions */

void PSDrawers_Initialized(void);
void PSDrawers_Main(void);

#endif /* INC_TASK_H_ */
