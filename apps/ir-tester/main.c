#include <ch.h>
#include <hal.h>
#include <stm32f10x.h>

// Simple thread for green LED blinkng.
static WORKING_AREA(waBlinker, 128);
static msg_t blinker(void *arg) {
    (void) arg;
    chRegSetThreadName("blinker");
    while (!chThdShouldTerminate()) {
        palTogglePad(GPIOB, GPIOB_ARMKA_LED);
        chThdSleepMilliseconds(500);
    }
    return 0;
}

void meandre_tim17(void) {
    //Enable clock for the timer
    RCC->APB2ENR |= RCC_APB2ENR_TIM17EN;
    // Enable ARR preloading
    TIM17->CR1 |= TIM_CR1_ARPE;
    // Enable CCR1 preloading
    TIM17->CCMR1 |= TIM_CCMR1_OC2PE;
    // PWM mode 1: OC1M = 110
    TIM17->CCMR1 |= (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1);
    // Prescaler keeps zero to allow control PWM frequency
    TIM17->PSC = 0;
    // PWM Period: 1ms
    TIM17->ARR = 667 - 1;
    // Duty cycle: 50%
    TIM17->CCR1 = 333;
    // Uncomment to change polarity
    //TIM2->CCER |= TIM_CCER_CC2P;
    // Enable Capture/Compare output 1, ie PB9
    TIM17->CCER |= TIM_CCER_CC1E;
    // Main Output enable
    TIM17->BDTR |= TIM_BDTR_MOE;
    // Start timer
    TIM17->CR1 |= TIM_CR1_CEN;
}

// Program entry point.
int main(void) {

    // System initialization.
    halInit();
    chSysInit();

    meandre_tim17();

    // Run blinker
    (void) chThdCreateStatic(waBlinker, sizeof(waBlinker), NORMALPRIO, blinker, NULL);

    // Main thread.
    while (!chThdShouldTerminate()) {
        chThdSleepMilliseconds(MS2ST(500));
    }
}
