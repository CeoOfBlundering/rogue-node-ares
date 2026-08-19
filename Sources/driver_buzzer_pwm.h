#ifndef DRIVER_BUZZER_PWM_H
#define DRIVER_BUZZER_PWM_H

#include <stdint.h>

/**
 * @brief  Initializes GPIOB Pin 3 (TIM1_CH2), GPIOC Pin 2 (Button Input),
 *         TIM1 PWM hardware, and SysTick for 1ms tick interrupts.
 */
void Buzzer_Init(void);

/**
 * @brief  Non-blocking state machine update routine.
 *         Polls button state on PC2 and handles dual-tone siren PWM frequency
 *         swapping every 200ms without blocking execution. Call within main while(1).
 */
void Buzzer_Update(void);

#endif // DRIVER_BUZZER_PWM_H