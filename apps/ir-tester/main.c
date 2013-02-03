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
    RCC->APB2ENR |= RCC_APB2ENR_TIM17EN;

    // Prescaler keeps zero to allow control PWM frequency
    TIM17->PSC = 0;
    // Autoreload register keeps 24MHz/36kHz value
    TIM17->ARR = 667 - 1;
    TIM17->CCR1 =333; // 50% duty cycle

    // Output Compare PWM1 Mode
    TIM17->CCMR1 |= (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1);

    // Capture/Compare 1 output enable, ie PB9
    TIM17->CCER |= TIM_CCER_CC1E;

    // Main Output enable
    TIM17->BDTR |= TIM_BDTR_MOE;

    // TIM17 start and enable autoreload
    TIM17->CR1 |= (TIM_CR1_CEN | TIM_CR1_ARPE);
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
