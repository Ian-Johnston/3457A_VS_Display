/**
  ******************************************************************************
  * @file    timer.h
  * @brief   This file contains all the function prototypes for
  *          the timer.c file
  ******************************************************************************
*/

#ifndef TIMER_H
#define TIMER_H

#include "stm32f1xx.h" // Include the STM32 HAL/LL header file
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_tim.h" // For TIM APIs and TIM_HandleTypeDef

// External TIM3 handle
extern TIM_HandleTypeDef htim3;

//***********************************************************************************
// Timer 2
 
// Externally accessible variables for TIM2
extern volatile uint8_t timer_flag;
extern volatile uint8_t task_ready;

// Function prototypes
void TIM2_Init(void);
void TIM2_IRQHandler(void);
void SetTimerDuration(uint16_t ms);


//***********************************************************************************
// Timer 3

// Externally accessible variables for TIM3
extern volatile uint16_t captured_value;
extern volatile uint8_t tim3_capture_flag;

// Function prototypes for TIM3
void TIM3_Init_InputCapture(void);
void TIM3_IRQHandler(void);
uint16_t TIM3_GetCapturedValue(void);


#endif // TIMER_H


