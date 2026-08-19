#include "driver_buzzer_pwm.h"

// --- RCC Registers ---
#define RCC_IOPENR    (*(volatile uint32_t *)(0x40021000 + 0x34))
#define RCC_APBENR2 (*(volatile uint32_t *)(0x40021000 + 0x40))

// --- TIM1 Registers ---
#define TIM1_CR1      (*(volatile uint32_t *)(0x40012C00 + 0x00))
#define TIM1_CCMR1    (*(volatile uint32_t *)(0x40012C00 + 0x18))
#define TIM1_CCER     (*(volatile uint32_t *)(0x40012C00 + 0x20))
#define TIM1_PSC      (*(volatile uint32_t *)(0x40012C00 + 0x28))
#define TIM1_ARR      (*(volatile uint32_t *)(0x40012C00 + 0x2C))
#define TIM1_CCR2     (*(volatile uint32_t *)(0x40012C00 + 0x38))
#define TIM1_BDTR     (*(volatile uint32_t *)(0x40012C00 + 0x44))

// --- GPIO Registers ---
#define GPIOB_MODER   (*(volatile uint32_t *)(0x50000400 + 0x00))
#define GPIOB_AFRL    (*(volatile uint32_t *)(0x50000400 + 0x20))
#define GPIOC_MODER   (*(volatile uint32_t *)(0x50000800 + 0x00))
#define GPIOC_PUPDR   (*(volatile uint32_t *)(0x50000800 + 0x0C))
#define GPIOC_IDR     (*(volatile uint32_t *)(0x50000800 + 0x10))

// --- SysTick Registers ---
#define SYST_CSR     (*(volatile uint32_t *)(0xE000E010)) 
#define SYST_RVR     (*(volatile uint32_t *)(0xE000E014)) 
#define SYST_CVR     (*(volatile uint32_t *)(0xE000E018)) 

static volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void) {
    ms_ticks++;
}

static void SysTick_Init(void) {
    SYST_RVR = 16000U - 1U; // 1ms tick at 16 MHz HSI
    SYST_CVR = 0U;
    SYST_CSR = 7U;          // Enable Clock, Interrupt, and Counter
}

void Buzzer_Init(void) {
    // 1. Enable Clocks for GPIOB, GPIOC, and TIM1
    RCC_IOPENR |= (3U << 1);     
    RCC_APBENR2 |= (1U << 11);   
    
    // 2. Configure PB3 -> AF1 (TIM1_CH2)
    GPIOB_MODER &= ~(3U << 6);
    GPIOB_MODER |= (2U << 6);    
    GPIOB_AFRL &= ~(15U << 12);
    GPIOB_AFRL |= (1U << 12);   

    // 3. Configure PC2 -> Input with Internal Pull-Up
    GPIOC_MODER &= ~(3U << 4);   
    GPIOC_PUPDR |= (1U << 4);    

    // 4. Configure TIM1 PWM
    TIM1_PSC = 15U;              // 16 MHz / (15 + 1) = 1 MHz tick rate
    TIM1_CCMR1 &= ~(7U << 12);
    TIM1_CCMR1 |= (6U << 12);    // PWM Mode 1
    TIM1_CCER |= (1U << 4);      // Enable Channel 2 Output
    TIM1_BDTR |= (1U << 15);     // Main Output Enable (MOE)

    // 5. Start SysTick Timer
    SysTick_Init();
}

void Buzzer_Update(void) {
    static uint32_t last_tone_swap = 0U;
    static uint8_t current_tone = 0U;
    static uint8_t was_pressed = 0U;

    uint8_t is_pressed = ((GPIOC_IDR & (1U << 2)) == 0);

    if (is_pressed) {
        // Immediate pitch load on transition from idle -> active
        if (!was_pressed) {
            TIM1_ARR = 384U;       // Tone 1: 2.6 kHz
            TIM1_CCR2 = 192U;      // 50% Duty
            current_tone = 1;
            last_tone_swap = ms_ticks;
            TIM1_CR1 |= (1U << 0); // Enable counter (CEN = 1)
            was_pressed = 1;
        }

        // Alternate tones every 200ms
        if ((ms_ticks - last_tone_swap) >= 200U) {
            last_tone_swap = ms_ticks;

            if (current_tone == 0){
                TIM1_ARR = 384U;   // Tone 1: 2.6 kHz
                TIM1_CCR2 = 192U;
                current_tone = 1;
            } 
            else{
                TIM1_ARR = 769U;   // Tone 2: 1.3 kHz
                TIM1_CCR2 = 385U;
                current_tone = 0;
            }
        }
    } 
    else {
        TIM1_CR1 &= ~(1U << 0);    // Disable counter (Clear CEN)
        current_tone = 0;
        was_pressed = 0;
    }
}