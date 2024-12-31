/**
  ******************************************************************************
  * @file    timer.c
  * @brief   This file provides code for the configuration
  *          of the timer, and used in main.c
  ******************************************************************************
*/

#include "timer.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_tim.h"

//***********************************************************************************
// Timer 2 - Timed Action loop
// Timer interrupt sets a flag at regular intervals.
// Use your main loop to monitor this flag and ensures other conditions (like task completion)
// are met before running the dependent subroutine.  

// Timer flag variables
volatile uint8_t timer_flag = 0;
volatile uint8_t task_ready = 0;

// Timer 2 - initialization function
void TIM2_Init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable TIM2 clock

    TIM2->PSC = 7200 - 1;               // Prescaler: 7200 (72 MHz / 7200 = 10 kHz)
    TIM2->ARR = 5000 - 1;               // Default Auto-reload: 500 ms
    TIM2->DIER |= TIM_DIER_UIE;         // Enable update interrupt
    TIM2->CR1 |= TIM_CR1_CEN;           // Enable the timer

    NVIC_EnableIRQ(TIM2_IRQn);          // Enable TIM2 interrupt in NVIC
}

// Timer 2 - interrupt handler
void TIM2_IRQHandler(void) {

    if (TIM2->SR & TIM_SR_UIF) { // Check for update interrupt
        TIM2->SR &= ~TIM_SR_UIF; // Clear the interrupt flag
        timer_flag = 1;          // Set the timer flag
    }
}

// Timer 2 - Dynamic timer duration setting function
void SetTimerDuration(uint16_t ms) {
    // Calculate ARR based on the desired duration in milliseconds
    TIM2->ARR = (10 * ms) - 1; // For a 10 kHz timer clock (1 tick = 0.1 ms)
    TIM2->EGR = TIM_EGR_UG;    // Force update to apply changes immediately
}


//***********************************************************************************
// Timer 3 - Input Capture Functionality

TIM_HandleTypeDef htim3;

// Timer 3 input capture flag
volatile uint16_t captured_value = 0;
volatile uint8_t tim3_capture_flag = 0;

// Timer 3 - Initialization function
void TIM3_Init_InputCapture(void) {
    // Initialize TIM3 Instance
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 72 - 1;              // 1 MHz resolution (72 MHz / 72)
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 0xFFFF;                 // Maximum 16-bit period
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    // Initialize TIM3 base
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
        Error_Handler(); // Initialization error
    }

    // Configure Input Capture Channel 4
    TIM_IC_InitTypeDef sConfigIC;
    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING; // Rising edge trigger
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;       // Direct input
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;                 // No prescaler
    sConfigIC.ICFilter = 0;                                 // No filter

    if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_4) != HAL_OK) {
        Error_Handler(); // Channel configuration error
    }

    // Start Input Capture in interrupt mode
    if (HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_4) != HAL_OK) {
        Error_Handler(); // Start error
    }

    // Enable TIM3 IRQ
    HAL_NVIC_SetPriority(TIM3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);

    TIM3->CR1 |= TIM_CR1_CEN;
}




// Timer 3 - Interrupt handler - OK
void TIM3_IRQHandler(void) {

    HAL_TIM_IRQHandler(&htim3); // Ensure this calls the HAL library to handle events

}


// Timer 3 - Get captured value
uint16_t TIM3_GetCapturedValue(void) {

    if (tim3_capture_flag) {
        tim3_capture_flag = 0;            // Clear the flag after reading
        return captured_value;            // Return the captured value
    }
    return 0;
}




